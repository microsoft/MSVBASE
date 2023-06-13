// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef SPANNINDEX_HPP
#define SPANNINDEX_HPP

#include <inc/Core/Common/WorkSpace.h>
#include <inc/Core/SearchQuery.h>
#include <inc/Core/VectorSet.h>

#include "spannindex_scan.hpp"
extern "C"
{
#include <postgres.h>

#include <access/amapi.h>
#include <float.h>
}

extern "C"
{
    typedef enum spann_DistCalcMethod
    {
        spann_Inner_Product,
        spann_L2_Distance
    } spann_DistCalcMethod;

    typedef struct spann_ParaOptions
    {
        int32 vl_len_;
        int batch;
        spann_DistCalcMethod distmethod;
    } spann_ParaOptions;

    /*
     * SPANNScanOpaqueData: private state for a scan of a SPANN index
     */
    typedef struct SPANNScanOpaqueData
    {
        bool first;
        bool hasRangeFilter;
        bool inRange;
        float range;
        SPTAG::ByteArray *data;
        SPANNIndexScan::WorkSpace *workSpace;
    } SPANNScanOpaqueData;

    typedef SPANNScanOpaqueData *SPANNScanOpaque;


    IndexBuildResult *spann_build(Relation heap,
                                  Relation index,
                                  IndexInfo *indexInfo);
    /*
     * Prepare for an index scan
     */
    IndexScanDesc spann_begin_scan(Relation index, int nkeys, int norderbys);

    void spann_rescan(IndexScanDesc scan,
                      ScanKey keys,
                      int nkeys,
                      ScanKey orderbys,
                      int norderbys);

    /*
     * Fetch the next tuple in the given scan
     */
    bool spann_gettuple(IndexScanDesc scan, ScanDirection dir);

    /*
     * End a scan and release resources
     */
    void spann_endscan(IndexScanDesc scan);

    /*
     * Validate catalog entries for the specified operator class
     */
    bool spann_validate(Oid opclassoid);

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
    );

    IndexBulkDeleteResult *spann_vacuumcleanup(IndexVacuumInfo *info,
                                               IndexBulkDeleteResult *stats);
}
#endif
