#!/bin/bash
CWD=$(pwd)
docker run --name=vbase_open_source -e PGPASSWORD=vectordb -e PGUSERNAME=vectordb -e PGDATABASE=vectordb -v $CWD:/vectordb vbase_open_source &
sleep 20
str=$"\n"
sstr=$(echo -e $str)
echo $sstr
