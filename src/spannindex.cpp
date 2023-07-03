// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <iostream>
#include <string>
#include "spannindex_builder.hpp"
#include "spannindex.hpp"
#include "util.hpp"
//#include "access/reloptions.h"

extern "C"
{
#include <access/reloptions.h>
#include <access/relscan.h>
#include <commands/vacuum.h>
#include <miscadmin.h>
#include <omp.h>
#include <utils/elog.h>
#include <utils/rel.h>
#include <utils/selfuncs.h>
}
/*
extern relopt_kind spann_para_relopt_kind;

//std::shared_ptr<SPTAG::Helper::Logger> g_pLogger(
//    new SPTAG::Helper::SimpleLogger(SPTAG::Helper::LogLevel::LL_Info));

bytea *spann_para_options(Datum reloptions, bool validate)
{
    static const relopt_parse_elt tab[] = {
        {"batch", RELOPT_TYPE_INT, offsetof(spann_ParaOptions, batch)},
        {"distmethod",
         RELOPT_TYPE_ENUM,
         offsetof(spann_ParaOptions, distmethod)}};

    return (bytea *) build_reloptions(reloptions,
                                      validate,
                                      spann_para_relopt_kind,
                                      sizeof(spann_ParaOptions),
                                      tab,
                                      lengthof(tab));
}
*/
CPP_PG_FUNCTION_INFO_V1(spann_handler);
Datum spann_handler(PG_FUNCTION_ARGS)
{
    IndexAmRoutine *amroutine = makeNode(IndexAmRoutine);

    amroutine->amstrategies = 0;
    amroutine->amsupport = 4;
#if PG_VERSION_NUM >= 130000
    amroutine->amoptsprocnum = 0;
#endif
    amroutine->amcanorder = false;
    amroutine->amcanorderbyop = true;
    amroutine->amcanrelaxedorderbyop=true;
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
    amroutine->ambuild = spann_build;
    amroutine->ambuildempty = nullptr;  // TODO: index access method
    amroutine->aminsert = nullptr;      // TODO: index access method
    amroutine->ambulkdelete = nullptr;  // TODO: index access method
    amroutine->amvacuumcleanup = spann_vacuumcleanup;
    amroutine->amcanreturn = nullptr;
    amroutine->amcostestimate = spann_costestimate;
    amroutine->amoptions = nullptr;  // TODO: index access method
    amroutine->amproperty = nullptr; /* TODO AMPROP_DISTANCE_ORDERABLE */
#if PG_VERSION_NUM >= 120000
    amroutine->ambuildphasename = nullptr;
#endif
    amroutine->amvalidate = spann_validate;
    amroutine->ambeginscan = spann_begin_scan;
    amroutine->amrescan = spann_rescan;
    amroutine->amgettuple = spann_gettuple;
    amroutine->amgetbitmap = nullptr;
    amroutine->amendscan = spann_endscan;
    amroutine->ammarkpos = nullptr;
    amroutine->amrestrpos = nullptr;
#if PG_VERSION_NUM >= 100000
    amroutine->amestimateparallelscan = nullptr;
    amroutine->aminitparallelscan = nullptr;
    amroutine->amparallelrescan = nullptr;
#endif

    PG_RETURN_POINTER(amroutine);
}

