#include <gtest/gtest.h>

#include "../src/Config.hpp"
#include "../src/memtable/Memtable.hpp"

TEST(MemtableTest, BasicPutGet) {
    Memtable mem;

    int result = mem.put(1, 1000);
    ASSERT_EQ(result, 0);

    auto val = mem.get(1);
    ASSERT_TRUE(val.has_value());
    ASSERT_EQ(val.value(), 1000);
}

TEST(MemtableTest, GetNonExistent) {
    Memtable mem;

    auto val = mem.get(-999999);
    ASSERT_FALSE(val.has_value());
}

TEST(MemtableTest, UpdateExisting) {
    Memtable mem;

    mem.put(10, 1);
    mem.put(10, 2);

    auto val = mem.get(10);
    ASSERT_TRUE(val.has_value());
    ASSERT_EQ(val.value(), 2);
}

TEST(MemtableTest, SizeTrackingDefaultLimit) {
    Memtable mem;

    ASSERT_FALSE(mem.full());
    for (int i = 0; i < MEMTABLE_DEF_SZ; i++) {
        ASSERT_EQ(mem.put((Key)i, (Val)(i * 10)), 0);
    }
    ASSERT_TRUE(mem.full());
}

TEST(MemtableTest, UpdateDoesNotIncrementSizeWhenFull) {
    Memtable mem;

    for (int i = 0; i < MEMTABLE_DEF_SZ; i++) {
        ASSERT_EQ(mem.put((Key)i, (Val)i), 0);
    }
    ASSERT_TRUE(mem.full());

    ASSERT_EQ(mem.put(1, 999), 0);
    auto val = mem.get(1);
    ASSERT_TRUE(val.has_value());
    ASSERT_EQ(val.value(), 999);
}

TEST(MemtableTest, RejectNewKeyWhenFull) {
    Memtable mem;

    for (int i = 0; i < MEMTABLE_DEF_SZ; i++) {
        ASSERT_EQ(mem.put((Key)i, (Val)i), 0);
    }

    int result = mem.put((Key)MEMTABLE_DEF_SZ + 1, 30);
    ASSERT_EQ(result, -1);
}

TEST(MemtableTest, SortedOrder) {
    Memtable mem;

    mem.put(400, 4);
    mem.put(100, 1);
    mem.put(300, 3);
    mem.put(200, 2);

    auto data = mem.flush();
    ASSERT_EQ(data.size(), 4u);
    ASSERT_EQ(data[0].first, 100);
    ASSERT_EQ(data[1].first, 200);
    ASSERT_EQ(data[2].first, 300);
    ASSERT_EQ(data[3].first, 400);
}

TEST(MemtableTest, ScanRange) {
    Memtable mem;

    mem.put(10, 1);
    mem.put(20, 2);
    mem.put(30, 3);
    mem.put(40, 4);
    mem.put(50, 5);

    auto results = mem.scan(20, 40);
    ASSERT_EQ(results.size(), 3u);
    ASSERT_EQ(results[0].first, 20);
    ASSERT_EQ(results[1].first, 30);
    ASSERT_EQ(results[2].first, 40);
}

TEST(MemtableTest, ScanEmptyRange) {
    Memtable mem;

    mem.put(10, 1);
    mem.put(50, 5);

    auto results = mem.scan(20, 40);
    ASSERT_EQ(results.size(), 0u);
}

TEST(MemtableTest, ScanInvalidRange) {
    Memtable mem;

    auto results = mem.scan(100, 10);
    ASSERT_EQ(results.size(), 0u);
}

TEST(MemtableTest, FlushClearsMemtable) {
    Memtable mem;

    mem.put(1, 10);
    mem.put(2, 20);

    auto data = mem.flush();
    ASSERT_EQ(data.size(), 2u);
    ASSERT_FALSE(mem.full());

    auto val = mem.get(1);
    ASSERT_FALSE(val.has_value());

    mem.put(3, 30);
    ASSERT_TRUE(mem.get(3).has_value());
}

TEST(MemtableTest, LargeDataset) {
    Memtable mem;

    for (int i = 0; i < 100; i++) {
        Key key = static_cast<Key>(i);
        Val val = static_cast<Val>(i * 1000 + 7);
        mem.put(key, val);
    }

    for (int i = 0; i < 100; i++) {
        Key key = static_cast<Key>(i);
        auto val = mem.get(key);
        ASSERT_TRUE(val.has_value());
    }

    auto data = mem.flush();
    ASSERT_EQ(data.size(), 100u);
    for (size_t i = 1; i < data.size(); i++) {
        ASSERT_LT(data[i - 1].first, data[i].first);
    }
}
