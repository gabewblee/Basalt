#include <gtest/gtest.h>
#include <limits>
#include <vector>

#include "../src/storage/BPlusTree.hpp"
#include "../src/Config.hpp"
#include "../src/Types.hpp"

TEST(BPlusTreeTest, EmptyInput) {
    BPlusTree tree;
    std::vector<BTreeNode> result = tree.build({});
    ASSERT_TRUE(result.empty());
}

TEST(BPlusTreeTest, SingleEntry) {
    BPlusTree tree;
    std::vector<BTreeNode> result = tree.build({{42, 100}});
    ASSERT_EQ(result.size(), 1u);
    ASSERT_TRUE(result[0].leaf);
    ASSERT_EQ(result[0].cnt, 1u);
    ASSERT_EQ(result[0].keys[0], 42);
    ASSERT_EQ(result[0].vals[0], 100);
}

TEST(BPlusTreeTest, TwoLeavesAndRoot) {
    BPlusTree tree;
    std::vector<std::pair<Key, Val>> pairs;
    for (int i = 0; i < BTREE_NODE_DEF_SZ + 1; i++)
        pairs.push_back({(Key)i, (Val)(i * 2)});
    std::vector<BTreeNode> result = tree.build(pairs);
    ASSERT_EQ(result.size(), 3u);
    ASSERT_TRUE(result[0].leaf);
    ASSERT_TRUE(result[1].leaf);
    ASSERT_FALSE(result[2].leaf);
    ASSERT_EQ(result[2].cnt, 2u);
}

TEST(BPlusTreeTest, InternalChildOffsetsValid) {
    BPlusTree tree;
    std::vector<std::pair<Key, Val>> pairs;
    for (int i = 0; i < BTREE_NODE_DEF_SZ * 4; i++)
        pairs.push_back({(Key)i, (Val)i});
    std::vector<BTreeNode> result = tree.build(pairs);
    for (const auto& node : result) {
        if (node.leaf)
            continue;
        for (uint32_t i = 0; i < node.cnt; i++) {
            ASSERT_GE((int)node.vals[i], 0);
            ASSERT_LT((int)node.vals[i], (int)result.size());
        }
    }
}

TEST(BPlusTreeTest, TraversalFindsPresentKey) {
    BPlusTree tree;
    std::vector<std::pair<Key, Val>> pairs;
    for (int i = 0; i < 1000; i++)
        pairs.push_back({(Key)(i * 3), (Val)(i * 7)});
    std::vector<BTreeNode> result = tree.build(pairs);

    Key target = 300;
    Val expected = 700;
    int idx = (int)result.size() - 1;
    bool found = false;

    while (true) {
        const BTreeNode& node = result[idx];
        if (node.leaf) {
            for (uint32_t i = 0; i < node.cnt; i++) {
                if (node.keys[i] == target) {
                    ASSERT_EQ(node.vals[i], expected);
                    found = true;
                    break;
                }
            }
            break;
        }
        idx = (int)node.vals[0];
        for (uint32_t i = 1; i < node.cnt; i++) {
            if (target >= node.keys[i])
                idx = (int)node.vals[i];
            else
                break;
        }
    }

    ASSERT_TRUE(found);
}

TEST(BPlusTreeTest, LeafKeysAreSorted) {
    BPlusTree tree;
    std::vector<std::pair<Key, Val>> pairs;
    for (int i = 0; i < 800; i++)
        pairs.push_back({(Key)(i * 5), (Val)i});
    std::vector<BTreeNode> result = tree.build(pairs);

    Key prev = std::numeric_limits<Key>::min();
    for (const auto& node : result) {
        if (!node.leaf)
            break;
        for (uint32_t i = 0; i < node.cnt; i++) {
            ASSERT_GT(node.keys[i], prev);
            prev = node.keys[i];
        }
    }
}
