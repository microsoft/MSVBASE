#include "operator.hpp"

#include <cmath>
#include <hnswlib.h>
#include "util.hpp"

extern "C"
{
#include <postgres.h>

#include <commands/vacuum.h>
#include <miscadmin.h>
#include <utils/elog.h>
#include <utils/rel.h>
#include <utils/selfuncs.h>
}

extern "C"
{
    PGDLLEXPORT PG_FUNCTION_INFO_V1(l2_distance);
    PGDLLEXPORT PG_FUNCTION_INFO_V1(range_l2_distance);
    PGDLLEXPORT PG_FUNCTION_INFO_V1(inner_product_distance);
    PGDLLEXPORT PG_FUNCTION_INFO_V1(range_inner_product_distance);
}

static hnswlib::InnerProductSpace IPDistance(0);
static hnswlib::L2Space L2Distance(0);

Datum l2_distance(PG_FUNCTION_ARGS)
{
    auto lhs = convert_array_to_vector(PG_GETARG_DATUM(0));
    auto rhs = convert_array_to_vector(PG_GETARG_DATUM(1));
    //double distance = 0.0;
    //double diff;

    if (lhs.size() != rhs.size())
    {
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("inconsistent array length, expected %ld, found %ld",
                        lhs.size(),
                        rhs.size())));
    }

    size_t dim = lhs.size();

    PG_RETURN_FLOAT8(L2Distance.get_dist_func()(lhs.data(), rhs.data(), &dim));

    /*
    for (size_t i = 0; i < lhs.size(); i++)
    {
        diff = lhs[i] - rhs[i];
        distance += diff * diff;
    }

    PG_RETURN_FLOAT8(std::sqrt(distance));
    */
}

Datum range_l2_distance(PG_FUNCTION_ARGS)
{
    std::vector<float> lhs = convert_array_to_vector(PG_GETARG_DATUM(0));
    std::vector<float> rhs = convert_array_to_vector(PG_GETARG_DATUM(1));
    
    float range = rhs[0];

    size_t dim = lhs.size();

    PG_RETURN_BOOL(
        L2Distance.get_dist_func()(lhs.data(), rhs.data() + 1, &dim) < range);
}

Datum inner_product_distance(PG_FUNCTION_ARGS)
{
    auto lhs = convert_array_to_vector(PG_GETARG_DATUM(0));
    auto rhs = convert_array_to_vector(PG_GETARG_DATUM(1));
    //double distance = 0.0;
    //double diff;

    if (lhs.size() != rhs.size())
    {
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("inconsistent array length, expected %ld, found %ld",
                        lhs.size(),
                        rhs.size())));
    }
    /*
    for (size_t i = 0; i < lhs.size(); i++)
    {
        diff = lhs[i] * rhs[i];
        distance += diff * diff;
    }

    PG_RETURN_FLOAT8(1 - distance);
    */
    size_t dim = lhs.size();

    PG_RETURN_FLOAT8(IPDistance.get_dist_func()(lhs.data(), rhs.data(), &dim));
}

Datum range_inner_product_distance(PG_FUNCTION_ARGS)
{
    std::vector<float> lhs = convert_array_to_vector(PG_GETARG_DATUM(0));
    std::vector<float> rhs = convert_array_to_vector(PG_GETARG_DATUM(1));

    float range = rhs[0];

    size_t dim = lhs.size();

    PG_RETURN_BOOL(
        IPDistance.get_dist_func()(lhs.data(), rhs.data() + 1, &dim) < range);
}

