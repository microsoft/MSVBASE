// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pase_hnswindex_scan.hpp"

std::map<std::string, std::shared_ptr<hnswlib::SpaceInterface<float>>> PASE_HNSWIndexScan::distanceFunction_map;
std::map<std::string, std::shared_ptr<hnswlib::HierarchicalNSW<float>>> PASE_HNSWIndexScan::vector_index_map;

void PASE_HNSWIndexScan::LoadIndex(const std::string &p_path,
                              DistanceMethod distance_method,
                              int dim)
{
    if(vector_index_map.find(p_path)==vector_index_map.end())
    {
        std::shared_ptr<hnswlib::SpaceInterface<float>> distanceFunction;
        switch (distance_method)
        {
        case DistanceMethod::L2:
            distanceFunction = std::make_shared<hnswlib::L2Space>(dim);
            break;
        case DistanceMethod::InnerProduct:
            distanceFunction = std::make_shared<hnswlib::InnerProductSpace>(dim);
            break;
        default:
            exit(1);
        }
        auto vector_index = std::make_shared<hnswlib::HierarchicalNSW<float>>(
            distanceFunction.get(), p_path);
        distanceFunction_map.insert(std::make_pair(p_path,distanceFunction));
        vector_index_map.insert(std::make_pair(p_path,vector_index));
    }
}

std::priority_queue<std::pair<float, std::uint64_t>> PASE_HNSWIndexScan::BeginScan(
    const void* p_target, const std::string &p_path, int k)
{
    return vector_index_map[p_path]->searchKnn(p_target, k);
}


void PASE_HNSWIndexScan::EndScan()
{
}
