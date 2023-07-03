// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <iostream>
#include <string>
#include "index.hpp"
#include "index_builder.hpp"
#include "util.hpp"
//#include "access/reloptions.h"

extern "C"
{
#include <access/relscan.h>
#include <commands/vacuum.h>
#include <miscadmin.h>
#include <utils/elog.h>
#include <utils/rel.h>
#include <utils/selfuncs.h>
#include <omp.h>
#include <access/reloptions.h>
}

extern relopt_kind sptag_para_relopt_kind;

std::shared_ptr<SPTAG::Helper::Logger> g_pLogger(
    new SPTAG::Helper::SimpleLogger(SPTAG::Helper::LogLevel::LL_Info));

bytea *sptag_para_options(Datum reloptions, bool validate) {

  static const relopt_parse_elt tab[] = {
      {"threads", RELOPT_TYPE_INT, offsetof(sptag_ParaOptions, threads)},
      {"distmethod", RELOPT_TYPE_ENUM, offsetof(sptag_ParaOptions, distmethod)}
  };

  return (bytea *)build_reloptions(reloptions, validate, sptag_para_relopt_kind, sizeof(sptag_ParaOptions), tab, lengthof(tab));
}

CPP_PG_FUNCTION_INFO_V1(sptag_handler);
Datum sptag_handler(PG_FUNCTION_ARGS)
{
    IndexAmRoutine *amroutine = makeNode(IndexAmRoutine);

    amroutine->amstrategies = 0;
    amroutine->amsupport = 4;
#if PG_VERSION_NUM >= 130000
    amroutine->amoptsprocnum = 0;
#endif
    amroutine->amcanorder = false;
    amroutine->amcanorderbyop = true;
    amroutine->amcanbackward = false;  // can change direction mid-scan
    amroutine->amcanunique = false;
    amroutine->amcanmulticol = false;
    amroutine->amoptionalkey = true;
    amroutine->amsearcharray = false;
    amroutine->amsearchnulls = false;
    amroutine->amstorage = true;
    amroutine->amclusterable = false;
    amroutine->ampredlocks = false;
#if PG_VERSION_NUM >= 100000
    amroutine->amcanparallel = false;
#endif
#if PG_VERSION_NUM >= 110000
    amroutine->amcaninclude = false;
#endif
#if PG_VERSION_NUM >= 130000
    amroutine->amusemaintenanceworkmem = false;  // not used during VACUUM
    amroutine->amparallelvacuumoptions =
        VACUUM_OPTION_NO_PARALLEL;  // FIXME: support parallel vacumm
#endif
    amroutine->amkeytype = 0;

    // For a brief introduction of what these functions are used for, see
    // https://www.postgresql.org/docs/13/index-functions.html.
    amroutine->ambuild = sptag_build;
    amroutine->ambuildempty = nullptr;     // TODO: index access method
    amroutine->aminsert = nullptr;         // TODO: index access method
    amroutine->ambulkdelete = nullptr;     // TODO: index access method
    amroutine->amvacuumcleanup =
        sptag_vacuumcleanup;
    amroutine->amcanreturn = nullptr;
    amroutine->amcostestimate = sptag_costestimate;
    amroutine->amoptions = sptag_para_options;  // TODO: index access method
    amroutine->amproperty = nullptr; /* TODO AMPROP_DISTANCE_ORDERABLE */
#if PG_VERSION_NUM >= 120000
    amroutine->ambuildphasename = nullptr;
#endif
    amroutine->amvalidate = sptag_validate;
    amroutine->ambeginscan = sptag_begin_scan;
    amroutine->amrescan = sptag_rescan;
    amroutine->amgettuple = sptag_gettuple;
    amroutine->amgetbitmap = nullptr;
    amroutine->amendscan = sptag_endscan;
    amroutine->ammarkpos = nullptr;
    amroutine->amrestrpos = nullptr;
#if PG_VERSION_NUM >= 100000
    amroutine->amestimateparallelscan = nullptr;
    amroutine->aminitparallelscan = nullptr;
    amroutine->amparallelrescan = nullptr;
#endif

    PG_RETURN_POINTER(amroutine);
}

