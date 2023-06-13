#include <iostream>
#include <string>

#include "pase_hnswindex.hpp"
#include "pase_hnswindex_builder.hpp"
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
#include <optimizer/cost.h>
}

extern relopt_kind pase_hnsw_para_relopt_kind;

bytea *pase_hnsw_para_options(Datum reloptions, bool validate) {

  static const relopt_parse_elt tab[] = {
      {"dimension", RELOPT_TYPE_INT, offsetof(PASE_hnsw_ParaOptions, dimension)},
      {"distmethod", RELOPT_TYPE_ENUM, offsetof(PASE_hnsw_ParaOptions, distmethod)}
  };

  return (bytea *)build_reloptions(reloptions, validate, pase_hnsw_para_relopt_kind, sizeof(PASE_hnsw_ParaOptions), tab, lengthof(tab));
}

int pase_hnsw_ParaGetDimension(Relation index) {
  PASE_hnsw_ParaOptions *opts = (PASE_hnsw_ParaOptions *)index->rd_options;

  if (opts)
    return opts->dimension;

  return 1;
}

PASE_hnsw_DistCalcMethod pase_hnsw_ParaGetDistmethod(Relation index) {
  PASE_hnsw_ParaOptions *opts = (PASE_hnsw_ParaOptions *)index->rd_options;

  if (opts)
    return opts->distmethod;

  return hnsw_Inner_Product;
}

CPP_PG_FUNCTION_INFO_V1(pase_hnsw_handler);
Datum pase_hnsw_handler(PG_FUNCTION_ARGS)
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
    amroutine->ambuild = pase_hnsw_build;
    amroutine->ambuildempty = nullptr;     // TODO: index access method
    amroutine->aminsert = nullptr;         // TODO: index access method
    amroutine->ambulkdelete = nullptr;     // TODO: index access method
    amroutine->amvacuumcleanup =
        pase_hnsw_vacuumcleanup;
    amroutine->amcanreturn = nullptr;
    amroutine->amcostestimate = pase_hnsw_costestimate;
    amroutine->amoptions = pase_hnsw_para_options;  // TODO: index access method
    amroutine->amproperty = nullptr; /* TODO AMPROP_DISTANCE_ORDERABLE */
#if PG_VERSION_NUM >= 120000
    amroutine->ambuildphasename = nullptr;
#endif
    amroutine->amvalidate = pase_hnsw_validate;
    amroutine->ambeginscan = pase_hnsw_begin_scan;
    amroutine->amrescan = pase_hnsw_rescan;
    amroutine->amgettuple = pase_hnsw_gettuple;
    amroutine->amgetbitmap = nullptr;
    amroutine->amendscan = pase_hnsw_endscan;
    amroutine->ammarkpos = nullptr;
    amroutine->amrestrpos = nullptr;
#if PG_VERSION_NUM >= 100000
    amroutine->amestimateparallelscan = nullptr;
    amroutine->aminitparallelscan = nullptr;
    amroutine->amparallelrescan = nullptr;
#endif

    PG_RETURN_POINTER(amroutine);
}

