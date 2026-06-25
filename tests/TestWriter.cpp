#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

#include "../src/disk/Writer.hpp"
#include "../src/Config.hpp"
#include "../src/Types.hpp"

TEST(WriterTest, EmptyWriter) {
    std::filesystem::remove("/tmp/writer_test_empty.bin");
    {
        std::ofstream out("/tmp/writer_test_empty.bin", std::ios::binary);
        Writer w(out);
        ASSERT_EQ(w.finish(), 0);
    }
    std::ifstream in("/tmp/writer_test_empty.bin", std::ios::binary);
    in.seekg(0, std::ios::end);
    ASSERT_EQ((int)in.tellg(), 0);
    std::filesystem::remove("/tmp/writer_test_empty.bin");
}

TEST(WriterTest, SingleLeaf) {
    std::filesystem::remove("/tmp/writer_test_single.bin");
    {
        std::ofstream out("/tmp/writer_test_single.bin", std::ios::binary);
        Writer w(out);
        w.add(1, 10);
        w.add(2, 20);
        ASSERT_EQ(w.finish(), 1);
    }

    std::ifstream in("/tmp/writer_test_single.bin", std::ios::binary);
    BTreeNode leaf(true);
    in.read(reinterpret_cast<char*>(&leaf), sizeof(BTreeNode));
    ASSERT_TRUE(leaf.leaf);
    ASSERT_EQ(leaf.cnt, 2u);
    ASSERT_EQ(leaf.keys[0], 1);
    ASSERT_EQ(leaf.vals[1], 20);
    std::filesystem::remove("/tmp/writer_test_single.bin");
}

TEST(WriterTest, TwoLeavesAndRoot) {
    std::filesystem::remove("/tmp/writer_test_two.bin");
    {
        std::ofstream out("/tmp/writer_test_two.bin", std::ios::binary);
        Writer w(out);
        for (int i = 0; i < BTREE_NODE_DEF_SZ + 1; i++)
            w.add((Key)i, (Val)(i * 11));
        ASSERT_EQ(w.finish(), 2);
    }

    std::ifstream in("/tmp/writer_test_two.bin", std::ios::binary);
    BTreeNode leaf0(true), leaf1(true), root(false);
    in.read(reinterpret_cast<char*>(&leaf0), sizeof(BTreeNode));
    in.read(reinterpret_cast<char*>(&leaf1), sizeof(BTreeNode));
    in.read(reinterpret_cast<char*>(&root), sizeof(BTreeNode));
    ASSERT_TRUE(leaf0.leaf);
    ASSERT_TRUE(leaf1.leaf);
    ASSERT_FALSE(root.leaf);
    ASSERT_EQ(root.cnt, 2u);
    ASSERT_EQ((int)root.vals[0], 0);
    ASSERT_EQ((int)root.vals[1], 1);
    std::filesystem::remove("/tmp/writer_test_two.bin");
}

TEST(WriterTest, LastNodeIsRoot) {
    std::filesystem::remove("/tmp/writer_test_last.bin");
    {
        std::ofstream out("/tmp/writer_test_last.bin", std::ios::binary);
        Writer w(out);
        for (int i = 0; i < BTREE_NODE_DEF_SZ * 3 + 10; i++)
            w.add((Key)i, (Val)i);
        w.finish();
    }

    std::ifstream in("/tmp/writer_test_last.bin", std::ios::binary);
    in.seekg(0, std::ios::end);
    std::streamsize sz = in.tellg();
    in.seekg(sz - (std::streamsize)sizeof(BTreeNode));
    BTreeNode node(false);
    in.read(reinterpret_cast<char*>(&node), sizeof(BTreeNode));
    ASSERT_FALSE(node.leaf);
    std::filesystem::remove("/tmp/writer_test_last.bin");
}

TEST(WriterTest, TraversalFindsKey) {
    std::filesystem::remove("/tmp/writer_test_find.bin");
    {
        std::ofstream out("/tmp/writer_test_find.bin", std::ios::binary);
        Writer w(out);
        for (int i = 0; i < 1000; i++)
            w.add((Key)i, (Val)(i * 9));
        w.finish();
    }

    std::ifstream in("/tmp/writer_test_find.bin", std::ios::binary);
    in.seekg(0, std::ios::end);
    int n = (int)(in.tellg() / (std::streamsize)sizeof(BTreeNode));
    int idx = n - 1;
    Key target = 750;
    bool found = false;

    while (true) {
        in.seekg((std::streamoff)(idx * (int)sizeof(BTreeNode)));
        BTreeNode node(false);
        in.read(reinterpret_cast<char*>(&node), sizeof(BTreeNode));
        if (node.leaf) {
            for (uint32_t i = 0; i < node.cnt; i++) {
                if (node.keys[i] == target) {
                    ASSERT_EQ(node.vals[i], 6750);
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
    std::filesystem::remove("/tmp/writer_test_find.bin");
}
