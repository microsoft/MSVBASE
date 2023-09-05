// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <iostream>
#include <string>

#include "hnswindex.hpp"
#include "hnswindex_builder.hpp"
#include "util.hpp"

extern "C"
{
#include <access/relscan.h>
#include <commands/vacuum.h>
#include <miscadmin.h>
#include <utils/elog.h>
#include <utils/rel.h>
#include <utils/selfuncs.h>
#include <access/reloptions.h>
}

extern relopt_kind hnsw_para_relopt_kind;

bytea *hnsw_para_options(Datum reloptions, bool validate) {

  static const relopt_parse_elt tab[] = {
      {"dimension", RELOPT_TYPE_INT, offsetof(hnsw_ParaOptions, dimension)},
      {"distmethod", RELOPT_TYPE_ENUM, offsetof(hnsw_ParaOptions, distmethod)}
  };

  return (bytea *)build_reloptions(reloptions, validate, hnsw_para_relopt_kind, sizeof(hnsw_ParaOptions), tab, lengthof(tab));
}

int hnsw_ParaGetDimension(Relation index) {
  hnsw_ParaOptions *opts = (hnsw_ParaOptions *)index->rd_options;

  if (opts)
    return opts->dimension;

  return 1;
}

hnsw_DistCalcMethod hnsw_ParaGetDistmethod(Relation index) {
  hnsw_ParaOptions *opts = (hnsw_ParaOptions *)index->rd_options;

  if (opts)
    return opts->distmethod;

  return hnsw_Inner_Product;
}

CPP_PG_FUNCTION_INFO_V1(hnsw_handler);
Datum hnsw_handler(PG_FUNCTION_ARGS)
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
    amroutine->ambuild = hnsw_build;
    amroutine->ambuildempty = nullptr;     // TODO: index access method
    amroutine->aminsert = hnsw_insert;         // TODO: index access method
    amroutine->ambulkdelete = nullptr;     // TODO: index access method
    amroutine->amvacuumcleanup =
        hnsw_vacuumcleanup;
    amroutine->amcanreturn = nullptr;
    amroutine->amcostestimate = hnsw_costestimate;
    amroutine->amoptions = hnsw_para_options;  // TODO: index access method
    amroutine->amproperty = nullptr; /* TODO AMPROP_DISTANCE_ORDERABLE */
#if PG_VERSION_NUM >= 120000
    amroutine->ambuildphasename = nullptr;
#endif
    amroutine->amvalidate = hnsw_validate;
    amroutine->ambeginscan = hnsw_begin_scan;
    amroutine->amrescan = hnsw_rescan;
    amroutine->amgettuple = hnsw_gettuple;
    amroutine->amgetbitmap = nullptr;
    amroutine->amendscan = hnsw_endscan;
    amroutine->ammarkpos = nullptr;
    amroutine->amrestrpos = nullptr;
#if PG_VERSION_NUM >= 100000
    amroutine->amestimateparallelscan = nullptr;
    amroutine->aminitparallelscan = nullptr;
    amroutine->amparallelrescan = nullptr;
#endif

    PG_RETURN_POINTER(amroutine);
}

