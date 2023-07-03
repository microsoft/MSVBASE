-- Copyright (c) Microsoft Corporation. All rights reserved.
-- Licensed under the MIT License.

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION vectordb" to load this file. \quit

-- type

-- Guide on How to Create Custom Types, See "Base Types"
-- https://www.postgresql.org/docs/13/sql-createtype.html
-- TODO: decide on whether supply custom vector type, or use existing 'array'

-- operators

CREATE TABLE IF NOT EXISTS model (
    name        text     PRIMARY KEY,
    handler     text,
    version     integer
);

CREATE FUNCTION model_handler(text) RETURNS text AS $$
    SELECT handler FROM model WHERE name = $1 LIMIT 1;
    $$ LANGUAGE SQL;

CREATE FUNCTION inference2(text, text) RETURNS float8[]
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION inference(text, text) RETURNS float8[]
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;


CREATE FUNCTION l2_distance(float8[], float8[]) RETURNS float8
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION range_l2_distance(float8[], float8[]) RETURNS bool
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR <-> (
	LEFTARG = float8[], RIGHTARG = float8[], PROCEDURE = l2_distance,
	COMMUTATOR = '<->'
);

CREATE OPERATOR <<->> (
	LEFTARG = float8[], RIGHTARG = float8[], PROCEDURE = range_l2_distance,
	COMMUTATOR = '<<->>'
);

CREATE FUNCTION inner_product_distance(float8[], float8[]) RETURNS float8
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION range_inner_product_distance(float8[], float8[]) RETURNS bool
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;


CREATE OPERATOR <*> (
	LEFTARG = float8[], RIGHTARG = float8[], PROCEDURE = inner_product_distance,
	COMMUTATOR = '<*>'
);

CREATE OPERATOR <<*>> (
	LEFTARG = float8[], RIGHTARG = float8[], PROCEDURE = range_inner_product_distance,
	COMMUTATOR = '<<*>>'
);

-- index handlers

CREATE FUNCTION sptag_handler(internal) RETURNS index_am_handler
	AS 'MODULE_PATHNAME' LANGUAGE C;

CREATE ACCESS METHOD sptag TYPE INDEX HANDLER sptag_handler;

COMMENT ON ACCESS METHOD sptag IS 'sptag index access method';

 -- hnsw index handlers
 CREATE FUNCTION hnsw_handler(internal) RETURNS index_am_handler
	AS 'MODULE_PATHNAME' LANGUAGE C;

CREATE ACCESS METHOD hnsw TYPE INDEX HANDLER hnsw_handler;

COMMENT ON ACCESS METHOD hnsw IS 'hnsw index access method';

 -- pase hnsw index handlers
 CREATE FUNCTION pase_hnsw_handler(internal) RETURNS index_am_handler
	AS 'MODULE_PATHNAME' LANGUAGE C;

CREATE ACCESS METHOD pase_hnsw TYPE INDEX HANDLER pase_hnsw_handler;

COMMENT ON ACCESS METHOD pase_hnsw IS 'pase hnsw index access method';


-- spann index handlers

CREATE FUNCTION spann_handler(internal) RETURNS index_am_handler
	AS 'MODULE_PATHNAME' LANGUAGE C;

CREATE ACCESS METHOD spann TYPE INDEX HANDLER spann_handler;

COMMENT ON ACCESS METHOD spann IS 'sptag index access method';

-- opclass
-- Operator Classes are used to provide the information required by the index, which is defined and consumed by the index access methods.
-- For more details, see https://www.postgresql.org/docs/13/xindex.html.

CREATE OPERATOR CLASS vector_l2_ops
	DEFAULT FOR TYPE float8[] USING sptag AS
	OPERATOR 1 <-> (float8[], float8[]) FOR ORDER BY float_ops,
	OPERATOR 2 <<->> (float8[], float8[]);

CREATE OPERATOR CLASS vector_inner_product_ops
	FOR TYPE float8[] USING sptag AS
	OPERATOR 1 <*> (float8[], float8[]) FOR ORDER BY float_ops,
	OPERATOR 2 <<*>> (float8[], float8[]);

CREATE OPERATOR CLASS hnsw_vector_l2_ops
	DEFAULT FOR TYPE float8[] USING hnsw AS
	OPERATOR 1 <-> (float8[], float8[]) FOR ORDER BY float_ops,
	OPERATOR 2 <<->> (float8[], float8[]);

CREATE OPERATOR CLASS hnsw_vector_inner_product_ops
	FOR TYPE float8[] USING hnsw AS
	OPERATOR 1 <*> (float8[], float8[]) FOR ORDER BY float_ops,
	OPERATOR 2 <<*>> (float8[], float8[]);

CREATE OPERATOR CLASS pase_hnsw_vector_l2_ops
	DEFAULT FOR TYPE float8[] USING pase_hnsw AS
	OPERATOR 1 <-> (float8[], float8[]) FOR ORDER BY float_ops;

CREATE OPERATOR CLASS pase_hnsw_vector_inner_product_ops
	FOR TYPE float8[] USING pase_hnsw AS
	OPERATOR 1 <*> (float8[], float8[]) FOR ORDER BY float_ops;

CREATE OPERATOR CLASS spann_vector_l2_ops
	DEFAULT FOR TYPE float8[] USING spann AS
	OPERATOR 1 <-> (float8[], float8[]) FOR ORDER BY float_ops,
	OPERATOR 2 <<->> (float8[], float8[]);

CREATE OPERATOR CLASS spann_vector_inner_product_ops
	FOR TYPE float8[] USING spann AS
	OPERATOR 1 <*> (float8[], float8[]) FOR ORDER BY float_ops,
	OPERATOR 2 <<*>> (float8[], float8[]);
-- topk hack
-- rank_expression left as '' will default to summation of all orderby expression
-- term_cond is the number of consecutive drops of table row after a priority queue is stable
CREATE FUNCTION topk(table_name text, k integer, ef integer, term_cond integer, attr_exp text, filter_exp text, rank_exp text, VARIADIC arr text[]) RETURNS SETOF record
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION multicol_topk(table_name text, k integer, term_cond integer, attr_exp text, filter_exp text, rank_exp text, VARIADIC arr text[]) RETURNS SETOF record
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
