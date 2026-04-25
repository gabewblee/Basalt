#include <cstring>
#include <gtest/gtest.h>

#include "../src/buf/ExtendibleBufferPool.hpp"
#include "../src/buf/LRU.hpp"

TEST(LRU, EvictWhenEmpty) {
    LRU lru;
    ASSERT_EQ(lru.evict(), nullptr);
}

TEST(LRU, PutOneEvictReturnsIt) {
    LRU lru;
    Frame frame;
    frame.pid = "p7";
    frame.next = nullptr;
    frame.prev = nullptr;
    lru.put(&frame);
    ASSERT_EQ(lru.evict(), &frame);
}

TEST(LRU, TwoPutsEvictDropsOlder) {
    LRU lru;
    Frame first;
    first.pid = "p1";
    first.next = nullptr;
    first.prev = nullptr;

    Frame second;
    second.pid = "p2";
    second.next = nullptr;
    second.prev = nullptr;

    lru.put(&first);
    lru.put(&second);
    ASSERT_EQ(lru.evict(), &first);
    ASSERT_EQ(lru.evict(), &second);
    ASSERT_EQ(lru.evict(), nullptr);
}

TEST(LRU, TouchMovesToMostRecent) {
    LRU lru;
    Frame alpha;
    alpha.pid = "p10";
    alpha.next = nullptr;
    alpha.prev = nullptr;

    Frame beta;
    beta.pid = "p20";
    beta.next = nullptr;
    beta.prev = nullptr;

    lru.put(&alpha);
    lru.put(&beta);
    lru.touch(&alpha);
    ASSERT_EQ(lru.evict(), &beta);
    ASSERT_EQ(lru.evict(), &alpha);
}

TEST(LRU, DeleteMiddleThenEvictRest) {
    LRU lru;
    Frame x;
    x.pid = "p3";
    x.next = nullptr;
    x.prev = nullptr;

    Frame y;
    y.pid = "p4";
    y.next = nullptr;
    y.prev = nullptr;

    lru.put(&x);
    lru.put(&y);
    lru.del(&x);
    ASSERT_EQ(lru.evict(), &y);
}

TEST(ExtendibleBufferPoolTest, PutReturnsReadablePage) {
    ExtendibleBufferPool pool;

    uint8_t payload[PG_DEF_SZ];
    std::memset(payload, 0x5A, PG_DEF_SZ);
    payload[0] = 0x12;

    Frame* frame = pool.put("pid:100", payload);
    ASSERT_NE(frame, nullptr);
    ASSERT_EQ(frame->pid, "pid:100");
    ASSERT_EQ(frame->data[0], 0x12);
    ASSERT_EQ(frame->data[PG_DEF_SZ - 1], 0x5A);
}

TEST(ExtendibleBufferPoolTest, PutSamePidOverwrites) {
    ExtendibleBufferPool pool;

    uint8_t first[PG_DEF_SZ];
    uint8_t second[PG_DEF_SZ];
    std::memset(first, 0x11, PG_DEF_SZ);
    std::memset(second, 0xEE, PG_DEF_SZ);

    Frame* a = pool.put("pid:5", first);
    Frame* b = pool.put("pid:5", second);

    ASSERT_EQ(a, b);
    ASSERT_EQ(b->data[0], 0xEE);
    ASSERT_EQ(b->data[PG_DEF_SZ - 1], 0xEE);
}

TEST(ExtendibleBufferPoolTest, DeleteUnlinksPage) {
    ExtendibleBufferPool pool;

    uint8_t payload[PG_DEF_SZ];
    std::memset(payload, 0x44, PG_DEF_SZ);
    pool.put("pid:42", payload);

    pool.del("pid:42");
    ASSERT_FALSE(pool.get("pid:42").has_value());

    uint8_t again[PG_DEF_SZ];
    std::memset(again, 0x55, PG_DEF_SZ);
    Frame* fresh = pool.put("pid:42", again);

    ASSERT_NE(fresh, nullptr);
    ASSERT_EQ(fresh->data[0], 0x55);
}

TEST(ExtendibleBufferPoolTest, SeveralDistinctPages) {
    ExtendibleBufferPool pool;

    uint8_t p1[PG_DEF_SZ];
    uint8_t p2[PG_DEF_SZ];
    uint8_t p3[PG_DEF_SZ];
    std::memset(p1, 1, PG_DEF_SZ);
    std::memset(p2, 2, PG_DEF_SZ);
    std::memset(p3, 3, PG_DEF_SZ);

    Frame* a = pool.put("pid:1", p1);
    Frame* b = pool.put("pid:2", p2);
    Frame* c = pool.put("pid:3", p3);

    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    ASSERT_NE(c, nullptr);
    ASSERT_NE(a, b);
    ASSERT_NE(a, c);
    ASSERT_NE(b, c);
}
