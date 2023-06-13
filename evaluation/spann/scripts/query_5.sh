#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

rm /artifacts/spann/query_run/table8/result/query_5_vbase_spann.out >/dev/null 2>&1
psql -U vectordb -f /artifacts/spann/query_run/table8/sql/query_5_vbase_spann.sql > /artifacts/spann/query_run/table8/result/query_5_vbase_spann.out

