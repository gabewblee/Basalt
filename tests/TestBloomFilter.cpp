#include <gtest/gtest.h>
#include <vector>

#include "../src/filter/BloomFilter.hpp"

TEST(BloomFilterTest, FillEmptyThrows) {
    BloomFilter filter;
    std::vector<std::pair<Key, Val>> empty;
    EXPECT_THROW(filter.fill(empty), std::invalid_argument);
}

TEST(BloomFilterTest, UnfilledContainsNothing) {
    BloomFilter filter;
    EXPECT_FALSE(filter.contains(0));
    EXPECT_FALSE(filter.contains(42));
}

TEST(BloomFilterTest, SingleKey) {
    BloomFilter filter;
    filter.fill({{7, 700}});
    EXPECT_TRUE(filter.contains(7));
}

TEST(BloomFilterTest, ManyKeysPresent) {
    std::vector<std::pair<Key, Val>> rows;
    for (Key i = 0; i < 200; ++i)
        rows.push_back({i, i * 10});
    BloomFilter filter;
    filter.fill(rows);
    for (Key i = 0; i < 200; ++i)
        EXPECT_TRUE(filter.contains(i)) << "key " << i;
}

TEST(BloomFilterTest, TrueNegativeFarKey) {
    std::vector<std::pair<Key, Val>> rows;
    for (Key i = 0; i < 400; ++i)
        rows.push_back({i, i});
    BloomFilter filter;
    filter.fill(rows);
    EXPECT_FALSE(filter.contains(static_cast<Key>(8'000'000)));
}

TEST(BloomFilterTest, CopyByMetadataRoundTrip) {
    BloomFilter first;
    first.fill({{1, 10}, {2, 20}, {3, 30}});

    BloomFilter second;
    second.set_nbits(first.get_nbits());
    second.set_nhashes(first.get_nhashes());
    second.set_filter(first.get_filter());

    EXPECT_TRUE(second.contains(1));
    EXPECT_TRUE(second.contains(2));
    EXPECT_TRUE(second.contains(3));
}

TEST(BloomFilterTest, ClearResetsMembership) {
    BloomFilter filter;
    filter.fill({{11, 1}, {22, 2}});
    ASSERT_TRUE(filter.contains(11));

    filter.clear();
    EXPECT_FALSE(filter.contains(11));
    EXPECT_FALSE(filter.contains(22));
}

TEST(BloomFilterTest, NegativeKeys) {
    BloomFilter filter;
    filter.fill({{-100, 1}, {-50, 2}, {-1, 3}});
    EXPECT_TRUE(filter.contains(-100));
    EXPECT_TRUE(filter.contains(-50));
    EXPECT_TRUE(filter.contains(-1));
    EXPECT_FALSE(filter.contains(0));
}

TEST(BloomFilterTest, DuplicateKeysInFill) {
    BloomFilter filter;
    filter.fill({{1, 10}, {1, 99}, {2, 20}});
    EXPECT_TRUE(filter.contains(1));
    EXPECT_TRUE(filter.contains(2));
}