IndexBuildResult *spann_build(Relation heap,
                              Relation index,
                              IndexInfo *indexInfo)
{
    IndexBuildResult *result;
    try
    {
        auto builder = SPANNIndexBuilder(heap, index, indexInfo);
        builder.LoadTuples();
        std::string path =
            std::string(DataDir) + std::string("/") + 
	    std::string(DatabasePath) + std::string("/");
       	//+ std::string(RelationGetRelationName(index));
        builder.SaveIndex(path);
        result = (IndexBuildResult *) palloc(sizeof(IndexBuildResult));
        result->heap_tuples = builder.m_reltuples;
        result->index_tuples = builder.m_indtuples;
        ereport(INFO, errmsg("Saved to %s", path.c_str()));
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

IndexScanDesc spann_begin_scan(Relation index, int nkeys, int norderbys)
{
   // std::string path = std::string(DataDir) + std::string("/") +
   //                    std::string(DatabasePath) + std::string("/") +
   //                    std::string(RelationGetRelationName(index));
    std::string path = std::string("/") + std::string(RelationGetRelationName(index)) + std::string("/");
    SPANNIndexScan::LoadIndex(path);

    IndexScanDesc scan = RelationGetIndexScan(index, nkeys, norderbys);

    scan->xs_orderbyvals = (Datum *) palloc0(sizeof(Datum));
    scan->xs_orderbynulls = (bool *) palloc(sizeof(bool));

    SPANNScanOpaque scanState =
        (SPANNScanOpaque) palloc(sizeof(SPANNScanOpaqueData));
    scanState->first = true;
    scanState->workSpace = nullptr;
    scan->opaque = scanState;
    return scan;
}

void spann_rescan(IndexScanDesc scan,
                  ScanKey keys,
                  int nkeys,
                  ScanKey orderbys,
                  int norderbys)
{
    SPANNScanOpaque scanState = (SPANNScanOpaque) scan->opaque;

    if (scanState->workSpace != nullptr)
    {
        SPANNIndexScan::EndScan(scanState->workSpace->resultIterator);
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
bool spann_gettuple(IndexScanDesc scan, ScanDirection dir)
{
    SPANNScanOpaque scanState = (SPANNScanOpaque) scan->opaque;
    /*
     * Index can be used to scan backward, but Postgres doesn't support
     * backward scan on operators
     */
    Assert(ScanDirectionIsForward(dir));

    if (scanState->first)
    {
        scanState->workSpace = new SPANNIndexScan::WorkSpace();
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
            auto data_value =
                SPTAG::ByteArray::Alloc((array.size() - 1) * sizeof(float));
            *(scanState->data) = data_value;
            std::copy(
                reinterpret_cast<std::uint8_t *>(array.data() + 1),
                reinterpret_cast<std::uint8_t *>(array.data() + array.size()),
                scanState->data->Data());
        }
        else
        {
            if (scan->orderByData->sk_flags & SK_ISNULL)
                return false;
            Datum value = scan->orderByData->sk_argument;
            std::vector<float> array = convert_array_to_vector(value);
            scanState->hasRangeFilter = false;
            scanState->range = 1500;
	    auto data_value =
                SPTAG::ByteArray::Alloc(array.size() * sizeof(float));
            *(scanState->data) = data_value;
            std::copy(
                reinterpret_cast<std::uint8_t *>(array.data()),
                reinterpret_cast<std::uint8_t *>(array.data() + array.size()),
                scanState->data->Data());
        }

       // std::string path =
       //     std::string(DataDir) + std::string("/") +
       //     std::string(DatabasePath) + std::string("/") +
       //     std::string(RelationGetRelationName(scan->indexRelation));
        std::string path = std::string("/") + std::string(RelationGetRelationName(scan->indexRelation))
	       	+ std::string("/");
       	scanState->workSpace->resultIterator =
            SPANNIndexScan::BeginScan(scanState->data->Data(), path);
	//scan->xs_inorder = false;
        scan->xs_inorder = true;
        scanState->first = false;
	int queueThreshold = 10000;
        SPTAG::BasicResult result;
        while (true)
        {
            bool validResult = SPANNIndexScan::GetNet(
                scanState->workSpace->resultIterator, result);
            if (!validResult || result.VID == -2)
            {
		   //ereport(INFO, errmsg("In Order, next cycle")); 
                    break;
            }
	    scanState->workSpace->distanceQueue.push(result);
	    if (scanState->workSpace->distanceQueue.size() >= queueThreshold)
	    {
		    break;
	    }
        }
    }
    SPTAG::BasicResult result;
    int i = 0;
    // TODO(Qianxi): set parameter
    int distanceThreshold = 200;
    //int queueThreshold = 3000;
    while (true)
    {
	if (!scanState->workSpace->distanceQueue.empty())
	{
		result = scanState->workSpace->distanceQueue.top();
		scanState->workSpace->distanceQueue.pop();
	}
	else
	{
            bool validResult = SPANNIndexScan::GetNet(
            scanState->workSpace->resultIterator, result);
            if (!validResult)
            {
                return false;
            }
	}
        if (scanState->hasRangeFilter)
        {
            if(result.Dist < scanState->range)
            {
                scanState->inRange = true;
            }
            else if (scanState->inRange)
            {
                i++;
                if (i >= distanceThreshold)
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
                if (scanState->workSpace->distanceQueue.empty())
                {
                    return false;
                }
                else
                {
                    continue;
                }
                
            }
        }
	/*
        bool validResult = SPANNIndexScan::GetNet(
            scanState->workSpace->resultIterator, result);
        if (!validResult)
        {
            return false;
        }

        if (scanState->hasRangeFilter)
        {
            if (!scanState->inRange)
            {
	    */
		    /*
                if (scanState->workSpace->distanceQueue.size() <
                        queueThreshold ||
                    scanState->workSpace->distanceQueue.top() >
                        result.Dist)
                {
                    if (scanState->workSpace->distanceQueue.size() ==
                        queueThreshold)
                    {
                        scanState->workSpace->distanceQueue.pop();
                    }
                    scanState->workSpace->distanceQueue.push(
                        result.Dist);
                }

                {
                    return false;
                }
		*/
	/*
		queueThreshold--;
		if (queueThreshold <= 0)
		{
			return false;
		}
            }
            if(result.Dist > scanState->range)
            {
                i++;
                if (scanState->inRange && i >= distanceThreshold)
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
	*/
	/*
        else if(!scan->xs_inorder){
         if (scanState->workSpace->distanceQueue.size() == scanState->range &&
             scanState->workSpace->distanceQueue.top() < result.Dist)
         {
             scan->xs_inorder=true;
         }
         else
         {
                    if (scanState->workSpace->distanceQueue.size() == scanState->range)
                    {
                    scanState->workSpace->distanceQueue.pop();
                    }
                    scanState->workSpace->distanceQueue.push(result.Dist);
         }
        }
	*/
	/*
        else {
	scanState->range--;
	if (scanState->range <=0 && !scan->xs_inorder)
	{
		scan->xs_inorder = true;
	}
        }
        */
        std::uint64_t number;
        std::memcpy(&number, result.Meta.Data(), 8);
        BlockNumber blkno = (std::uint32_t) (number >> 32);
        OffsetNumber offset = (std::uint32_t) number;
//                     BlockNumber blkno = (std::uint32_t) 0;
//		     	               OffsetNumber offset = (std::uint32_t) 1;
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
void spann_endscan(IndexScanDesc scan)
{
    SPANNScanOpaque scanState = (SPANNScanOpaque) scan->opaque;
    SPANNIndexScan::EndScan(scanState->workSpace->resultIterator);
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
bool spann_validate(Oid opclassoid)
{
    return true;
}

void spann_costestimate(PlannerInfo *root,
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
	  IndexOptInfo *index = path->indexinfo;
	    List *qinfos;
	      GenericCosts costs;
    // Do preliminary analysis of indexquals
//    qinfos = deconstruct_indexquals(path);

    MemSet(&costs, 0, sizeof(costs));

    // We have to visit all index tuples anyway
    costs.numIndexTuples = index->tuples;

    // Use generic estimate
    genericcostestimate(root, path, loop_count, &costs);

    *indexStartupCost = costs.indexStartupCost;
    *indexTotalCost = costs.indexTotalCost;
    *indexSelectivity = 0.01;
    *indexCorrelation = costs.indexCorrelation;
#if PG_VERSION_NUM >= 100000
    *indexPages = costs.numIndexPages;
#endif
}
/*
 * Clean up after a VACUUM operation
 */
IndexBulkDeleteResult *spann_vacuumcleanup(IndexVacuumInfo *info,
                                           IndexBulkDeleteResult *stats)
{
    Relation rel = info->index;

    if (stats == NULL)
        return NULL;

    stats->num_pages = 0;

    return stats;
}
