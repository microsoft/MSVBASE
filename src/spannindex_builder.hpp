#ifndef SPANNINDEX_BUILDER_HPP
#define SPANNINDEX_BUILDER_HPP

#include <fstream>
#include <vector>
extern "C"
{
#include <postgres.h>

#include <access/genam.h>
#include <access/tableam.h>
#include <utils/relcache.h>
}

class SPANNIndexBuilder
{
public:
    SPANNIndexBuilder(Relation m_heap, Relation m_index, IndexInfo *m_indexInfo)
        : m_heap(m_heap),
          m_index(m_index),
          m_indexInfo(m_indexInfo),
          m_numDimension(0)
    {
    }
    void LoadTuples();
    void SaveIndex(const std::string &p_path);
    double m_indtuples;
    double m_reltuples;

private:
    Relation m_heap;
    Relation m_index;
    IndexInfo *m_indexInfo;
    int m_numDimension;
    std::vector<std::uint64_t> m_meta;
    friend void SPANNLoadTupleCallback(Relation index,
                                       ItemPointer tid,
                                       Datum *values,
                                       bool *isnull,
                                       bool tupleIsAlive,
                                       void *state);
};

void SPANNLoadTupleCallback(Relation index,
                            ItemPointer tid,
                            Datum *values,
                            bool *isnull,
                            bool tupleIsAlive,
                            void *state)
{
    SPANNIndexBuilder &self = *reinterpret_cast<SPANNIndexBuilder *>(state);
    std::int32_t blockId = ItemPointerGetBlockNumberNoCheck(tid);
    std::int32_t offset = ItemPointerGetOffsetNumberNoCheck(tid);
    std::uint64_t meta = blockId;
    meta = (meta << 32) + offset;
    self.m_meta.push_back(meta);
}

void SPANNIndexBuilder::LoadTuples()
{
    table_index_build_scan(m_heap,
                           m_index,
                           m_indexInfo,
                           false,
                           true,
                           SPANNLoadTupleCallback,
                           reinterpret_cast<void *>(this),
                           NULL);
}

void SPANNIndexBuilder::SaveIndex(const std::string &p_path)
{
    std::vector<uint64_t> meta_index;
    meta_index.resize(m_meta.size() + 1);
    for (uint64_t i = 0; i <= m_meta.size(); i++)
        meta_index[i] = i * sizeof(uint64_t);  // uint64_t -> 8 bytes

    // need to be tested
    std::string meta_bin = p_path + "/meta.bin";
    std::ofstream meta_out(meta_bin, std::ios::out | std::ios::binary);
    meta_out.write(reinterpret_cast<char *>(m_meta.data()),
                   m_meta.size() * sizeof(uint64_t));
    meta_out.close();
    std::string meta_index_bin = p_path + "/metaIndex.bin";
    std::ofstream meta_index_out(meta_index_bin,
                                 std::ios::out | std::ios::binary);
    int32_t meta_size = m_meta.size();
    // 4 bytes representing size
    meta_index_out.write(reinterpret_cast<char *>(&meta_size), sizeof(int32_t));
    // meta byte-array index indicators
    meta_index_out.write(reinterpret_cast<char *>(meta_index.data()),
                         meta_index.size() * sizeof(uint64_t));
    meta_index_out.close();
}

#endif
