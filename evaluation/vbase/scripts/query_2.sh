#!/bin/bash
rm /artifacts/vbase/query_run/table4/result/query_2_vbase.out >/dev/null 2>&1
#cp /tmp/vectordb/eval_src/hnswindex_ef_200.cpp /tmp/vectordb/src/hnswindex.cpp && cd /tmp/vectordb/build && make -j40  >/dev/null 2>&1 && make install >/dev/null 2>&1
#echo "Start to run vbase query 2 in Table-5 of paper"
psql -U vectordb -f /artifacts/vbase/query_run/table4/sql/query_2_vbase.sql > /artifacts/vbase/query_run/table4/result/query_2_vbase.out

