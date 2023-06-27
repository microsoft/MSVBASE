#!/bin/bash
rm /artifacts/vbase/query_run/table4/result/query_7_vbase.out >/dev/null 2>&1 
#echo "Start to run vbase query 7 in Table-5 of paper"
psql -U vectordb -f /artifacts/vbase/query_run/table4/sql/query_7_vbase.sql > /artifacts/vbase/query_run/table4/result/query_7_vbase.out

