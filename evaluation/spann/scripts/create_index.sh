#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

mkdir -p /indexdata
/tmp/vectordb/thirdparty/SPTAG/Release/ssdserving /artifacts/spann/scripts/create_index_image.config
#cp meta into /indexdata/image_spann_index
chmod 755 /indexdata/image_spann_index/meta*
