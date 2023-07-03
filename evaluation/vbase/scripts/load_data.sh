#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

psql -U vectordb -f /artifacts/vbase/load_data/load.sql
