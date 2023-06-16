#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

wget https://aka.ms/downloadazcopy-v10-linux
tar -xvf downloadazcopy-v10-linux
rm -f /usr/bin/azcopy
cp ./azcopy_linux_amd64_*/azcopy /usr/bin/
chmod 755 /usr/bin/azcopy

# Clean the kitchen
rm -f downloadazcopy-v10-linux
rm -rf ./azcopy_linux_amd64_*/
azcopy copy --recursive "https://vbasebaseline.blob.core.windows.net/vbench/vbench" /artifacts/
mv /artifacts/vbench /artifacts/raw_data

