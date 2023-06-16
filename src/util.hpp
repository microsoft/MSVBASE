// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef UTIL_HPP
#define UTIL_HPP

#include <vector>
#include <string>

extern "C"
{
#include "postgres.h"
#include <catalog/pg_type_d.h>
#include <utils/array.h>
}

#define CPP_PG_FUNCTION_INFO_V1(NAME)          \
    extern "C"                                 \
    {                                          \
        PGDLLEXPORT PG_FUNCTION_INFO_V1(NAME); \
    }
std::string convert_text_data_to_string(Datum value);
std::string convert_data_to_string(Datum value);
std::vector<float> convert_array_to_vector(Datum value);
ArrayType* convert_vector_to_array(std::vector<float> d);
std::vector<std::string> convert_array_to_vector_str(Datum value);

enum DistanceMethod : uint8_t
{
    L2 = 0,

    InnerProduct
};

#endif
