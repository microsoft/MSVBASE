// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef SPANNINDEX_SCAN_HPP
#define SPANNINDEX_SCAN_HPP

#include "inc/Core/SPANNResultIterator.h"
#include <inc/Core/SearchQuery.h>
#include <inc/Core/VectorIndex.h>
#include "inc/Core/SPANN/Index.h"

#include <atomic>
#include <map>
#include <vector>

class SPANNIndexScan
{
public:
    struct basic_result_compare_gt
    {
        bool operator()(const SPTAG::BasicResult& lhs, const SPTAG::BasicResult& rhs)
	{
	    return lhs.Dist > rhs.Dist;
	}
    };
    struct WorkSpace
    {
        std::shared_ptr<SPTAG::SPANN::SPANNResultIterator<float>>
            resultIterator;
	std::priority_queue<SPTAG::BasicResult,std::vector<SPTAG::BasicResult>, basic_result_compare_gt> distanceQueue;
    };
    static void LoadIndex(const std::string &p_path);
    static std::shared_ptr<SPTAG::SPANN::SPANNResultIterator<float>>
    BeginScan(const void *p_target, const std::string &p_path);
    static bool GetNet(
        std::shared_ptr<SPTAG::SPANN::SPANNResultIterator<float>>
            &resultIterator,
        SPTAG::BasicResult &result);
    static void EndScan(
        std::shared_ptr<SPTAG::SPANN::SPANNResultIterator<float>>
            &resultIterator);

    static std::map<std::string, std::shared_ptr<SPTAG::VectorIndex>>
        vector_index_map;
};
#endif
