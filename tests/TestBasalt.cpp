#include <filesystem>
#include <gtest/gtest.h>

#include "../src/database/Basalt.hpp"
#include "../src/Config.hpp"

TEST(BasaltTest, PutGet) {
    std::filesystem::remove_all("/tmp/basalt_test_put_get");
    Basalt db("/tmp/basalt_test_put_get");
    ASSERT_EQ(db.put(1, 100), 0);
    auto val = db.get(1);
    ASSERT_TRUE(val.has_value());
    ASSERT_EQ(*val, 100);
    std::filesystem::remove_all("/tmp/basalt_test_put_get");
}

TEST(BasaltTest, UpdateWins) {
    std::filesystem::remove_all("/tmp/basalt_test_update");
    Basalt db("/tmp/basalt_test_update");
    db.put(7, 70);
    db.put(7, 77);
    auto val = db.get(7);
    ASSERT_TRUE(val.has_value());
    ASSERT_EQ(*val, 77);
    std::filesystem::remove_all("/tmp/basalt_test_update");
}

TEST(BasaltTest, DeleteHidesKey) {
    std::filesystem::remove_all("/tmp/basalt_test_delete");
    Basalt db("/tmp/basalt_test_delete");
    db.put(9, 90);
    db.del(9);
    ASSERT_FALSE(db.get(9).has_value());
    std::filesystem::remove_all("/tmp/basalt_test_delete");
}

TEST(BasaltTest, ScanRange) {
    std::filesystem::remove_all("/tmp/basalt_test_scan");
    Basalt db("/tmp/basalt_test_scan");
    for (int i = 1; i <= 5; i++)
        db.put(i, i * 10);
    auto result = db.scan(2, 4);
    ASSERT_EQ(result.size(), 3u);
    ASSERT_EQ(result[0].first, 2);
    ASSERT_EQ(result[1].first, 3);
    ASSERT_EQ(result[2].first, 4);
    std::filesystem::remove_all("/tmp/basalt_test_scan");
}

TEST(BasaltTest, FlushRetainsReads) {
    std::filesystem::remove_all("/tmp/basalt_test_flush");
    Basalt db("/tmp/basalt_test_flush");
    for (int i = 0; i < MEMTABLE_DEF_SZ + 1; i++)
        db.put((Key)i, (Val)(i * 5));
    for (int i = 0; i < MEMTABLE_DEF_SZ + 1; i++) {
        auto val = db.get(i);
        ASSERT_TRUE(val.has_value());
        ASSERT_EQ(*val, (Val)(i * 5));
    }
    std::filesystem::remove_all("/tmp/basalt_test_flush");
}

TEST(BasaltTest, ScanSkipsTombstones) {
    std::filesystem::remove_all("/tmp/basalt_test_tomb");
    Basalt db("/tmp/basalt_test_tomb");
    db.put(1, 10);
    db.put(2, 20);
    db.put(3, 30);
    db.del(2);
    auto result = db.scan(1, 3);
    ASSERT_EQ(result.size(), 2u);
    ASSERT_EQ(result[0].first, 1);
    ASSERT_EQ(result[1].first, 3);
    std::filesystem::remove_all("/tmp/basalt_test_tomb");
}
