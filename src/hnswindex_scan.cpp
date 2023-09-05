// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "hnswindex_scan.hpp"

std::map<std::string, std::shared_ptr<hnswlib::SpaceInterface<float>>> HNSWIndexScan::distanceFunction_map;
std::map<std::string, std::shared_ptr<hnswlib::HierarchicalNSW<float>>> HNSWIndexScan::vector_index_map;

void HNSWIndexScan::LoadIndex(const std::string &p_path,
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

std::shared_ptr<hnswlib::ResultIterator<float>> HNSWIndexScan::BeginScan(
    const void* p_target, const std::string &p_path)
{
    return std::make_shared<hnswlib::ResultIterator<float>>(vector_index_map[p_path].get(), p_target);
}

hnswlib::QueryResult<float>* HNSWIndexScan::GetNet(
    std::shared_ptr<hnswlib::ResultIterator<float>> &resultIterator)
{
    return resultIterator->Next();
}

void HNSWIndexScan::EndScan(
    std::shared_ptr<hnswlib::ResultIterator<float>> &resultIterator)
{
    resultIterator->Close();
}

bool HNSWIndexScan::Insert(const std::string &p_path,
    Datum* values,
    bool* isnull,
    ItemPointer heap_tid,
    IndexUniqueCheck checkUnique,
    int dim)
{
    if (*isnull)
    {
        return true;
    }

    Datum value = values[0];

    // retrieve array and perform some checks
    auto array = convert_array_to_vector(value);
    if (static_cast<size_t>(dim) != array.size())
    {
        ereport(ERROR,
            (errcode(ERRCODE_DATA_EXCEPTION),
                errmsg("inconsistent array length, expected %d, found %ld",
                    dim,
                    array.size())));
    }

    std::int32_t blockId = ItemPointerGetBlockNumberNoCheck(heap_tid);
    std::int32_t offset = ItemPointerGetOffsetNumberNoCheck(heap_tid);
    std::uint64_t number = blockId;
    number = (number << 32) + offset;
    vector_index_map[p_path]->addPoint((char*)array.data(), number);
    return true;
}
