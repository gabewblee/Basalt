#pragma once

#include "IBTree.hpp"

class BPlusTree : public IBTree {
public:
    /**
     * build - Build a B+ Tree from flushed Memtable nodes.
     * @nodes: The flushed Memtable nodes.
     * Returns: An ordered vector of B+ Tree nodes.
     */
    std::vector<BTreeNode> build(std::vector<std::pair<Key, Val>> nodes) const override;
};
