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
