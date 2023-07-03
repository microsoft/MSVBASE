// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef HNSWINDEX_HPP
#define HNSWINDEX_HPP

#include "hnswindex_scan.hpp"

extern "C"
{
#include <postgres.h>

#include <access/amapi.h>
#include <float.h>
}

extern "C"
{
    typedef enum hnsw_DistCalcMethod
    {
	    hnsw_Inner_Product,
	    hnsw_L2_Distance
    }   hnsw_DistCalcMethod;

    typedef struct hnsw_ParaOptions {
        int32 vl_len_;
        int dimension;
        hnsw_DistCalcMethod distmethod;
    } hnsw_ParaOptions;

    typedef struct HNSWScanOpaqueData
    {
        bool first;
        bool hasRangeFilter;
        bool inRange;
        float range;
        HNSWIndexScan::WorkSpace *workSpace;
    } HNSWScanOpaqueData;

    typedef HNSWScanOpaqueData* HNSWScanOpaque;

    /**
     * @brief Build index, part of the IndexAmRoutine API. This function will
     * use IndexBuilder in "hnswindex_builder.cpp" to perform actual operations.
     */
    IndexBuildResult *hnsw_build(Relation heap,
                                  Relation index,
                                  IndexInfo *indexInfo);

    /*
     * Prepare for an index scan
     */
    IndexScanDesc hnsw_begin_scan(Relation index, int nkeys, int norderbys);

    void hnsw_rescan(IndexScanDesc scan,
                      ScanKey keys,
                      int nkeys,
                      ScanKey orderbys,
                      int norderbys);

    /*
     * Fetch the next tuple in the given scan
     */
    bool hnsw_gettuple(IndexScanDesc scan, ScanDirection dir);

    /*
     * End a scan and release resources
     */
    void hnsw_endscan(IndexScanDesc scan);

    /*
     * Validate catalog entries for the specified operator class
     */
    bool hnsw_validate(Oid opclassoid);

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
    );

    IndexBulkDeleteResult *hnsw_vacuumcleanup(IndexVacuumInfo *info,
                                              IndexBulkDeleteResult *stats);
}
#endif
