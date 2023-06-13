#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

echo "Run Queries in Table-4 by VBase"
docker exec --privileged --user=root vbase_evaluation bash -c "/artifacts/vbase/scripts/run_table4.sh"

echo "Save and Analaysis Results..."
echo "Table-4"
echo "--------------------------------------------------"
docker exec --privileged --user=root vbase_evaluation bash -c "/artifacts/vbase/scripts/analysis_table4.sh"
echo "--------------------------------------------------"
