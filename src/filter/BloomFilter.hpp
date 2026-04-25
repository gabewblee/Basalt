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
     * Size the filter for num_entries and zero the bitmap
     * @param num_entries Expected number of keys to insert
     */
    void init(int num_entries) override;

    /**
     * Clear the filter
     */
    void clear() override;

    /**
     * Fill the filter with the given entries
     * @param entries Flushed Memtable entries
     * @throws std::invalid_argument if the entries are empty
     */
    void fill(const std::vector<std::pair<Key, Val>>& entries) override;

    /**
     * Insert a single key into an already-initialized filter
     * @param key The key to insert
     */
    void insert(Key key) override;

    /**
     * Check whether the key may be present
     * @param key The key to probe
     * @returns True if key may be present, false otherwise
     */
    bool contains(Key key) const override;

    /**
     * Get the number of bits in the filter
     * @returns The number of bits in the filter
     */
    int get_nbits() const override;

    /**
     * Get the number of hash functions used by the filter
     * @returns The number of hash functions used by the filter
     */
    int get_nhashes() const override;

    /**
     * Get the filter bitmap
     * @returns The filter bitmap
     */
    std::vector<uint8_t> get_filter() const override;

    /**
     * Set the number of bits in the filter
     * @param nbits The number of bits in the filter
     */
    void set_nbits(int nbits) override;

    /**
     * Set the number of hash functions used by the filter
     * @param nhashes The number of hash functions
     */
    void set_nhashes(int nhashes) override;

    /**
     * Set the filter bitmap
     * @param filter The filter bitmap
     */
    void set_filter(const std::vector<uint8_t>& filter) override;

private:
    int nhashes = 0;              /* Number of hash functions */
    int nbits = 0;                /* Number of bits in the Bloom Filter */
    std::vector<uint8_t> filter;  /* Bit vector backing the filter */

    /**
     * Hash the key with the given seed
     * @param key The key to hash
     * @param seed The seed passed to XXH3_64bits_withSeed
     * @returns The 64-bit hash val
     */
    uint64_t hash(Key key, uint64_t seed) const;

    /**
     * Get the bit index for the i-th hash function
     * @param h1 First hash of the key
     * @param h2 Second hash of the key
     * @param i Probe index in [0, nhashes)
     * @returns Bit index in [0, nbits)
     */
    int nth(uint64_t h1, uint64_t h2, int i) const;

    /**
     * Set the bit at index i
     * @param i Linear bit index
     */
    void set(int i);

    /**
     * Read the bit at index i
     * @param i Linear bit index
     * @returns True if the bit is set
     */
    bool get(int i) const;
};
