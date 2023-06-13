#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

PG_VERSION="13.4"
PG_INSTALL_DIR="/u01/app/postgres/product/$PG_VERSION"

cd ../Postgres
mkdir -p build
cd build
../configure                    \
--enable-integer-datetimes      \
--enable-thread-safety          \
--with-pgport=5432              \
       --prefix=$PG_INSTALL_DIR \
       --with-ldap              \
       --with-python            \
       --with-openssl           \
       --with-libxml \
       --with-libxslt \
       --enable-debug \
       --enable-cassert \
       CFLAGS='-ggdb -O0'