IndexBuildResult *sptag_build(Relation heap,
                              Relation index,
                              IndexInfo *indexInfo)
{
    IndexBuildResult *result;
    try
    {
        auto builder = IndexBuilder(heap, index, indexInfo);
        builder.ConstructInternalBuilder();
        builder.LoadTuples();
        auto vector = builder.GetVectorSet();
        auto metadata = builder.GetMetadataSet();
        builder.BuildIndex(vector, metadata);
        std::string path = std::string(DataDir) + std::string("/") +
            std::string(DatabasePath) + std::string("/") +
            std::string(RelationGetRelationName(index));
        builder.SaveIndex(path);
        result = (IndexBuildResult *) palloc(sizeof(IndexBuildResult));
        result->heap_tuples = builder.m_reltuples;
        result->index_tuples = builder.m_indtuples;
    }
    catch (std::exception &ex)
    {
        ereport(FATAL, errcode(0), errmsg("C++ exception: %s", ex.what()));
    }
    catch (...)
    {
        ereport(FATAL, errcode(0), errmsg("C++ exception"));
    }
    return result;
}

IndexScanDesc sptag_begin_scan(Relation index, int nkeys, int norderbys)
{
    std::string path = std::string(DataDir) + std::string("/") +
        std::string(DatabasePath) + std::string("/") +
        std::string(RelationGetRelationName(index));
    IndexScan::LoadIndex(path);

    IndexScanDesc scan = RelationGetIndexScan(index, nkeys, norderbys);

    scan->xs_orderbyvals = (Datum *) palloc0(sizeof(Datum));
    scan->xs_orderbynulls = (bool *) palloc(sizeof(bool));

    SPTAGScanOpaque scanState =
        (SPTAGScanOpaque) palloc(sizeof(SPTAGScanOpaqueData));
    scanState->first = true;
    scanState->workSpace = nullptr;
    scan->opaque = scanState;
    return scan;
}

void sptag_rescan(IndexScanDesc scan,
                  ScanKey keys,
                  int nkeys,
                  ScanKey orderbys,
                  int norderbys)
{
    SPTAGScanOpaque scanState = (SPTAGScanOpaque) scan->opaque;

    if (scanState->workSpace != nullptr)
    {
        IndexScan::EndScan(scanState->workSpace->resultIterator);
        scanState->data->Clear();
        delete scanState->data;
        scanState->workSpace->resultIterator = nullptr;
        delete scanState->workSpace;
        scanState->workSpace = nullptr;
    }
    scanState->first = true;

    if (keys && scan->numberOfKeys > 0)
        memmove(scan->keyData, keys, scan->numberOfKeys * sizeof(ScanKeyData));

    if (orderbys && scan->numberOfOrderBys > 0)
        memmove(scan->orderByData,
                orderbys,
                scan->numberOfOrderBys * sizeof(ScanKeyData));
}

/*
 * Fetch the next tuple in the given scan
 */
