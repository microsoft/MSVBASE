#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

CWD=$(pwd)
docker run --name=vbase_open_source -e PGPASSWORD=vectordb -e PGUSERNAME=vectordb -e PGDATABASE=vectordb -v $CWD:/vectordb vbase_open_source &
sleep 20
str=$"\n"
sstr=$(echo -e $str)
echo $sstr
