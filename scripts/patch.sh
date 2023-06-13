#!/bin/bash
CURRENT_PATH=$(dirname "$0")
SPTAG_PATH="${CURRENT_PATH}/../thirdparty/SPTAG"
(cd $SPTAG_PATH && git apply ../../patch/SPTAG.patch)
HNSW_PATH="${CURRENT_PATH}/../thirdparty/hnsw"
(cd $HNSW_PATH && git apply ../../patch/hnsw.patch)
POSTGRES_PATH="${CURRENT_PATH}/../thirdparty/Postgres"
(cd $POSTGRES_PATH && git apply ../../patch/Postgres.patch)
