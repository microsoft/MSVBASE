FROM debian:bullseye-slim

# make the "en_US.UTF-8" locale so postgres will be utf-8 enabled by default
ENV LANG en_US.utf8
ENV PG_MAJOR 13
ENV PG_VERSION 13.4
ENV PGDATA /u02/pgdata
ENV PGDATABASE "" \
    PGUSERNAME "" \
    PGPASSWORD ""

ARG UID=999
ARG GID=999

ENV PG_INSTALL_DIR /u01/app/postgres/product/${PG_VERSION}

RUN set -ex \
    \
    && apt-get update && apt-get install -y \
    gettext \
    ca-certificates \
    build-essential \
    curl \
    procps \
    sysstat \
    libldap2-dev \
    python-dev \
    libreadline-dev \
    libssl-dev \
    bison \
    flex \
    libghc-zlib-dev \
    libcrypto++-dev \
    libxml2-dev \
    libxslt1-dev \
    bzip2 \
    make \
    gcc \
    unzip \
    python \
    locales \
    wget \
    \
    && rm -rf /var/lib/apt/lists/* \
    && localedef -i en_US -c -f UTF-8 en_US.UTF-8

RUN apt-get update && \
	apt-get install -y --no-install-recommends wget git golang-go python-dev swig vim\
    libboost-filesystem-dev libboost-test-dev libboost-serialization-dev libboost-regex-dev libboost-serialization-dev libboost-regex-dev libboost-thread-dev libboost-system-dev

RUN wget "https://boostorg.jfrog.io/artifactory/main/release/1.71.0/source/boost_1_71_0.tar.gz" --no-check-certificate -q -O - \
        | tar -xz && \
        cd boost_1_71_0 && \
        ./bootstrap.sh && \
        ./b2 install && \
        ldconfig && \
        cd .. && rm -rf boost_1_71_0

RUN wget "https://github.com/Kitware/CMake/releases/download/v3.14.4/cmake-3.14.4-Linux-x86_64.tar.gz" --no-check-certificate -q -O - \
	| tar -xz --strip-components=1 -C /usr/local

RUN apt-get install -y software-properties-common && add-apt-repository 'deb http://archive.debian.org/debian stretch stretch-security main contrib non-free' && apt-get update && apt-get install -y openjdk-8-jdk

RUN apt install -y pip && pip install numpy && pip install pandas

RUN git config --global http.sslverify false

RUN wget http://ftp.gnu.org/gnu/gcc/gcc-12.2.0/gcc-12.2.0.tar.gz \
    && tar -zxvf gcc-12.2.0.tar.gz \
    && cd gcc-12.2.0 \
    && ./contrib/download_prerequisites \
    && mkdir build \
    && cd build/ \
    && ../configure -enable-checking=release -enable-languages=c,c++ -disable-multilib \
    && make -j4 \
    && make install \
    && rm /usr/bin/gcc \
    && ln -s /usr/local/bin/gcc /usr/bin/gcc \
    && rm /usr/bin/g++ \
    && ln -s /usr/local/bin/g++ /usr/bin/g++ \
    && rm /usr/lib/x86_64-linux-gnu/libstdc++.so.6 \
    && ln -s /usr/local/lib64/libstdc++.so.6.0.30 /usr/lib/x86_64-linux-gnu/libstdc++.so.6

RUN mkdir /u01/ \
    \
    && groupadd -r postgres --gid=$GID \
    && useradd -m -r -g postgres --uid=$UID postgres \
    && chown postgres:postgres /u01/ \
    && mkdir -p "$PGDATA" \
    && chown -R postgres:postgres "$PGDATA" \
    && chmod 700 "$PGDATA" 

COPY ./thirdparty/Postgres /home/postgres/src/
RUN echo "this line could be executed for a very long time" && chown -R postgres:postgres /home/postgres/src
    
RUN cd /home/postgres/src \
    && su postgres -c "./configure \
    --with-blocksize=32 \
    --enable-integer-datetimes \
    --enable-thread-safety \
    --with-pgport=5432 \
    --prefix=$PG_INSTALL_DIR \
    --with-ldap \
    --with-python \
    --with-openssl \
    --with-libxml \
    --with-libxslt \
    --enable-nls=yes" \
#    --enable-debug \
#    --enable-cassert \
#    CFLAGS='-ggdb -O0'" \
    && su postgres -c "make -j$(nproc) all" \
    && su postgres -c "make install" \
    && su postgres -c "make -C contrib install" \
    && apt-get install -y libxml2

ENV PGDATA ${PGDATA}/${PG_MAJOR}
COPY ./scripts/pg_scripts/*.sh /usr/local/bin/

ENV PATH="${PATH}:${PG_INSTALL_DIR}/bin"
ENV LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${PG_INSTALL_DIR}/lib"

COPY . /tmp/vectordb

RUN chown -R postgres:postgres /tmp/vectordb

ENV PostgreSQL_ROOT ${PG_INSTALL_DIR}


RUN cd /tmp/vectordb && \
    mkdir build && \
	cd build && \
    cmake -DCMAKE_INSTALL_PREFIX=/usr/local/vectordb -DLIBRARYONLY=ON -DSEEK_ENABLE_TESTS=ON -DCMAKE_BUILD_TYPE=Release .. && \
	make -j$(nproc) && \
	make install

# the followings two commands install an http-client library called curlpp
RUN apt-get update && apt-get install -y libcurl4-openssl-dev pkg-config
RUN git clone https://github.com/jpbarrette/curlpp.git && cd curlpp && \
    git reset --hard  592552a && \
    mkdir build && cd build && \
    cmake .. && make -j$(nproc) && \
    make install

ENV PATH="${PATH}:/usr/local/lib"
ENV LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:/usr/local/lib"

ENV LANG en_US.utf8
USER postgres
EXPOSE 5432
ENTRYPOINT ["docker-entrypoint.sh"]

