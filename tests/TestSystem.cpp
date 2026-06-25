#include <filesystem>
#include <gtest/gtest.h>

#include "../src/database/Basalt.hpp"
#include "../src/Config.hpp"

TEST(SystemTest, MultiFlushReadback) {
    std::filesystem::remove_all("/tmp/basalt_sys_readback");
    Basalt db("/tmp/basalt_sys_readback");
    int n = MEMTABLE_DEF_SZ * 3 + 1;
    for (int i = 0; i < n; i++)
        db.put((Key)i, (Val)(i * 3));
    for (int i = 0; i < n; i++) {
        auto val = db.get(i);
        ASSERT_TRUE(val.has_value());
        ASSERT_EQ(*val, (Val)(i * 3));
    }
    std::filesystem::remove_all("/tmp/basalt_sys_readback");
}

TEST(SystemTest, CompactionProducesNextLevel) {
    std::filesystem::remove_all("/tmp/basalt_sys_compact");
    Basalt db("/tmp/basalt_sys_compact");
    for (int i = 0; i < MEMTABLE_DEF_SZ * 2 + 1; i++)
        db.put((Key)i, (Val)(i * 2));
    ASSERT_FALSE(std::filesystem::exists("/tmp/basalt_sys_compact/sstables/SST0"));
    ASSERT_TRUE(std::filesystem::exists("/tmp/basalt_sys_compact/sstables/SST1"));
    std::filesystem::remove_all("/tmp/basalt_sys_compact");
}

TEST(SystemTest, LatestVersionWinsAfterCompaction) {
    std::filesystem::remove_all("/tmp/basalt_sys_latest");
    Basalt db("/tmp/basalt_sys_latest");
    for (int i = 0; i < MEMTABLE_DEF_SZ; i++)
        db.put((Key)i, 1);
    db.put((Key)MEMTABLE_DEF_SZ, 1);
    db.put(0, 999);
    for (int i = MEMTABLE_DEF_SZ + 1; i < MEMTABLE_DEF_SZ * 2; i++)
        db.put((Key)i, 1);
    db.put((Key)(MEMTABLE_DEF_SZ * 2), 1);
    auto val = db.get(0);
    ASSERT_TRUE(val.has_value());
    ASSERT_EQ(*val, 999);
    std::filesystem::remove_all("/tmp/basalt_sys_latest");
}

TEST(SystemTest, ScanAcrossSSTAndMemtable) {
    std::filesystem::remove_all("/tmp/basalt_sys_scan_mix");
    Basalt db("/tmp/basalt_sys_scan_mix");
    for (int i = 0; i < MEMTABLE_DEF_SZ * 2; i++)
        db.put((Key)i, (Val)(i + 10));
    for (int i = MEMTABLE_DEF_SZ * 2; i < MEMTABLE_DEF_SZ * 2 + 20; i++)
        db.put((Key)i, (Val)(i + 10));
    auto result = db.scan(MEMTABLE_DEF_SZ * 2 - 5, MEMTABLE_DEF_SZ * 2 + 5);
    ASSERT_EQ(result.size(), 11u);
    for (int i = 0; i < 11; i++) {
        Key key = (Key)(MEMTABLE_DEF_SZ * 2 - 5 + i);
        ASSERT_EQ(result[i].first, key);
        ASSERT_EQ(result[i].second, (Val)(key + 10));
    }
    std::filesystem::remove_all("/tmp/basalt_sys_scan_mix");
}

TEST(SystemTest, DeletePersistsAcrossFlushes) {
    std::filesystem::remove_all("/tmp/basalt_sys_delete");
    Basalt db("/tmp/basalt_sys_delete");
    for (int i = 0; i < MEMTABLE_DEF_SZ + 1; i++)
        db.put((Key)i, (Val)(i * 4));
    db.del(42);
    for (int i = MEMTABLE_DEF_SZ + 1; i < MEMTABLE_DEF_SZ * 2 + 2; i++)
        db.put((Key)i, (Val)(i * 4));
    ASSERT_FALSE(db.get(42).has_value());
    std::filesystem::remove_all("/tmp/basalt_sys_delete");
}

TEST(SystemTest, TenLevelStress) {
    const std::string path = "/tmp/basalt_sys_10level";
    std::filesystem::remove_all(path);
    Basalt db(path);

    const int N = MEMTABLE_DEF_SZ * 1024 + 1;
    for (int i = 0; i < N; i++)
        db.put((Key)i, (Val)(i * 2));

    ASSERT_TRUE(std::filesystem::exists(path + "/sstables/SST10"));
    for (int i = 0; i < N; i += 10000)
        db.put((Key)i, (Val)(i * 3));
    for (int i = 5000; i < N; i += 10000)
        db.del((Key)i);

    for (int i = N; i < N + MEMTABLE_DEF_SZ; i++)
        db.put((Key)i, (Val)(i * 2));

    for (int i = 0; i < N; i += 1000) {
        auto val = db.get((Key)i);
        if (i % 10000 == 0) {
            ASSERT_TRUE(val.has_value()) << "updated key " << i << " missing";
            ASSERT_EQ(*val, (Val)(i * 3)) << "updated key " << i << " wrong value";
        } else if (i % 10000 == 5000) {
            ASSERT_FALSE(val.has_value()) << "deleted key " << i << " should be absent";
        } else {
            ASSERT_TRUE(val.has_value()) << "key " << i << " missing";
            ASSERT_EQ(*val, (Val)(i * 2)) << "key " << i << " wrong value";
        }
    }

    const Key scan_start = 500000;
    const Key scan_end   = 500200;
    auto result = db.scan(scan_start, scan_end);

    ASSERT_FALSE(result.empty());
    for (int i = 1; i < (int)result.size(); i++)
        ASSERT_LT(result[i - 1].first, result[i].first) << "scan not sorted at index " << i;

    for (auto& [k, v] : result) {
        ASSERT_GE(k, scan_start);
        ASSERT_LE(k, scan_end);
        if (k % 10000 == 0)
            ASSERT_EQ(v, (Val)(k * 3));
        else
            ASSERT_EQ(v, (Val)(k * 2));
    }

    for (auto& [k, v] : result)
        ASSERT_NE(k % 10000, 5000) << "deleted key " << k << " appeared in scan";

    std::filesystem::remove_all(path);
}
