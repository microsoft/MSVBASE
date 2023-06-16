// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "util.hpp"

extern "C"
{
#include <catalog/pg_type_d.h>
#include <utils/array.h>
#include <utils/fmgrprotos.h>
#include <utils/lsyscache.h>
#include <utils/numeric.h>
#include <utils/builtins.h>
}

std::string convert_text_data_to_string(Datum value)
{
	    std::string ret = DatumGetCString(DirectFunctionCall1(textout, value));
	        return ret;
}

std::string convert_data_to_string(Datum value)
{
    std::string ret = DatumGetCString(value);
    return ret;
}

std::vector<float> convert_array_to_vector(Datum value)
{
    ArrayType *value_array = DatumGetArrayTypeP(value);
    int dim = ArrayGetNItems(ARR_NDIM(value_array), ARR_DIMS(value_array));
    float8* data = (float8 *) ARR_DATA_PTR(value_array);
    std::vector<float> result(data, data + dim);
    return result;
}


ArrayType* convert_vector_to_array(std::vector<float> d){
    size_t sz = d.size();
    Datum    * vals = (Datum*) palloc(sizeof(Datum) * sz);
    // ArrayType* result = NULL;

    for ( size_t i = 0; i < d.size(); i++){
        vals[i] = Float8GetDatum(d[i]);
    }

    return construct_array(vals,
                           sz,FLOAT8OID,
                           sizeof(float8),true,'d');
}

std::vector<std::string> convert_array_to_vector_str(Datum value)
{
    // retrieve array and perform some checks
    ArrayType *value_array = DatumGetArrayTypeP(value);
    if (ARR_NDIM(value_array) > 1)
    {
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION), errmsg("array must be 1-D")));
    }
    int16 typlen;
    bool typbyval;
    char typalign;
    Datum *elemsp;
    bool *nullsp;
    int nelemsp;
    get_typlenbyvalalign(
        ARR_ELEMTYPE(value_array), &typlen, &typbyval, &typalign);
    deconstruct_array(value_array,
                      ARR_ELEMTYPE(value_array),
                      typlen,
                      typbyval,
                      typalign,
                      &elemsp,
                      &nullsp,
                      &nelemsp);

    std::vector<std::string> ret(nelemsp);

    // iterate over each cell of the vector
    for (int i = 0; i < nelemsp; i++)
    {
        if (nullsp[i])
            ereport(ERROR,
                    (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                     errmsg("array must not containing NULLs")));

        if (ARR_ELEMTYPE(value_array) == TEXTOID)
        {
            ret[i] = std::string(text_to_cstring(DatumGetTextPP(elemsp[i])));
        }
        else
        {
            ereport(ERROR,
                    (errcode(ERRCODE_DATA_EXCEPTION),
                     errmsg("unsupported array type")));
        }
    }

    return ret;
}
