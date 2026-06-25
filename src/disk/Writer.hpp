#pragma once

#include <fstream>
#include <utility>
#include <vector>

#include "../storage/BPlusTree.hpp"
#include "../Types.hpp"

class Writer {
public:
    /**
     * Writer - Constructor for the Writer class.
     * @out: The output file stream to write to.
     */
    Writer(std::ofstream& out);

    /**
     * add - Append a key-value pair to the current leaf node, sealing when full.
     * @key: The key to append.
     * @val: The value to append.
     */
    void add(Key key, Val val);

    /**
     * finish - Seal the final leaf, build internal nodes bottom-up, and return leaf count.
     * Returns: The number of leaf nodes written.
     */
    int finish();

private:
    std::ofstream&                   out;     /* Output file stream                         */
    BTreeNode                        leaf;    /* Current leaf node being filled             */
    std::vector<std::pair<Key, int>> spine;   /* First key and position of each sealed leaf */
    int                              written; /* Number of nodes written so far             */

    /**
     * seal - Write the current leaf to disk and record its metadata.
     */
    void seal();
};
