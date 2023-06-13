#ifndef PASE_HNSWINDEX_SCAN_HPP
#define PASE_HNSWINDEX_SCAN_HPP

#include "util.hpp"
#include <hnswlib.h>
#include <memory>
#include <atomic>
#include <map>

class PASE_HNSWIndexScan
{
public:
    struct WorkSpace
    {
	    std::vector<float> array;
        std::priority_queue<std::pair<float, std::uint64_t>> result;
    };
    static void LoadIndex(const std::string &p_path,
                          DistanceMethod distance_method, int dim);
    static std::priority_queue<std::pair<float, std::uint64_t>> BeginScan(
        const void *p_target, const std::string &p_path, int k);
    static void EndScan();
    
    static std::map<std::string, std::shared_ptr<hnswlib::SpaceInterface<float>>> distanceFunction_map;
    static std::map<std::string, std::shared_ptr<hnswlib::HierarchicalNSW<float>>> vector_index_map;
};
#endif
