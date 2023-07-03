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
    
    static std::map<std::string, std::shared_ptr<hnswlib::SpaceInterface<float>>> distanceFunction_map;
    static std::map<std::string, std::shared_ptr<hnswlib::HierarchicalNSW<float>>> vector_index_map;
};
#endif
