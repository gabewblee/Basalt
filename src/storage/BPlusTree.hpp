#pragma once

#include "IBTree.hpp"

class BPlusTree : public IBTree {
public:
    /**
     * Builds a B+ Tree from flushed Memtable nodes
     * @param nodes The flushed Memtable nodes
     * @return An ordered vector of B+ Tree nodes
     */
    std::vector<BTreeNode> build(std::vector<std::pair<Key, Val>> nodes) const override;
};