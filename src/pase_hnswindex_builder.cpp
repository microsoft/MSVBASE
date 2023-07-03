// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pase_hnswindex_builder.hpp"
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

PASE_HNSWIndexBuilder::PASE_HNSWIndexBuilder(Relation p_heap,
                           Relation p_index,
                           IndexInfo *p_indexInfo)
{
    m_heap = p_heap;
    m_index = p_index;
    m_indexInfo = p_indexInfo;
    m_numDimension = 0;
}

void PASE_HNSWIndexBuilder::ConstructInternalBuilder(DistanceMethod distance_method)
{
    ComputeElements();
    switch (distance_method)
    {
    case DistanceMethod::L2:
        distance = std::make_shared<hnswlib::L2Space>(m_numDimension);
        break;
    case DistanceMethod::InnerProduct:
        distance = std::make_shared<hnswlib::InnerProductSpace>(m_numDimension);
        break;
    default:
        exit(1);
    }
    vector_index = std::make_shared<hnswlib::HierarchicalNSW<float>>(
        distance.get(), m_indtuples);
}

void PASE_HNSWComputeElementsCallback(Relation index,
                                             ItemPointer tid,
                                             Datum *values,
                                             bool *isnull,
                                             bool tupleIsAlive,
                                             void *state)
{
    PASE_HNSWIndexBuilder &self = *reinterpret_cast<PASE_HNSWIndexBuilder *>(state);
    Datum value = values[0];

    if (*isnull)
    {
        return;
    }

    // retrieve array and perform some checks
    auto array = convert_array_to_vector(value);

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
    self.m_indtuples += 1;
}

void PASE_HNSWIndexBuilder::ComputeElements()
{
    m_reltuples = table_index_build_scan(m_heap,
                                         m_index,
                                         m_indexInfo,
                                         false,
                                         true,
                                         PASE_HNSWComputeElementsCallback,
                                         reinterpret_cast<void *>(this),
                                         NULL);
}

void PASE_HNSWLoadTupleCallback(Relation index,
                         ItemPointer tid,
                         Datum *values,
                         bool *isnull,
                         bool tupleIsAlive,
                         void *state)
{
    PASE_HNSWIndexBuilder &self = *reinterpret_cast<PASE_HNSWIndexBuilder *>(state);
    Datum value = values[0];

    if (*isnull)
    {
        return;
    }

    // retrieve array and perform some checks
    auto array = convert_array_to_vector(value);
    if (static_cast<size_t>(self.m_numDimension) != array.size())
    {
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("inconsistent array length, expected %d, found %ld",
                        self.m_numDimension,
                        array.size())));
    }

    std::int32_t blockId = ItemPointerGetBlockNumberNoCheck(tid);
    std::int32_t offset = ItemPointerGetOffsetNumberNoCheck(tid);
    std::uint64_t number = blockId;
    number = (number << 32) + offset;
    self.vector_index->addPoint((char *)array.data(), number);
}

void PASE_HNSWIndexBuilder::LoadTuples()
{
    table_index_build_scan(m_heap,
                           m_index,
                           m_indexInfo,
                           false,
                           true,
                           PASE_HNSWLoadTupleCallback,
                           reinterpret_cast<void *>(this),
                           NULL);
}

void PASE_HNSWIndexBuilder::SaveIndex(const std::string &p_path)
{
    vector_index->saveIndex(p_path);
}


