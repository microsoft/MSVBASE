#include "model_mng.hpp"

#include <cmath>
#include <string>
#include <dlfcn.h>
#include <vector>

#include "util.hpp"

extern "C"
{
#include <postgres.h>
#include <catalog/pg_type_d.h>
#include <utils/array.h>

#include <commands/vacuum.h>
#include <miscadmin.h>
#include <utils/elog.h>
#include <utils/rel.h>
#include <utils/selfuncs.h>
// #include <tcop/utility.h>
#include <executor/spi.h>
#include <utils/builtins.h>
}


extern "C"
{
    PGDLLEXPORT PG_FUNCTION_INFO_V1(inference);
    PGDLLEXPORT PG_FUNCTION_INFO_V1(inference2);
}

typedef std::vector<float> (*Inference_Function)(char* input);

// TODO:: libname to be initliazed via env variable
char libname[] = "/usr/lib/libmodelzoo.so";
void* libhandle = NULL;

// TODO:: this function should have been moved to PG_INIT
void model_handler_init(){
    // TODO:: dlclose somewhere
    libhandle = dlopen(libname, RTLD_NOW | RTLD_GLOBAL);
    if ( !libhandle ){
        ereport(ERROR, errcode_for_file_access(),
                errmsg("could not load library \"%s\": %s", libname, dlerror()));
    }
}

std::vector<float> do_inference(char* funcname, char* input){
    if ( !libhandle ) model_handler_init();
    if ( !libhandle ) return std::vector<float>();

    Inference_Function func = (Inference_Function)dlsym(libhandle, funcname);

    if (func) {
        return (*func)(input);
    }

    ereport(ERROR, errcode_for_file_access(),
            errmsg("could not load function \"%s\": %s", funcname, dlerror()));

    return std::vector<float>();
}


Datum inference2(PG_FUNCTION_ARGS)
{
    text *func_name = PG_GETARG_TEXT_PP(0);
    text *input_text = PG_GETARG_TEXT_PP(1);

    // auto hs = convert_array_to_vector(PG_GETARG_DATUM(1));
    
    // fprintf(stderr, "%s(%s) ---->[", text_to_cstring(func_name), text_to_cstring(input_text));
    // fprintf(stderr, "]\n");

    std::vector<float> result = do_inference(text_to_cstring(func_name), text_to_cstring(input_text));

    if ( result.size() == 0 ){
        ereport(ERROR, errcode_for_file_access(),
                errmsg("failed inference function \"%s\": no result returned", text_to_cstring(func_name)));
        PG_RETURN_NULL();
    }


    ArrayType* pgarray = convert_vector_to_array(result);
    PG_RETURN_ARRAYTYPE_P(pgarray);
}


Datum inference(PG_FUNCTION_ARGS)
{
    text *model_name = PG_GETARG_TEXT_PP(0);
    // TODO:: look into polymorphic functions/types
    text *input_text = PG_GETARG_TEXT_PP(1);
    // auto hs = convert_array_to_vector(PG_GETARG_DATUM(1));

    // BEGIN SECTION::
    // the following section rellys trys to run a simple SELECT FROM query
    // TODO:: MAYBE* memoize the query result (e.g. in a map) so that future queries is on a fast path
    // I said MAYBE* because pg already cache the query result so our memoization does to gain much
    char cmd[256];
    int cmd_len = snprintf(cmd, sizeof(cmd), "SELECT handler FROM model WHERE name = '%s';", text_to_cstring(model_name));

    if ( cmd_len < 0 )
        PG_RETURN_FLOAT8(-1.0);

    SPI_connect();
    int ret = SPI_exec(cmd, 1);

    uint64 proc = SPI_processed;

    char func_name[8192];
    func_name[0] = 0;

    if ( ret > 0 && SPI_tuptable != NULL ){
        TupleDesc tupdesc = SPI_tuptable->tupdesc;
        SPITupleTable *tuptable = SPI_tuptable;

        if ( proc > 1 )
            ereport(ERROR,
                    (errcode(ERRCODE_DATA_EXCEPTION), errmsg("only a single model handler should be returned")));
        
        if ( tupdesc->natts != 1 )
            ereport(ERROR,
                    (errcode(ERRCODE_DATA_EXCEPTION), errmsg("only a single attribute should be returned")));


        if ( proc == 1 ) {
            HeapTuple tuple = tuptable->vals[0];
            snprintf(func_name, sizeof(func_name), "%s", SPI_getvalue(tuple, tupdesc, 1));
        }
    }

    SPI_finish();

    if ( strlen(func_name) == 0 ){
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION), errmsg("did not find model name (%s) in model zoo", text_to_cstring(model_name))));
        PG_RETURN_NULL();
    }
    // END SECTION:: 

    
    std::vector<float> result = do_inference(func_name, text_to_cstring(input_text));

    if ( result.size() == 0 ){
        ereport(ERROR, errcode_for_file_access(),
                errmsg("failed inference function \"%s\": no result returned", func_name));
        PG_RETURN_NULL();
    }

    ArrayType* pgarray = convert_vector_to_array(result);
    PG_RETURN_ARRAYTYPE_P(pgarray);
}
