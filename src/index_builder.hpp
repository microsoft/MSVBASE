// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef INDEX_BUILDER_HPP
#define INDEX_BUILDER_HPP

#include <inc/Core/Common.h>
#include <inc/Core/CommonDataStructure.h>
#include <inc/Core/MetadataSet.h>
#include <inc/Core/VectorIndex.h>
#include "index.hpp"

#include <memory>

extern "C"
{
#include <postgres.h>

#include <access/genam.h>
#include <utils/relcache.h>
}

class IndexOptions
{
public:
    SPTAG::IndexAlgoType algo;
    SPTAG::VectorValueType valueType;

private:
};


class VectorMetadata
{
public:
    std::int32_t blockId;
    std::int32_t offset;

    VectorMetadata(std::int32_t p_blockId, std::int32_t p_offset);
};

/**
 * @brief Build index step-by-step.
 * This class will be used like:
 * - Call IndexBuilder::ConstructInternalBuilder to construct
 *   SPTAG::IndexBuilder for futher usage.
 * - Call IndexBuilder::LoadTuples to load all tuples into memory (maybe
 *   file in the future).
 * - Generate VectorSet and MetadataSet by IndexBuilder::GetVectorSet and
 *   IndexBuilder::GetMetadataSet correspondingly.
 * - Call IndexBuilder::BuildIndex to build index, which uses
 *   SPTAG::BuildIndex.
 * - Call IndexBuilder::SaveIndex to save index.
 */
class IndexBuilder
{
public:
    /**
     * @brief constructs index builder
     *
     * @param p_heap pass as of amroutine->ambuild
     * @param p_index pass as of amroutine->ambuild
     * @param p_indexInfo pass as of amroutine->ambuild
     */
    IndexBuilder(Relation m_heap, Relation m_index, IndexInfo *m_indexInfo);

    /**
     * @brief Constructs SPTAG::IndexBuilder for internal usage.
     */
    void ConstructInternalBuilder();
    /**
     * @brief This function is expected to load all tuples from the specific
     * "heap" (original table), and then transform them into packed vectors
     * in the format of SPTAG::VectorSet.
     *
     * It uses table_index_build_scan to access tuples, and
     * assigns metadata (heap tuple location) to each tuple for later
     * scanning usage. See ivfbuild.c/BuildCallback for a reference.
     */
    void LoadTuples();
    /**
     * @brief Convert internal IndexBuilder::m_vectorData into a
     * SPTAG::VectorSet.
     *
     * @return std::shared_ptr<SPTAG::VectorSet>
     */
    std::shared_ptr<SPTAG::VectorSet> GetVectorSet();
    /**
     * @brief Convert internal IndexBuilder::m_metadata into a
     * SPTAG::MetadataSet.
     *
     * @return std::shared_ptr<SPTAG::MetadataSet>
     */
    std::shared_ptr<SPTAG::MetadataSet> GetMetadataSet();
    /**
     * @brief Build index.
     *
     * @param p_vectorSet Use IndexBuilder::GetVectorSet.
     * @param p_metadataSet Use IndexBuilder::GetMetadataSet.
     */
    void BuildIndex(std::shared_ptr<SPTAG::VectorSet> p_vectorSet,
                    std::shared_ptr<SPTAG::MetadataSet> p_metadataSet);
    /**
     * @brief Save index to a directory.
     *
     * @param p_path The path of index directory.
     */
    void SaveIndex(const std::string &p_path);

    /**
     * @brief Count of index tuples, used by PostgreSQL for statistics.
     */
    double m_indtuples;
    /**
     * @brief Count of heap tuples, used by PostgreSQL for statistics.
     */
    double m_reltuples;

private:
    IndexOptions ParseOptions() const;
    inline size_t VectorCount() const
    {
        return m_vectorData.size() / m_numDimension;
    }

    /**
     * @brief The heap relation, or to say, the original table relation.
     */
    Relation m_heap;
    /**
     * @brief The current index relation, which should contains tuples of
     * referred columns. In PostgreSQL, all indices are considered as
     * relations, just like tables.
     */
    Relation m_index;
    /**
     * @brief Information about current index.
     * @see IndexInfo
     */
    IndexInfo *m_indexInfo;
    /**
     * @brief SPTAG index builder, which will be actually building indices.
     */
    std::shared_ptr<SPTAG::VectorIndex> m_builder;
    /**
     * @brief Cardinality of the vector. This will be initially zero, and
     * set upon scanning first tuple. Then, each tuple will be checked
     * against this value.
     */
    SPTAG::SizeType m_numDimension;
    /**
     * @brief Raw data of all indexed tuples.
     */
    std::vector<float> m_vectorData;
    /**
     * @brief Metadata of all indexed tuples, which is the information
     * needed for scanning to retrieve tuple informations to feed to
     * PostgreSQL.
     */
    std::vector<VectorMetadata> m_metadata;

    friend void LoadTupleCallback(Relation index,
                                  ItemPointer tid,
                                  Datum *values,
                                  bool *isnull,
                                  bool tupleIsAlive,
                                  void *state);
};

#endif
