// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "index_builder.hpp"

#include "util.hpp"

extern "C"
{
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

#include <inc/Core/CommonDataStructure.h>
#include <inc/Core/VectorIndex.h>
#include <inc/Core/VectorSet.h>
#include <string>
#include <iostream>
#include <memory>

VectorMetadata::VectorMetadata(std::int32_t p_blockId, std::int32_t p_offset)
{
    blockId = p_blockId;
    offset = p_offset;
}

IndexBuilder::IndexBuilder(Relation p_heap,
                           Relation p_index,
                           IndexInfo *p_indexInfo)
{
    m_heap = p_heap;
    m_index = p_index;
    m_indexInfo = p_indexInfo;
    // FIXME: find a way of getting dim from m_indexInfo
    m_numDimension = 0;
}

IndexOptions IndexBuilder::ParseOptions() const
{
    IndexOptions ret;
    ret.algo = SPTAG::IndexAlgoType::BKT;
    // TODO: support more value types
    ret.valueType = SPTAG::VectorValueType::Float;
    return ret;
}

int ParaGetThreads(Relation index) {
  sptag_ParaOptions *opts = (sptag_ParaOptions *)index->rd_options;

  if (opts)
    return opts->threads;

  return 1;
}

sptag_DistCalcMethod ParaGetDistmethod(Relation index) {
  sptag_ParaOptions *opts = (sptag_ParaOptions *)index->rd_options;

  if (opts)
    return opts->distmethod;

  return sptag_Inner_Product;
}

void IndexBuilder::ConstructInternalBuilder()
{
    auto opts = ParseOptions();
    m_builder = SPTAG::VectorIndex::CreateInstance(opts.algo, opts.valueType);
    
    std::cout<<ParaGetDistmethod(m_index)<<std::endl;
    m_builder->SetParameter("NumberOfThreads", std::to_string(ParaGetThreads(m_index)));
    switch(ParaGetDistmethod(m_index))
    {
        case sptag_Inner_Product:
            m_builder->SetParameter("DistCalcMethod", "InnerProduct");
            break;
        case sptag_L2_Distance:
            m_builder->SetParameter("DistCalcMethod", "L2_Distance");
            break;
        default:
            elog(ERROR, "sptag index parameter value error.");
    }
    
}

void LoadTupleCallback(Relation index,
                       ItemPointer tid,
                       Datum *values,
                       bool *isnull,
                       bool tupleIsAlive,
                       void *state)
{
    IndexBuilder &self = *reinterpret_cast<IndexBuilder *>(state);
    Datum value = values[0];

    if (*isnull)
    {
        return;
    }

    // retrieve array and perform some checks
    auto array = convert_array_to_vector(value);
    // check vector length
    [[unlikely]] if (self.m_numDimension == 0)
    {
        self.m_numDimension = array.size();
    }
    if (static_cast<size_t>(self.m_numDimension) != array.size())
    {
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("inconsistent array length, expected %d, found %ld",
                        self.m_numDimension,
                        array.size())));
    }

    // append current vector data to the dataset
    self.m_vectorData.insert(
        self.m_vectorData.end(), array.begin(), array.end());
    self.m_indtuples += 1;

    // Assign metadata to current tuple, i.e. the position of current tuple.
    // The metadata is used for retrieving data from the heap, for what data is
    // required, see <access/relscan.h> L121-129.
    // Because we do not have index tuple, so the metadata must be sufficient
    // for getting an heap tuple.
    // TODO: check if the data is sufficient
    self.m_metadata.push_back(
        VectorMetadata(ItemPointerGetBlockNumberNoCheck(tid),
                       ItemPointerGetOffsetNumberNoCheck(tid)));
}

void IndexBuilder::LoadTuples()
{
    m_reltuples = table_index_build_scan(m_heap,
                                         m_index,
                                         m_indexInfo,
                                         false,
                                         true,
                                         LoadTupleCallback,
                                         reinterpret_cast<void *>(this),
                                         NULL);
}

std::shared_ptr<SPTAG::VectorSet> IndexBuilder::GetVectorSet()
{
    auto data = SPTAG::ByteArray::Alloc(
        m_vectorData.size() * sizeof(float));
    //std::copy(reinterpret_cast<std::uint8_t *>(m_vectorData.data()),
    //          reinterpret_cast<std::uint8_t *>(m_vectorData.data() +
    //                                           m_vectorData.size()),
    //          data.Data());
    //std::uint8_t result[m_vectorData.size() * sizeof(float)];
    int step = 0;
    for (float i : m_vectorData)
    {
        std::memcpy(data.Data() + step * sizeof(float), &i, sizeof(float));
        step++;
    }

    return std::make_shared<SPTAG::BasicVectorSet>(
        data, SPTAG::VectorValueType::Float, m_numDimension, VectorCount());
}

std::shared_ptr<SPTAG::MetadataSet> IndexBuilder::GetMetadataSet()
{
    // TODO: how to construct a MetadataSet needs futher investigation,
    // here it just works, but not assured.
    auto set = std::make_shared<SPTAG::MemMetadataSet>(
        1000000,
        VectorCount() + 1,
        8);
    for (const auto &data : m_metadata)
    {
        std::uint64_t number = data.blockId;
        number = (number << 32) + data.offset;
        std::uint8_t result[sizeof(number)];
        std::memcpy(result, &number, sizeof(number));
        std::cout << "block_id:" << data.blockId << std::endl;
        std::cout << "offset:" << data.offset << std::endl;
        std::cout << "number:" << number << std::endl;
        set->Add(SPTAG::ByteArray(result, 8, false));
    }
    return set;
}

void IndexBuilder::BuildIndex(std::shared_ptr<SPTAG::VectorSet> p_vectorSet,
                              std::shared_ptr<SPTAG::MetadataSet> p_metadataSet)
{
    m_builder->BuildIndex(p_vectorSet, p_metadataSet);
}

void IndexBuilder::SaveIndex(const std::string &p_path)
{
    m_builder->SaveIndex(p_path);
}
