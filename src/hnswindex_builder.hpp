// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef HNSWINDEX_BUILDER_HPP
#define HNSWINDEX_BUILDER_HPP

#include <hnswlib.h>
#include <memory>
#include "util.hpp"
extern "C"
{
#include <postgres.h>

#include <access/genam.h>
#include <utils/relcache.h>
}

class HNSWIndexBuilder
{
public:
    /**
     * @brief constructs index builder
     *
     * @param p_heap pass as of amroutine->ambuild
     * @param p_index pass as of amroutine->ambuild
     * @param p_indexInfo pass as of amroutine->ambuild
     */
    HNSWIndexBuilder(Relation m_heap, Relation m_index, IndexInfo *m_indexInfo);

    void ConstructInternalBuilder(DistanceMethod distance_method);
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
    std::shared_ptr<hnswlib::HierarchicalNSW<float>> vector_index;
    std::shared_ptr<hnswlib::SpaceInterface<float>> distance; 
    /**
     * @brief Cardinality of the vector. This will be initially zero, and
     * set upon scanning first tuple. Then, each tuple will be checked
     * against this value.
     */
    int m_numDimension;

    void ComputeElements();

    friend void HNSWLoadTupleCallback(Relation index,
                                  ItemPointer tid,
                                  Datum *values,
                                  bool *isnull,
                                  bool tupleIsAlive,
                                  void *state);

    friend void HNSWComputeElementsCallback(Relation index,
                                      ItemPointer tid,
                                      Datum *values,
                                      bool *isnull,
                                      bool tupleIsAlive,
                                      void *state);
    
};

#endif
