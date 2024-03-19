#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

CURRENT_PATH=$(dirname "$0")
SPTAG_PATH="${CURRENT_PATH}/../thirdparty/SPTAG"
(cd $SPTAG_PATH && git apply ../../patch/spann.patch)
HNSW_PATH="${CURRENT_PATH}/../thirdparty/hnsw"
(cd $HNSW_PATH && git apply ../../patch/hnsw.patch)
POSTGRES_PATH="${CURRENT_PATH}/../thirdparty/Postgres"
(cd $POSTGRES_PATH && git apply ../../patch/Postgres.patch)
