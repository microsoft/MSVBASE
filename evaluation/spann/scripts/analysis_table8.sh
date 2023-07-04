#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

echo "------------VBase SPANN Result--------------"
echo ""
echo "*Query 1"
python3 /artifacts/result_analysis/recall.py --path-gt /artifacts/postgres/query_run/table4/groundtruth/query_1_gt.out --path-query /artifacts/spann/query_run/table8/result/query_1_vbase_spann.out
python3 /artifacts/result_analysis/latency.py --path-result /artifacts/spann/query_run/table8/result/query_1_vbase_spann.out 
echo ""
echo "*Query 2"
python3 /artifacts/result_analysis/recall.py --path-gt /artifacts/postgres/query_run/table4/groundtruth/query_2_gt.out --path-query /artifacts/spann/query_run/table8/result/query_2_vbase_spann.out
python3 /artifacts/result_analysis/latency.py --path-result /artifacts/spann/query_run/table8/result/query_2_vbase_spann.out 
echo ""
echo "*Query 3"
python3 /artifacts/result_analysis/recall.py --path-gt /artifacts/postgres/query_run/table4/groundtruth/query_3_gt.out --path-query /artifacts/spann/query_run/table8/result/query_3_vbase_spann.out
python3 /artifacts/result_analysis/latency.py --path-result /artifacts/spann/query_run/table8/result/query_3_vbase_spann.out 
echo ""
echo "*Query 4"
python3 /artifacts/result_analysis/recall.py --path-gt /artifacts/postgres/query_run/table4/groundtruth/query_4_gt.out --path-query /artifacts/spann/query_run/table8/result/query_4_vbase_spann.out
python3 /artifacts/result_analysis/latency.py --path-result /artifacts/spann/query_run/table8/result/query_4_vbase_spann.out 
echo ""
echo "*Query 5"
python3 /artifacts/result_analysis/recall.py --path-gt /artifacts/postgres/query_run/table4/groundtruth/query_5_gt.out --path-query /artifacts/spann/query_run/table8/result/query_5_vbase_spann.out
python3 /artifacts/result_analysis/latency.py --path-result /artifacts/spann/query_run/table8/result/query_5_vbase_spann.out 
echo ""
echo "*Query 6"
python3 /artifacts/result_analysis/recall.py --path-gt /artifacts/postgres/query_run/table4/groundtruth/query_6_gt.out --path-query /artifacts/spann/query_run/table8/result/query_6_vbase_spann.out
python3 /artifacts/result_analysis/latency.py --path-result /artifacts/spann/query_run/table8/result/query_6_vbase_spann.out 
echo ""
echo "*Query 7"
python3 /artifacts/result_analysis/recall.py --path-gt /artifacts/postgres/query_run/table4/groundtruth/query_7_gt.out --path-query /artifacts/spann/query_run/table8/result/query_7_vbase_spann.out
python3 /artifacts/result_analysis/latency.py --path-result /artifacts/spann/query_run/table8/result/query_7_vbase_spann.out 
echo ""
echo "*Query 8"
python3 /artifacts/result_analysis/recall_q8.py  --path-query /artifacts/spann/query_run/table8/result/query_8_vbase_spann.out
python3 /artifacts/result_analysis/latency.py --path-result /artifacts/spann/query_run/table8/result/query_8_vbase_spann.out 
