#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

echo "Run Queries in Table-5 by VBase"
docker exec --privileged --user=root vbase_evaluation bash -c "/artifacts/vbase/scripts/run_table5.sh"


echo "Save and Analaysis Results..."
echo "Table-5"
echo "--------------------------------------------------"
docker exec --privileged --user=root vbase_evaluation bash -c "/artifacts/vbase/scripts/analysis_table5.sh"
echo "--------------------------------------------------"
