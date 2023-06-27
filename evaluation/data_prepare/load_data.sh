#!/bin/bash
DIRPATH=$(dirname "$0")
docker exec --privileged --user=root vbase_evaluation bash -c "/artifacts/vbase/scripts/load_data.sh"
