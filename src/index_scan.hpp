#ifndef INDEX_SCAN_HPP
#define INDEX_SCAN_HPP

#include <inc/Core/VectorIndex.h>
#include <inc/Core/SearchQuery.h>
#include <inc/Core/ResultIterator.h>
#include <atomic>
#include <map>

class IndexScan
{
public:
    struct WorkSpace
    {
	    std::shared_ptr<SPTAG::ResultIterator> resultIterator;
    };
    static void LoadIndex(const std::string &p_path);
    static std::shared_ptr<SPTAG::ResultIterator> BeginScan(
        const void *p_target, const std::string &p_path);
    static bool GetNet(std::shared_ptr<SPTAG::ResultIterator> &resultIterator,
        SPTAG::BasicResult &result);
    static void EndScan(std::shared_ptr<SPTAG::ResultIterator> &resultIterator);

    static std::map<std::string, std::shared_ptr<SPTAG::VectorIndex>> vector_index_map;
};
#endif
