find_package(OpenMP REQUIRED)
set (LIBRARYONLY ON)
set(SPTAG_SOURCE "${CMAKE_CURRENT_LIST_DIR}/SPTAG")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/SPTAG")

set(HNSW_SOURCE "${CMAKE_CURRENT_LIST_DIR}/hnsw")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/hnsw")

function(thirdparty_dependent target)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -mavx2)

    add_definitions(-DUSING_APSDK_NAMESPACE -DNOMINMAX -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS -DUNICODE -DZ_PREFIX -DLOG4CPP_FIX_ERROR_COLLISION)

    target_include_directories("${target}" BEFORE PUBLIC "${SPTAG_SOURCE}/AnnService")
    target_link_libraries("${target}" PostgreSQL::PostgreSQL OpenMP::OpenMP_CXX SPTAGLibStatic)

    target_include_directories("${target}" BEFORE PUBLIC "${HNSW_SOURCE}/hnswlib")
    target_link_libraries("${target}" hnswlib)
endfunction()
