#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

rm /artifacts/vbase/query_run/table4/result/query_1_vbase.out >/dev/null 2>&1
#cp /tmp/vectordb/eval_src/hnswindex_ef_86.cpp /tmp/vectordb/src/hnswindex.cpp && cd /tmp/vectordb/build && make -j40  >/dev/null 2>&1 && make install >/dev/null 2>&1
#echo "Start to run vbase query 1 in Table-5 of paper"
psql -U vectordb -f /artifacts/vbase/query_run/table4/sql/query_1_vbase.sql > /artifacts/vbase/query_run/table4/result/query_1_vbase.out

