#ifndef INDEX_HPP
#define INDEX_HPP

#include <inc/Core/SearchQuery.h>
#include <inc/Core/VectorSet.h>
#include <inc/Core/Common/WorkSpace.h>
#include "index_scan.hpp"
extern "C"
{
#include <postgres.h>
#include <float.h>
#include <access/amapi.h>
}

extern "C"
{
    typedef enum sptag_DistCalcMethod
    {
	    sptag_Inner_Product,
	    sptag_L2_Distance
    }   sptag_DistCalcMethod;

    typedef struct sptag_ParaOptions {
        int32 vl_len_;
        int threads;
        sptag_DistCalcMethod distmethod;
    } sptag_ParaOptions;

    /*
     * SPTAGScanOpaqueData: private state for a scan of a SPTAG index
     */
    typedef struct SPTAGScanOpaqueData
    {
        bool first;
        bool hasRangeFilter;
        bool inRange;
        float range;
        SPTAG::ByteArray *data;
        IndexScan::WorkSpace *workSpace;
    } SPTAGScanOpaqueData;

    typedef SPTAGScanOpaqueData *SPTAGScanOpaque;

    /**
     * @brief Build index, part of the IndexAmRoutine API. This function will
     * use IndexBuilder in "index_builder.cpp" to perform actual operations.
     */
    IndexBuildResult *sptag_build(Relation heap,
                                  Relation index,
                                  IndexInfo *indexInfo);

    /*
     * Prepare for an index scan
     */
    IndexScanDesc sptag_begin_scan(Relation index, int nkeys, int norderbys);

    void sptag_rescan(IndexScanDesc scan,
                      ScanKey keys,
                      int nkeys,
                      ScanKey orderbys,
                      int norderbys);

    /*
     * Fetch the next tuple in the given scan
     */
    bool sptag_gettuple(IndexScanDesc scan, ScanDirection dir);

    /*
     * End a scan and release resources
     */
    void sptag_endscan(IndexScanDesc scan);

    /*
     * Validate catalog entries for the specified operator class
     */
    bool sptag_validate(Oid opclassoid);

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
    );

    IndexBulkDeleteResult *sptag_vacuumcleanup(IndexVacuumInfo *info,
                                               IndexBulkDeleteResult *stats);
}
#endif
