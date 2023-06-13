#ifndef PASE_HNSWINDEX_HPP
#define PASE_HNSWINDEX_HPP

#include "pase_hnswindex_scan.hpp"

extern "C"
{
#include <postgres.h>

#include <access/amapi.h>
#include <float.h>
}

extern "C"
{
    typedef enum PASE_hnsw_DistCalcMethod
    {
	    hnsw_Inner_Product,
	    hnsw_L2_Distance
    } PASE_hnsw_DistCalcMethod;

    typedef struct PASE_hnsw_ParaOptions {
        int32 vl_len_;
        int dimension;
        PASE_hnsw_DistCalcMethod distmethod;
    } PASE_hnsw_ParaOptions;

    typedef struct PASE_HNSWScanOpaqueData
    {
        bool first;
        PASE_HNSWIndexScan::WorkSpace *workSpace;
    } PASE_HNSWScanOpaqueData;

    typedef PASE_HNSWScanOpaqueData* PASE_HNSWScanOpaque;

    /**
     * @brief Build index, part of the IndexAmRoutine API. This function will
     * use IndexBuilder in "hnswindex_builder.cpp" to perform actual operations.
     */
    IndexBuildResult *pase_hnsw_build(Relation heap,
                                  Relation index,
                                  IndexInfo *indexInfo);

    /*
     * Prepare for an index scan
     */
    IndexScanDesc pase_hnsw_begin_scan(Relation index, int nkeys, int norderbys);

    void pase_hnsw_rescan(IndexScanDesc scan,
                      ScanKey keys,
                      int nkeys,
                      ScanKey orderbys,
                      int norderbys);

    /*
     * Fetch the next tuple in the given scan
     */
    bool pase_hnsw_gettuple(IndexScanDesc scan, ScanDirection dir);

    /*
     * End a scan and release resources
     */
    void pase_hnsw_endscan(IndexScanDesc scan);

    /*
     * Validate catalog entries for the specified operator class
     */
    bool pase_hnsw_validate(Oid opclassoid);

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
    );

    IndexBulkDeleteResult *pase_hnsw_vacuumcleanup(IndexVacuumInfo *info,
                                                   IndexBulkDeleteResult *stats);
}
#endif
