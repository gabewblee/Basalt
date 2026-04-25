#pragma once

#include <cstdint>
#include <fstream>
#include <string>

#include "buf/LRU.hpp"
#include "Config.hpp"

typedef std::int64_t Key;                       /* Key type */
typedef std::int64_t Val;                       /* Value type */

typedef struct Frame {
    std::string pid;                            /* Page ID */
    uint8_t data[PG_DEF_SZ];                    /* Page data */
    Frame* next;                                /* Next frame in the list */
    Frame* prev;                                /* Previous frame in the list */
} Frame;

typedef struct Bucket {
    int depth;                                  /* Bucket depth */
    int occupancy;                              /* Bucket occupancy */
    int capacity;                               /* Bucket capacity */
    Frame* head;                                /* Bucket head */
    Frame* tail;                                /* Bucket tail */
    LRU lru;                                    /* Bucket LRU */
} Bucket;

typedef struct BTreeNode {
    Key keys[BTREE_NODE_DEF_SZ];                /* B-Tree node keys */
    Val vals[BTREE_NODE_DEF_SZ + 1];            /* B-Tree node values */
    uint32_t cnt;                               /* B-Tree node entry count */
    bool leaf;                                  /* Whether the node is a leaf */

    /**
     * Constructor for BTreeNode
     * @param leaf Whether the node is a leaf
     */
    BTreeNode(bool leaf) : cnt(0), leaf(leaf) {};
} BTreeNode;

typedef struct Cursor {
    std::ifstream in;                           /* Input file stream */
    std::string path;                           /* SSTable file path */
    std::streamoff base;                        /* B-Tree base offset */
    BTreeNode node;                             /* Current B-Tree node */
    int nleafs;                                 /* Number of B-Tree leaf nodes */
    int lidx;                                   /* Index of the current leaf node */
    int kidx;                                   /* Index of the current key within the node */
    int lvl;                                    /* SSTable level */
    bool valid;                                 /* Whether the cursor contains a valid entry */

    /**
     * Constructor for Cursor
     */
    Cursor() : base(0), node(true), nleafs(0), lidx(0), kidx(0), lvl(0), valid(false) {}
} Cursor;