#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# this are the environment variables which need to be set
PGDATA=${PGDATA}
PGHOME="/u01/app/postgres/product/${PG_VERSION}"
PGAUTOCONF=${PGDATA}/postgresql.auto.conf
PGHBACONF=${PGDATA}/pg_hba.conf
PGDATABASENAME=${PGDATABASE}
PGUSERNAME=${PGUSERNAME}
PGPASSWD=${PGPASSWORD}

# create the database and the user
_pg_create_database_and_user()
{
    if [ -z ${PGUSERNAME} ] || [ -z ${PGDATABASE} ] || [ -z ${PGPASSWORD} ]; then
        cat >&2 <<-'EOE'

			ERROR: NEED TO SET PGUSERNAME PGPASSWORD PGDATABASE 

		EOE
        exit 1
	fi

    # ${PGHOME}/bin/psql -c "create user ${PGUSERNAME} with login password '${PGPASSWD}'" postgres
    ${PGHOME}/bin/psql -c "create role ${PGUSERNAME} with login SUPERUSER password '${PGPASSWD}'" postgres
    ${PGHOME}/bin/psql -c "create database ${PGDATABASENAME} with owner = ${PGUSERNAME}" postgres
}

# start the PostgreSQL instance
_pg_prestart()
{
    ${PGHOME}/bin/pg_ctl -D ${PGDATA} -w start
}

# start postgres and do not disconnect
# required for docker
_pg_start()
{
    ${PGHOME}/bin/postgres "-D" "${PGDATA}"
}

# stop the PostgreSQL instance
_pg_stop()
{
    ${PGHOME}/bin/pg_ctl -D ${PGDATA} stop -m fast
}

# initdb a new cluster
_pg_initdb()
{
    ${PGHOME}/bin/initdb -D ${PGDATA}
}



# adjust the postgresql parameters
_pg_adjust_config() {
    # PostgreSQL parameters
    echo "shared_buffers='128MB'" >> ${PGAUTOCONF}
    echo "effective_cache_size='128MB'" >> ${PGAUTOCONF}
    echo "listen_addresses = '*'" >> ${PGAUTOCONF}
    echo "logging_collector = 'on'" >> ${PGAUTOCONF}
    echo "log_truncate_on_rotation = 'on'" >> ${PGAUTOCONF}
    echo "log_filename = 'postgresql-%a.log'" >> ${PGAUTOCONF}
    echo "log_rotation_age = '1440'" >> ${PGAUTOCONF}
    echo "log_line_prefix = '%m - %l - %p - %h - %u@%d '" >> ${PGAUTOCONF}
    echo "log_directory = 'pg_log'" >> ${PGAUTOCONF}
    echo "log_min_messages = 'WARNING'" >> ${PGAUTOCONF}
    echo "log_autovacuum_min_duration = '60s'" >> ${PGAUTOCONF}
    echo "log_min_error_statement = 'NOTICE'" >> ${PGAUTOCONF}
    echo "log_min_duration_statement = '30s'" >> ${PGAUTOCONF}
    echo "log_checkpoints = 'on'" >> ${PGAUTOCONF}
    echo "log_statement = 'none'" >> ${PGAUTOCONF}
    echo "log_lock_waits = 'on'" >> ${PGAUTOCONF}
    echo "log_temp_files = '0'" >> ${PGAUTOCONF}
    echo "log_timezone = 'Europe/Zurich'" >> ${PGAUTOCONF}
    echo "log_connections=on" >> ${PGAUTOCONF}
    echo "log_disconnections=on" >> ${PGAUTOCONF}
    echo "log_duration=off" >> ${PGAUTOCONF}
    echo "client_min_messages = 'WARNING'" >> ${PGAUTOCONF}
    echo "wal_level = 'replica'" >> ${PGAUTOCONF}
    echo "hot_standby_feedback = 'on'" >> ${PGAUTOCONF}
    echo "max_wal_senders = '10'" >> ${PGAUTOCONF}
    echo "cluster_name = '${PGDATABASENAME}'" >> ${PGAUTOCONF}
    echo "max_replication_slots = '10'" >> ${PGAUTOCONF}
    echo "work_mem=64MB" >> ${PGAUTOCONF}
    echo "maintenance_work_mem=64MB" >> ${PGAUTOCONF}
    echo "wal_compression=on" >> ${PGAUTOCONF}
    echo "max_wal_senders=20" >> ${PGAUTOCONF}
    echo "shared_preload_libraries='pg_stat_statements'" >> ${PGAUTOCONF}
    echo "autovacuum_max_workers=6" >> ${PGAUTOCONF}
    echo "autovacuum_vacuum_scale_factor=0.1" >> ${PGAUTOCONF}
    echo "autovacuum_vacuum_threshold=50" >> ${PGAUTOCONF}
    # Authentication settings in pg_hba.conf
    echo "host    all             all             0.0.0.0/0            md5" >> ${PGHBACONF}
}

# initialize and start a new cluster
_pg_init_and_start()
{
    # initialize a new cluster
    _pg_initdb
    # set params and access permissions
    _pg_adjust_config
    # start the new cluster
    _pg_prestart
    # set username and password
    _pg_create_database_and_user
}

# check if $PGDATA exists
if [ -e ${PGDATA} ]; then
    # when $PGDATA exists we need to check if there are files
    # because when there are files we do not want to initdb
    if [ -e "${PGDATA}/base" ]; then
        # when there is the base directory this
        # probably is a valid PostgreSQL cluster
        # so we just start it
        _pg_prestart
    else
        # when there is no base directory then we
        # should be able to initialize a new cluster
        # and then start it
        _pg_init_and_start
    fi
else
    # initialze and start the new cluster
    _pg_init_and_start
    # create PGDATA
    mkdir -p ${PGDATA}
    # create the log directory
    mkdir -p ${PGDATA}/pg_log
fi
# restart and do not disconnect from the postgres daemon
_pg_stop
_pg_start
