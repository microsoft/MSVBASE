cmake_minimum_required(VERSION 3.14.0)
project(vectordb VERSION 0.1.0)

# Find Dependency

set(PostgreSQL_ADDITIONAL_VERSIONS "13")
find_package(PostgreSQL REQUIRED COMPONENTS Server)

# Add Control and SQL Files

set(EXT_CONTROL_FILE ${PROJECT_NAME}.control)
set(EXT_SQL_FILES
    sql/${PROJECT_NAME}.sql
    # for each version upgrade, add like
    # sql/${PROJECT_NAME}--0.1.0--0.1.1.sql
)

list (GET EXT_SQL_FILES 0 EXT_SQL_MAIN)
configure_file(${EXT_CONTROL_FILE}.in ${EXT_CONTROL_FILE})

# Add Dependency

#Include(FetchContent)

#set(LIBRARYONLY ON)
#FetchContent_Declare(
#    sptag
#    GIT_REPOSITORY https://github.com/microsoft/SPTAG.git
#    GIT_TAG        master
#)
#FetchContent_GetProperties(sptag)
#if(NOT sptag_POPULATED)
#    FetchContent_Populate(sptag)
#    add_subdirectory(${sptag_SOURCE_DIR} ${sptag_BINARY_DIR} EXCLUDE_FROM_ALL)
#endif()

#find_package(OpenMP REQUIRED)

if (WIN32)
    add_definitions(-D_WIN32_WINNT=0x601)
    add_definitions(-DBOOST_THREAD_PROVIDES_NESTED_LOCKS)
else(UNIX)
    add_definitions(-DPLATFORM_UNIX)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2 -maes -mavx2 -fPIC")
    set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -mmwaitx ${CMAKE_EXE_LINKER_FLAGS} -fPIC")

    message (STATUS "UNIX: CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
    message (STATUS "UNIX: CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")

#Include(FetchContent)

#set(LIBRARYONLY ON)
#FetchContent_Declare(
#    sptag
#    #    GIT_REPOSITORY https://github.com/microsoft/SPTAG.git
#    #    GIT_TAG        master


#    GIT_REPOSITORY https://qiazh.visualstudio.com/SPTAG/_git/SPTAG
#    GIT_TAG        qiazh/debug
#)
#FetchContent_GetProperties(sptag)
#if(NOT sptag_POPULATED)
#    FetchContent_Populate(sptag)
#    add_subdirectory(${sptag_SOURCE_DIR} ${sptag_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# Add Extension Library

add_library(${PROJECT_NAME} SHARED
    src/lib.cpp
    src/index.cpp
    src/index_builder.cpp
    src/index_scan.cpp
    src/hnswindex.cpp
    src/hnswindex_builder.cpp
    src/hnswindex_scan.cpp
    src/pase_hnswindex.cpp
    src/pase_hnswindex_builder.cpp
    src/pase_hnswindex_scan.cpp
    src/operator.cpp
    src/util.cpp
    src/model_mng.cpp
    src/topk.cpp
    src/multicol_topk.cpp
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_14)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -mavx2)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

include(thirdparty/CMakeLists.txt)

thirdparty_dependent(${PROJECT_NAME})

if(WIN32)
    target_include_directories(${PROJECT_NAME} PUBLIC ${PostgreSQL_TYPE_INCLUDE_DIR}/port/win32)
    target_link_libraries(${PROJECT_NAME} PUBLIC ${PostgreSQL_LIBRARY_DIRS}/postgres.lib)
    if(MSVC)
        target_include_directories(${PROJECT_NAME} PUBLIC ${PostgreSQL_TYPE_INCLUDE_DIR}/port/win32_msvc)
        target_compile_options(${PROJECT_NAME} PRIVATE /TC)
    endif(MSVC)
endif(WIN32)

# Installation Configuration

## Find PostgreSQL Paths

#foreach(suffix ${PostgreSQL_KNOWN_VERSIONS})
#  if(WIN32)
#    list(APPEND PostgreSQL_BINARY_ADDITIONAL_SEARCH_SUFFIXES
#        "PostgreSQL/${suffix}/bin")
#  endif()
#  if(UNIX)
#    list(APPEND PostgreSQL_BINARY_ADDITIONAL_SEARCH_SUFFIXES
#        "postgresql${suffix}"
#        "postgresql/${suffix}"
#        "pgsql-${suffix}/bin")
#  endif()
#endforeach()
#find_program(PostgreSQL_PG_CONFIG pg_config
#NAMES ${ARGN}
#PATHS
#  ${PostgreSQL_ROOT_DIRECTORIES}
#PATH_SUFFIXES
#  bin
#  ${PostgreSQL_BINARY_ADDITIONAL_SEARCH_SUFFIXES}
# Help the user find it if we cannot.
#DOC "Set the PostgreSQL_BINARY_DIR cmake cache entry to the top-level directory containing the PostgreSQL binaries."
#)
set(PostgreSQL_PG_CONFIG  /u01/app/postgres/product/13.4/bin/pg_config)
if (NOT PostgreSQL_PG_CONFIG)
    message(FATAL_ERROR "Unable to find 'pg_config'")
endif ()
message(STATUS "Using pg_config ${PostgreSQL_PG_CONFIG}")

execute_process(
        COMMAND ${PostgreSQL_PG_CONFIG} --sharedir
        OUTPUT_VARIABLE PostgreSQL_SHAREDIR
        OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(
        COMMAND ${PostgreSQL_PG_CONFIG} --pkglibdir
        OUTPUT_VARIABLE PostgreSQL_PKGLIBDIR
        OUTPUT_STRIP_TRAILING_WHITESPACE)

## Install Files

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${EXT_CONTROL_FILE}
    DESTINATION "${PostgreSQL_SHAREDIR}/extension")
foreach(EXT_SQL_FILE ${EXT_SQL_FILES})
    install(
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/${EXT_SQL_FILE}
        DESTINATION "${PostgreSQL_SHAREDIR}/extension")
endforeach(EXT_SQL_FILE)
install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/${EXT_SQL_MAIN}
    DESTINATION "${PostgreSQL_SHAREDIR}/extension"
    RENAME ${PROJECT_NAME}--${PROJECT_VERSION}.sql)
install(
    TARGETS ${PROJECT_NAME}
    DESTINATION ${PostgreSQL_PKGLIBDIR})