IndexBuildResult *hnsw_build(Relation heap,
                              Relation index,
                              IndexInfo *indexInfo)
{
    IndexBuildResult *result;
    try
    {
        auto builder = HNSWIndexBuilder(heap, index, indexInfo);
        builder.ConstructInternalBuilder(DistanceMethod::L2);
        builder.LoadTuples();
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

bool hnsw_insert(Relation index,
	Datum* values,
	bool* isnull,
	ItemPointer heap_tid,
	Relation heapRelation,
	IndexUniqueCheck checkUnique,
	IndexInfo* indexInfo)
{
	std::string path = std::string(DataDir) + std::string("/") +
		std::string(DatabasePath) + std::string("/") +
		std::string(RelationGetRelationName(index));
	int dim = hnsw_ParaGetDimension(index);
        switch(hnsw_ParaGetDistmethod(index))
        {
            case hnsw_Inner_Product:
                HNSWIndexScan::LoadIndex(path, DistanceMethod::InnerProduct, dim);
                break;
            case hnsw_L2_Distance:
                HNSWIndexScan::LoadIndex(path, DistanceMethod::L2, dim);
                break;
            default:
                elog(ERROR, "hnsw index parameter value error.");
        }	
	return HNSWIndexScan::Insert(path,
		values,
		isnull,
		heap_tid,
		checkUnique,
		dim);
}

IndexScanDesc hnsw_begin_scan(Relation index, int nkeys, int norderbys)
{
    std::string path = std::string(DataDir) + std::string("/") +
                       std::string(DatabasePath) + std::string("/") +
                       std::string(RelationGetRelationName(index));
    //TODO(qianxi): how to get distance method and dimension
    switch(hnsw_ParaGetDistmethod(index))
    {
        case hnsw_Inner_Product:
            HNSWIndexScan::LoadIndex(path, DistanceMethod::InnerProduct, hnsw_ParaGetDimension(index));
            break;
        case hnsw_L2_Distance:
            HNSWIndexScan::LoadIndex(path, DistanceMethod::L2, hnsw_ParaGetDimension(index));
            break;
        default:
            elog(ERROR, "hnsw index parameter value error.");
    }

    IndexScanDesc scan = RelationGetIndexScan(index, nkeys, norderbys);
    scan->xs_orderbyvals = (Datum *) palloc0(sizeof(Datum));
    scan->xs_orderbynulls = (bool *) palloc(sizeof(bool));
    HNSWScanOpaque scanState =
        (HNSWScanOpaque) palloc(sizeof(HNSWScanOpaqueData));
    scanState->first = true;
    scanState->workSpace = nullptr;
    scan->opaque = scanState;
    return scan;
}

void hnsw_rescan(IndexScanDesc scan,
                  ScanKey keys,
                  int nkeys,
                  ScanKey orderbys,
                  int norderbys)
{
    HNSWScanOpaque scanState = (HNSWScanOpaque) scan->opaque;
    
    if (scanState->workSpace != nullptr)
    {
        HNSWIndexScan::EndScan(scanState->workSpace->resultIterator);
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
bool hnsw_gettuple(IndexScanDesc scan, ScanDirection dir)
{
    HNSWScanOpaque scanState = (HNSWScanOpaque) scan->opaque;
    /*
     * Index can be used to scan backward, but Postgres doesn't support
     * backward scan on operators
     */
    Assert(ScanDirectionIsForward(dir));

    if (scanState->first)
    {
       scanState->workSpace = new HNSWIndexScan::WorkSpace();
       std::string path = std::string(DataDir) + std::string("/") +
                        std::string(DatabasePath) + std::string("/") +
                        std::string(RelationGetRelationName(scan->indexRelation));
       if (scan->orderByData == NULL)
        {
            if (scan->keyData == NULL)
                return false;
            if (scan->keyData->sk_flags & SK_ISNULL)
                return false;

            Datum value = scan->keyData->sk_argument;
	        scanState->workSpace->array = convert_array_to_vector(value);
            scanState->hasRangeFilter = true;
            scanState->inRange = false;
            scanState->range = scanState->workSpace->array[0];
            scanState->workSpace->resultIterator =
                       HNSWIndexScan::BeginScan((char *)(scanState->workSpace->array.data() + 1),path);
        }
        else{
            if (scan->orderByData->sk_flags & SK_ISNULL)
                return false;
            Datum value = scan->orderByData->sk_argument;
	        scanState->workSpace->array = convert_array_to_vector(value);
            scanState->hasRangeFilter = false;
	    scanState->range = 86;
            scanState->workSpace->resultIterator =
                       HNSWIndexScan::BeginScan((char *)scanState->workSpace->array.data(),path);
        }
        scan->xs_inorder = false;
        scanState->first = false;
    }

    int i = 0;
    //TODO(Qianxi): set parameter
    int distanceThreshold = 3;
    int queueThreshold = 50;
    while(true)
    {
        hnswlib::QueryResult<float> *result =
            HNSWIndexScan::GetNet(scanState->workSpace->resultIterator);

        if (!result->HasResult())
        {
            return false;
        }
        if (scanState->hasRangeFilter)
        {
            if (!scanState->inRange)
            {
                if (scanState->workSpace->distanceQueue.size() <
                        queueThreshold ||
                    scanState->workSpace->distanceQueue.top() >
                        result->GetDistance())
                {
                    if (scanState->workSpace->distanceQueue.size() ==
                        queueThreshold)
                    {
                        scanState->workSpace->distanceQueue.pop();
                    }
                    scanState->workSpace->distanceQueue.push(
                        result->GetDistance());
                }
                else
                {
                    return false;
                }
            }
            if(result->GetDistance() > scanState->range)
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
        else if(!scan->xs_inorder){
         if (scanState->workSpace->distanceQueue.size() == scanState->range &&
             scanState->workSpace->distanceQueue.top() < result->GetDistance())
         {
             scan->xs_inorder=true;
         }
         else
         {
                    if (scanState->workSpace->distanceQueue.size() == scanState->range)
                    {
                    scanState->workSpace->distanceQueue.pop();
                    }
                    scanState->workSpace->distanceQueue.push(result->GetDistance());
         }
        }

        std::uint64_t number = result->GetLabel();
        BlockNumber blkno = (std::uint32_t) (number >> 32);
        OffsetNumber offset = (std::uint32_t) number;
    #if PG_VERSION_NUM >= 120000
        ItemPointerSet(&scan->xs_heaptid, blkno, offset);
    #else
        ItemPointerSet(&scan->xs_ctup.t_self, blkno, offset);
    #endif
        scan->xs_orderbyvals[0] = Float4GetDatum(result->GetDistance());
        scan->xs_orderbynulls[0] = false;
        scan->xs_recheckorderby = false;
        return true;
    }
}

/*
 * End a scan and release resources
 */
void hnsw_endscan(IndexScanDesc scan)
{
    HNSWScanOpaque scanState = (HNSWScanOpaque) scan->opaque;
    HNSWIndexScan::EndScan(scanState->workSpace->resultIterator);
    scanState->workSpace->resultIterator = nullptr;
    delete scanState->workSpace;
    pfree(scanState);
    scan->opaque = NULL;
}

/*
 * Validate catalog entries for the specified operator class
 */
bool hnsw_validate(Oid opclassoid)
{
    return true;
}

void hnsw_costestimate(PlannerInfo *root,
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
IndexBulkDeleteResult *hnsw_vacuumcleanup(IndexVacuumInfo *info,
                                            IndexBulkDeleteResult *stats)
{
    Relation rel = info->index;

    if (stats == NULL)
        return NULL;

    stats->num_pages = 0;

    return stats;
}
