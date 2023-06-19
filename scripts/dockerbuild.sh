#!/bin/bash
#VERSION=$(git rev-parse --short HEAD)
#DOCKERNAME=vectordb-$VERSION

docker build --build-arg UID=$(id -u) --build-arg GID=$(id -g) -t vbase_open_source -f Dockerfile .
