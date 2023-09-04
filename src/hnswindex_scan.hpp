// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef HNSWINDEX_SCAN_HPP
#define HNSWINDEX_SCAN_HPP

#include "util.hpp"
#include <hnswlib.h>
#include <memory>
#include <atomic>
#include <map>
#include <queue>
extern "C"
{
#include <postgres.h>
#include <access/genam.h>
#include <utils/relcache.h>
#include <access/tableam.h>
#include <storage/block.h>
#include <storage/bufmgr.h>
#include <utils/array.h>
#include <utils/elog.h>
#include <utils/fmgrprotos.h>
#include <utils/lsyscache.h>
#include <utils/numeric.h>
#include <utils/relcache.h>
}
class HNSWIndexScan
{
public:
    struct WorkSpace
    {
        std::shared_ptr<hnswlib::ResultIterator<float>> resultIterator;
	    std::vector<float> array;
        std::priority_queue<float> distanceQueue;
    };
    static void LoadIndex(const std::string &p_path,
                          DistanceMethod distance_method, int dim);
    static std::shared_ptr<hnswlib::ResultIterator<float>> BeginScan(
        const void *p_target, const std::string &p_path);
    static hnswlib::QueryResult<float>* GetNet(
        std::shared_ptr<hnswlib::ResultIterator<float>> &resultIterator);
    static void EndScan(
        std::shared_ptr<hnswlib::ResultIterator<float>> &resultIterator);
    static bool Insert(const std::string &p_path,
        Datum* values,
        bool* isnull,
        ItemPointer heap_tid,
        IndexUniqueCheck checkUnique,
	int dim);
    static IndexBulkDeleteResult* BulkDelete(const std::string& p_path,
            IndexVacuumInfo* info,
            IndexBulkDeleteResult* stats,
            IndexBulkDeleteCallback callback,
            void* callback_state);

    static std::map<std::string, std::shared_ptr<hnswlib::SpaceInterface<float>>> distanceFunction_map;
    static std::map<std::string, std::shared_ptr<hnswlib::HierarchicalNSW<float>>> vector_index_map;
};
#endif
