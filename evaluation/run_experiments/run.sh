#!/bin/bash
DIRPATH=$(dirname "$0")
PATH_DIR="${DIRPATH}"
docker exec --privileged --user=root vbase_evaluation bash -c "apt install -y  python3-numpy python3-pandas"
echo `date "+DATE: %Y-%m-%d %H:%M:%S"`
echo "++++++++++++++++++++++++++++++++++++++++"
${PATH_DIR}/run_table_4.sh
echo `date "+DATE: %Y-%m-%d %H:%M:%S"`
echo ""
echo "++++++++++++++++++++++++++++++++++++++++"
${PATH_DIR}/run_table_5.sh
echo `date "+DATE: %Y-%m-%d %H:%M:%S"`
echo ""
echo "++++++++++++++++++++++++++++++++++++++++"
${PATH_DIR}/run_table_6.sh
echo `date "+DATE: %Y-%m-%d %H:%M:%S"`
