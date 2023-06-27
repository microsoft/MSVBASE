#/bin/bash
mkdir /artifacts/vbase/query_run/table4/sql
mkdir /artifacts/vbase/query_run/table4/result
mkdir /artifacts/vbase/query_run/table5/sql
mkdir /artifacts/vbase/query_run/table5/result
python3 /artifacts/vbase/query_run/table4/query_1.py
python3 /artifacts/vbase/query_run/table4/query_2.py
python3 /artifacts/vbase/query_run/table4/query_3.py
python3 /artifacts/vbase/query_run/table4/query_4.py
python3 /artifacts/vbase/query_run/table4/query_5.py
python3 /artifacts/vbase/query_run/table4/query_6.py
python3 /artifacts/vbase/query_run/table4/query_7.py
python3 /artifacts/vbase/query_run/table4/query_8.py
python3 /artifacts/vbase/query_run/table5/query_selectivity_0.03.py
python3 /artifacts/vbase/query_run/table5/query_selectivity_0.3.py
python3 /artifacts/vbase/query_run/table5/query_selectivity_0.9.py
