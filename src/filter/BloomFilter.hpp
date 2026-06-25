#pragma once

#include <cmath>
#include <cstdint>
#include <fstream>
#include <utility>
#include <vector>

#include "IFilter.hpp"

#include "../Config.hpp"
#include "../Types.hpp"

class BloomFilter : public IFilter {
public:
    /**
     * init - Size the filter for num_entries and zero the bitmap.
     * @num_entries: The expected number of keys to insert.
     */
    void init(int num_entries) override;

    /**
     * clear - Clear the filter.
     */
    void clear() override;

    /**
     * fill - Fill the filter with the given entries.
     * @entries: The flushed Memtable entries.
     * Throws std::invalid_argument if the entries are empty.
     */
    void fill(const std::vector<std::pair<Key, Val>>& entries) override;

    /**
     * insert - Insert a single key into an already-initialized filter.
     * @key: The key to insert.
     */
    void insert(Key key) override;

    /**
     * contains - Check whether the key may be present.
     * @key: The key to probe.
     * Returns: True if key may be present, false otherwise.
     */
    bool contains(Key key) const override;

    /**
     * get_nbits - Get the number of bits in the filter.
     * Returns: The number of bits in the filter.
     */
    int get_nbits() const override;

    /**
     * get_nhashes - Get the number of hash functions used by the filter.
     * Returns: The number of hash functions used by the filter.
     */
    int get_nhashes() const override;

    /**
     * get_filter - Get the filter bitmap.
     * Returns: The filter bitmap.
     */
    std::vector<uint8_t> get_filter() const override;

    /**
     * set_nbits - Set the number of bits in the filter.
     * @nbits: The number of bits in the filter.
     */
    void set_nbits(int nbits) override;

    /**
     * set_nhashes - Set the number of hash functions used by the filter.
     * @nhashes: The number of hash functions.
     */
    void set_nhashes(int nhashes) override;

    /**
     * set_filter - Set the filter bitmap.
     * @filter: The filter bitmap.
     */
    void set_filter(const std::vector<uint8_t>& filter) override;

private:
    int                  nhashes = 0; /* Number of hash functions           */
    int                  nbits = 0;   /* Number of bits in the Bloom Filter */
    std::vector<uint8_t> filter;      /* Bit vector backing the filter      */

    /**
     * hash - Hash the key with the given seed.
     * @key: The key to hash.
     * @seed: The seed passed to XXH3_64bits_withSeed.
     * Returns: The 64-bit hash value.
     */
    uint64_t hash(Key key, uint64_t seed) const;

    /**
     * nth - Get the bit index for the i-th hash function.
     * @h1: The first hash of the key.
     * @h2: The second hash of the key.
     * @i: The probe index in [0, nhashes).
     * Returns: The bit index in [0, nbits).
     */
    int nth(uint64_t h1, uint64_t h2, int i) const;

    /**
     * set - Set the bit at index i.
     * @i: The linear bit index.
     */
    void set(int i);

    /**
     * get - Read the bit at index i.
     * @i: The linear bit index.
     * Returns: True if the bit is set.
     */
    bool get(int i) const;
};
