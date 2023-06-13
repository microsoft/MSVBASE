#include "index.hpp"
#include "hnswindex.hpp"

extern "C"
{
#include <postgres.h>

#include <fmgr.h>
#include <omp.h>
#include <access/reloptions.h>
}

extern "C"
{
    PG_MODULE_MAGIC;
    PGDLLEXPORT void _PG_init(void);
}

relopt_kind sptag_para_relopt_kind;
relopt_kind hnsw_para_relopt_kind;
relopt_kind pase_hnsw_para_relopt_kind;

relopt_enum_elt_def sptag_DistCalcMethodValues[] =
{
	{"inner_product", sptag_Inner_Product},
	{"l2_distance", sptag_L2_Distance},
	{(const char *) NULL}		/* list terminator */
};

relopt_enum_elt_def hnsw_DistCalcMethodValues[] =
{
	{"inner_product", hnsw_Inner_Product},
	{"l2_distance", hnsw_L2_Distance},
	{(const char *) NULL}		/* list terminator */
};

void _PG_init(void)
{
    int totalCoreNum;
    totalCoreNum = omp_get_num_procs();

    sptag_para_relopt_kind = add_reloption_kind();
    add_int_reloption(sptag_para_relopt_kind, "threads", "Thread Number",
                      1, 1, totalCoreNum,AccessExclusiveLock);
    add_enum_reloption(sptag_para_relopt_kind, "distmethod", "Distance Calculate Method",
                        sptag_DistCalcMethodValues, sptag_Inner_Product,
                        "Valid values are \"inner_product\" and \"l2_distance\".",
                        AccessExclusiveLock);
    
    hnsw_para_relopt_kind = add_reloption_kind();
    add_int_reloption(hnsw_para_relopt_kind, "dimension", "Vector Dimension",
                      1, 1, 4096,AccessExclusiveLock);
    add_enum_reloption(hnsw_para_relopt_kind, "distmethod", "Distance Calculate Method",
                        hnsw_DistCalcMethodValues, hnsw_Inner_Product,
                        "Valid values are \"inner_product\" and \"l2_distance\".",
                        AccessExclusiveLock);

    pase_hnsw_para_relopt_kind = add_reloption_kind();
    add_int_reloption(pase_hnsw_para_relopt_kind, "dimension", "Vector Dimension",
                      1, 1, 4096,AccessExclusiveLock);
    add_enum_reloption(pase_hnsw_para_relopt_kind, "distmethod", "Distance Calculate Method",
                        hnsw_DistCalcMethodValues, hnsw_Inner_Product,
                        "Valid values are \"inner_product\" and \"l2_distance\".",
                        AccessExclusiveLock);

}