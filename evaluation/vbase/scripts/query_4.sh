#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

rm /artifacts/vbase/query_run/table4/result/query_4_vbase.out >/dev/null 2>&1
#echo "Start to run VBase query 4 in Table-5 of paper" 
psql -U vectordb -f /artifacts/vbase/query_run/table4/sql/query_4_vbase.sql > /artifacts/vbase/query_run/table4/result/query_4_vbase.out

