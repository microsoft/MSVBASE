#!/bin/bash

docker exec --privileged --user=root vbase_evaluation bash -c "/artifacts/data_prepare/cook_data.sh"
docker exec --privileged --user=root vbase_evaluation bash -c "/artifacts/vbase/scripts/query_generate.sh"
