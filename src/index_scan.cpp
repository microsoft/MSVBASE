#include "index_scan.hpp"


std::map<std::string, std::shared_ptr<SPTAG::VectorIndex>> IndexScan::vector_index_map;

void IndexScan::LoadIndex(const std::string &p_path)
{
    if(vector_index_map.find(p_path)==vector_index_map.end())
    {
        std::shared_ptr<SPTAG::VectorIndex> vector_index;
        SPTAG::VectorIndex::LoadIndex(p_path, vector_index);
        vector_index_map.insert(std::make_pair(p_path,vector_index));
    }
}

std::shared_ptr<SPTAG::ResultIterator> IndexScan::BeginScan(
    const void *p_target, const std::string &p_path)
{
    return vector_index_map[p_path]->GetIterator(p_target);
}

bool IndexScan::GetNet(std::shared_ptr<SPTAG::ResultIterator> &resultIterator,
                   SPTAG::BasicResult &result)
{
    return resultIterator->Next(result);
}

void IndexScan::EndScan(std::shared_ptr<SPTAG::ResultIterator> &resultIterator)
{
    resultIterator->Close();
}

