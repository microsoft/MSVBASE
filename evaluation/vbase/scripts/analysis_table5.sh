#!/bin/bash

echo "------------VBase Result--------------"
echo ""
echo "*Query 1, Selectivity=0.03"
python3 /artifacts/result_analysis/recall.py --path-gt /artifacts/postgres/query_run/table5/groundtruth/query_selectivity_0.03_gt.out --path-query /artifacts/vbase/query_run/table5/result/query_selectivity_0.03_vbase.out
python3 /artifacts/result_analysis/latency.py --path-result /artifacts/vbase/query_run/table5/result/query_selectivity_0.03_vbase.out 
echo ""
echo "*Query 2, Selectivity=0.3"
python3 /artifacts/result_analysis/recall.py --path-gt /artifacts/postgres/query_run/table5/groundtruth/query_selectivity_0.3_gt.out --path-query /artifacts/vbase/query_run/table5/result/query_selectivity_0.3_vbase.out
python3 /artifacts/result_analysis/latency.py --path-result /artifacts/vbase/query_run/table5/result/query_selectivity_0.3_vbase.out 
echo ""
echo "*Query 3, Selectivity=0.9"
python3 /artifacts/result_analysis/recall.py --path-gt /artifacts/postgres/query_run/table5/groundtruth/query_selectivity_0.9_gt.out --path-query /artifacts/vbase/query_run/table5/result/query_selectivity_0.9_vbase.out
python3 /artifacts/result_analysis/latency.py --path-result /artifacts/vbase/query_run/table5/result/query_selectivity_0.9_vbase.out 
