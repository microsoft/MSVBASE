#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

echo "------------VBase Result--------------"
echo ""
python3 /artifacts/result_analysis/recall.py --path-gt /artifacts/postgres/query_run/table4/groundtruth/query_7_gt.out --path-query /artifacts/vbase/query_run/table4/result/query_7_vbase.out
python3 /artifacts/result_analysis/latency.py --path-result /artifacts/vbase/query_run/table4/result/query_7_vbase.out 