bool sptag_gettuple(IndexScanDesc scan, ScanDirection dir)
{
    SPTAGScanOpaque scanState = (SPTAGScanOpaque) scan->opaque;
    /*
     * Index can be used to scan backward, but Postgres doesn't support
     * backward scan on operators
     */
    Assert(ScanDirectionIsForward(dir));

    if (scanState->first)
    {
        scanState->workSpace = new IndexScan::WorkSpace();
	    scanState->data = new SPTAG::ByteArray();

        if (scan->orderByData == NULL)
        {
            if (scan->keyData == NULL)
                return false;
            if (scan->keyData->sk_flags & SK_ISNULL)
                return false;

            Datum value = scan->keyData->sk_argument;
            std::vector<float> array = convert_array_to_vector(value);
            scanState->hasRangeFilter = true;
            scanState->inRange = false;
            scanState->range = array[0];
            auto data_value = SPTAG::ByteArray::Alloc((array.size() - 1) * sizeof(float));
	        *(scanState->data) = data_value;
            std::copy(reinterpret_cast<std::uint8_t *>(array.data() + 1),
                      reinterpret_cast<std::uint8_t *>(array.data() + array.size()),
                      scanState->data->Data());
        }
        else{
            if (scan->orderByData->sk_flags & SK_ISNULL)
                return false;
            Datum value = scan->orderByData->sk_argument;
            std::vector<float> array = convert_array_to_vector(value);
            scanState->hasRangeFilter = false;
            auto data_value = SPTAG::ByteArray::Alloc(array.size() * sizeof(float));
	        *(scanState->data) = data_value;
            std::copy(reinterpret_cast<std::uint8_t *>(array.data()),
                      reinterpret_cast<std::uint8_t *>(array.data() + array.size()),
                      scanState->data->Data());
        }

        std::string path = std::string(DataDir) + std::string("/") +
            std::string(DatabasePath) + std::string("/") +
            std::string(RelationGetRelationName(scan->indexRelation));
        scanState->workSpace->resultIterator = IndexScan::BeginScan(scanState->data->Data(),path);
        scanState->first = false;
    }

    SPTAG::BasicResult result;
    int i = 0;
    //TODO(Qianxi): set parameter
    int threshold = 5;
    while(true)
    {
        bool validResult = IndexScan::GetNet(scanState->workSpace->resultIterator, result);
        if (!validResult)
        {
            return false;
        }

        if (scanState->hasRangeFilter)
        {
            if(result.Dist > scanState->range)
            {
                i++;
                if (scanState->inRange && i >= threshold)
                {
                    return false;
                }
                else
                {
                    continue;
                }
            }
            else
            {
                scanState->inRange = true;
            }
        }

        std::uint64_t number;
        std::memcpy(&number, result.Meta.Data(), 8);
        BlockNumber blkno = (std::uint32_t) (number >> 32);
        OffsetNumber offset = (std::uint32_t) number;
    #if PG_VERSION_NUM >= 120000
        ItemPointerSet(&scan->xs_heaptid, blkno, offset);
    #else
        ItemPointerSet(&scan->xs_ctup.t_self, blkno, offset);
    #endif
        scan->xs_orderbyvals[0] = Float4GetDatum(result.Dist);
        scan->xs_orderbynulls[0] = false;
        scan->xs_recheckorderby = false;
        return true;
    }
}

/*
 * End a scan and release resources
 */
void sptag_endscan(IndexScanDesc scan)
{
    SPTAGScanOpaque scanState = (SPTAGScanOpaque) scan->opaque;
    IndexScan::EndScan(scanState->workSpace->resultIterator);
    scanState->data->Clear();
    delete scanState->data;
    scanState->workSpace->resultIterator = nullptr;
    delete scanState->workSpace;
    pfree(scanState);
    scan->opaque = NULL;
}

/*
 * Validate catalog entries for the specified operator class
 */
bool sptag_validate(Oid opclassoid)
{
    return true;
}

void sptag_costestimate(PlannerInfo *root,
                        IndexPath *path,
                        double loop_count,
                        Cost *indexStartupCost,
                        Cost *indexTotalCost,
                        Selectivity *indexSelectivity,
                        double *indexCorrelation
#if PG_VERSION_NUM >= 100000
                        ,
                        double *indexPages
#endif
)
{
    /* Startup cost and total cost are same */
    *indexStartupCost = 0;
    *indexTotalCost = 0.01;
    *indexSelectivity = 1;
    *indexCorrelation = 1;
#if PG_VERSION_NUM >= 100000
    *indexPages = 1;
#endif
}

/*
 * Clean up after a VACUUM operation
 */
IndexBulkDeleteResult *sptag_vacuumcleanup(IndexVacuumInfo *info,
                                          IndexBulkDeleteResult *stats)
{
    Relation rel = info->index;

    if (stats == NULL)
        return NULL;

    stats->num_pages = 0;

    return stats;
}
