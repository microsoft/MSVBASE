#!/bin/bash
#VERSION=$(git rev-parse --short HEAD)
#DOCKERNAME=vectordb-$VERSION
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

docker build --build-arg UID=$(id -u) --build-arg GID=$(id -g) -t vbase_open_source -f Dockerfile .