IndexBuildResult *pase_hnsw_build(Relation heap,
                              Relation index,
                              IndexInfo *indexInfo)
{
    IndexBuildResult *result;
    try
    {
        auto builder = PASE_HNSWIndexBuilder(heap, index, indexInfo);
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

IndexScanDesc pase_hnsw_begin_scan(Relation index, int nkeys, int norderbys)
{
    std::string path = std::string(DataDir) + std::string("/") +
                       std::string(DatabasePath) + std::string("/") +
                       std::string(RelationGetRelationName(index));
    //TODO(qianxi): how to get distance method and dimension
    switch(pase_hnsw_ParaGetDistmethod(index))
    {
        case hnsw_Inner_Product:
            PASE_HNSWIndexScan::LoadIndex(path, DistanceMethod::InnerProduct, pase_hnsw_ParaGetDimension(index));
            break;
        case hnsw_L2_Distance:
            PASE_HNSWIndexScan::LoadIndex(path, DistanceMethod::L2, pase_hnsw_ParaGetDimension(index));
            break;
        default:
            elog(ERROR, "pase hnsw index parameter value error.");
    }

    IndexScanDesc scan = RelationGetIndexScan(index, nkeys, norderbys);

    PASE_HNSWScanOpaque scanState =
        (PASE_HNSWScanOpaque) palloc(sizeof(PASE_HNSWScanOpaqueData));
    scanState->first = true;
    scanState->workSpace = nullptr;
    scan->opaque = scanState;
    return scan;
}

void pase_hnsw_rescan(IndexScanDesc scan,
                  ScanKey keys,
                  int nkeys,
                  ScanKey orderbys,
                  int norderbys)
{
    PASE_HNSWScanOpaque scanState = (PASE_HNSWScanOpaque) scan->opaque;
    
    if (scanState->workSpace != nullptr)
    {
        PASE_HNSWIndexScan::EndScan();
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
bool pase_hnsw_gettuple(IndexScanDesc scan, ScanDirection dir)
{
    PASE_HNSWScanOpaque scanState = (PASE_HNSWScanOpaque) scan->opaque;
    /*
     * Index can be used to scan backward, but Postgres doesn't support
     * backward scan on operators
     */
    Assert(ScanDirectionIsForward(dir));

    if (scanState->first)
    {
       scanState->workSpace = new PASE_HNSWIndexScan::WorkSpace();
       std::string path = std::string(DataDir) + std::string("/") +
                        std::string(DatabasePath) + std::string("/") +
                        std::string(RelationGetRelationName(scan->indexRelation));
         /* Safety check */
        if (scan->orderByData == NULL)
            elog(ERROR, "cannot scan sptag index without order");

        /* No items will match if null */
        if (scan->orderByData->sk_flags & SK_ISNULL)
            return false;

        Datum value = scan->orderByData->sk_argument;
	    scanState->workSpace->array = convert_array_to_vector(value);
        
        std::priority_queue<std::pair<float, std::uint64_t>> max_result = 
            PASE_HNSWIndexScan::BeginScan((char *)scanState->workSpace->array.data(),path, 188);
        
        while (max_result.size() > 0)
        {
            std::pair<float, std::uint64_t> item = max_result.top();
            max_result.pop();
            scanState->workSpace->result.push(std::pair<float, std::uint64_t>(1 - item.first, item.second));
        }
        
        scanState->first = false;
    }

    if (scanState->workSpace->result.size() > 0)
    {
        std::pair<float, std::uint64_t> item = scanState->workSpace->result.top();
        scanState->workSpace->result.pop();

        std::uint64_t number = item.second;
        BlockNumber blkno = (std::uint32_t) (number >> 32);
        OffsetNumber offset = (std::uint32_t) number;
    #if PG_VERSION_NUM >= 120000
        ItemPointerSet(&scan->xs_heaptid, blkno, offset);
    #else
        ItemPointerSet(&scan->xs_ctup.t_self, blkno, offset);
    #endif
        scan->xs_recheckorderby = false;
        return true;
    } else {
        return false;
    }
}

/*
 * End a scan and release resources
 */
void pase_hnsw_endscan(IndexScanDesc scan)
{
    PASE_HNSWScanOpaque scanState = (PASE_HNSWScanOpaque) scan->opaque;
    PASE_HNSWIndexScan::EndScan();
    delete scanState->workSpace;
    pfree(scanState);
    scan->opaque = NULL;
}

/*
 * Validate catalog entries for the specified operator class
 */
bool pase_hnsw_validate(Oid opclassoid)
{
    return true;
}

void pase_hnsw_costestimate(PlannerInfo *root,
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
    *indexSelectivity = costs.indexSelectivity;
    *indexCorrelation = costs.indexCorrelation;
#if PG_VERSION_NUM >= 100000
    *indexPages = costs.numIndexPages;
#endif
}
/*
 * Clean up after a VACUUM operation
 */
IndexBulkDeleteResult *pase_hnsw_vacuumcleanup(IndexVacuumInfo *info,
                                          IndexBulkDeleteResult *stats)
{
    Relation rel = info->index;

    if (stats == NULL)
        return NULL;

    stats->num_pages = 0;

    return stats;
}
