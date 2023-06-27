#!/bin/bash
rm /artifacts/vbase/query_run/table4/result/query_8_vbase.out >/dev/null 2>&1
#echo "Start to run vbase query 8 in Table-5 of paper"
psql -U vectordb -f /artifacts/vbase/query_run/table4/sql/query_8_vbase.sql > /artifacts/vbase/query_run/table4/result/query_8_vbase.out

