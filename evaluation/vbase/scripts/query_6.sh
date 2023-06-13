#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

rm /artifacts/vbase/query_run/table4/result/query_6_vbase.out >/dev/null 2>&1
#echo "Start to run vbase query 6 in Table-5 of paper"
psql -U vectordb -f /artifacts/vbase/query_run/table4/sql/query_6_vbase.sql > /artifacts/vbase/query_run/table4/result/query_6_vbase.out

