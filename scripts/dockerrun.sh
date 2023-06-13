#!/bin/bash
CWD=$(pwd)
docker run --name=vbase_artifacts --security-opt seccomp=seccomp-perf.json -v /dev/shm/artifacts:/u02  -v /datadrive/vbase_artifacts:/artifacts -e PGPASSWORD=vectordb -e PGUSERNAME=vectordb -e PGDATABASE=vectordb -v $CWD:/vectordb vbase_artifacts &
sleep 20
str=$"\n"
sstr=$(echo -e $str)
echo $sstr
