#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

DIRPATH=$(dirname "$0")
docker exec --privileged --user=root vbase_evaluation bash -c "/artifacts/vbase/scripts/load_data.sh"
