#pragma once

#include <fstream>
#include <utility>
#include <vector>

#include "../storage/BPlusTree.hpp"
#include "../Types.hpp"

class Writer {
public:
    /**
     * Constructor for the Writer class
     * @param out The output file stream to write to
     */
    Writer(std::ofstream& out);

    /**
     * Append a key-value pair to the current leaf node, sealing when full
     * @param key The key to append
     * @param val The value to append
     */
    void add(Key key, Val val);

    /**
     * Seal the final leaf, build internal nodes bottom-up, and return leaf count
     * @return Number of leaf nodes written
     */
    int finish();

private:
    std::ofstream& out;                     /* Output file stream */
    BTreeNode leaf;                         /* Current leaf node being filled */
    std::vector<std::pair<Key, int>> spine; /* Tracks (first_key, position) of each sealed leaf */
    int written;                            /* Number of nodes written so far */

    /**
     * Write the current leaf to disk and record its metadata
     */
    void seal();
};