#pragma once

#include <vector>

#include "../Types.hpp"

class IBTree {
public:
    /**
     * ~IBTree - Destructor for the IBTree class.
     */
    virtual ~IBTree() = default;

    /**
     * build - Build a B-Tree from flushed Memtable nodes.
     * @nodes: The flushed Memtable nodes.
     * Returns: An ordered vector of B-Tree nodes.
     */
    virtual std::vector<BTreeNode> build(std::vector<std::pair<Key, Val>> nodes) const = 0;
};
