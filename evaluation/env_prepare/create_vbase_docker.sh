#!/bin/bash
sudo mkdir /dev/shm/vbase_artifacts
sudo chmod 777 /dev/shm/vbase_artifacts
DIRPATH=$(dirname "$0")
EVALUATION=`cd $DIRPATH/../ && pwd`
(cd $DIRPATH/../../ && docker build --build-arg UID=$(id -u) --build-arg GID=$(id -g) -t vbase_evaluation -f Dockerfile .)
docker run --name=vbase_evaluation -e PGPASSWORD=vectordb -e PGUSERNAME=vectordb -e PGDATABASE=vectordb -v /dev/shm/vbase_artifacts:/u02 -v $EVALUATION:/artifacts vbase_evaluation &
sleep 20
str=$"\n"
sstr=$(echo -e $str)
echo $sstr
