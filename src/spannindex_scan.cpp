#include "spannindex_scan.hpp"

std::map<std::string, std::shared_ptr<SPTAG::VectorIndex>>
    SPANNIndexScan::vector_index_map;

void SPANNIndexScan::LoadIndex(const std::string &p_path)
{
    if (vector_index_map.find(p_path) == vector_index_map.end())
    {
        std::shared_ptr<SPTAG::VectorIndex> vector_index;
        SPTAG::VectorIndex::LoadIndex("/indexdata" + p_path, vector_index);
        vector_index_map.insert(std::make_pair(p_path, vector_index));
    }
}

std::shared_ptr<SPTAG::SPANN::SPANNResultIterator<float>>
SPANNIndexScan::BeginScan(const void *p_target, const std::string &p_path)
{
    return ((SPTAG::SPANN::Index<float> *) (vector_index_map[p_path].get()))
        ->GetSPANNIterator(p_target, false, 32);
}

bool SPANNIndexScan::GetNet(
    std::shared_ptr<SPTAG::SPANN::SPANNResultIterator<float>>
        &resultIterator,
    SPTAG::BasicResult &result)
{
    return resultIterator->Next(result);
}

void SPANNIndexScan::EndScan(
    std::shared_ptr<SPTAG::SPANN::SPANNResultIterator<float>>
        &resultIterator)
{
    resultIterator->Close();
}
