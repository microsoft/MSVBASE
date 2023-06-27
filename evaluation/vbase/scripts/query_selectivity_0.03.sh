#!/bin/bash
rm /artifacts/vbase/query_run/table5/result/query_selectivity_0.03_vbase.out >/dev/null 2>&1
#cp /tmp/vectordb/eval_src/hnswindex_ef_200.cpp /tmp/vectordb/src/hnswindex.cpp && cd /tmp/vectordb/build && make -j40  >/dev/null 2>&1 && make install >/dev/null 2>&1
#echo "Start to run vbase query with selectivity=0.03 in Table-6 of paper"
psql -U vectordb -f /artifacts/vbase/query_run/table5/sql/query_selectivity_0.03_vbase.sql > /artifacts/vbase/query_run/table5/result/query_selectivity_0.03_vbase.out

