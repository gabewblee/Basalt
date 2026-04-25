#pragma once

#include <vector>

#include "../Types.hpp"

class IBTree {
public:
    /**
     * Destructor for the IBTree class
     */
    virtual ~IBTree() = default;

    /**
     * Builds a B-Tree from flushed Memtable nodes
     * @param nodes The flushed Memtable nodes
     * @return An ordered vector of B-Tree nodes
     */
    virtual std::vector<BTreeNode> build(std::vector<std::pair<Key, Val>> nodes) const = 0;
};