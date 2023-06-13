#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

echo "Save and Analaysis Results..."
echo "Table-6"
echo "--------------------------------------------------"
docker exec --privileged --user=root vbase_evaluation bash -c "/artifacts/vbase/scripts/analysis_table6.sh"
echo "--------------------------------------------------"
