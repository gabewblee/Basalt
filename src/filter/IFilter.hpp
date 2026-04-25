#pragma once

#include <fstream>
#include <vector>

#include "../Types.hpp"

class IFilter {
public:
    /**
     * Size the filter for num_entries and zero the bitmap
     * @param num_entries Expected number of keys to insert
     */
    virtual void init(int num_entries) = 0;

    /**
     * Clear the filter
     */
    virtual void clear() = 0;

    /**
     * Fill the filter with the given entries
     * @param entries The flushed Memtable entries
     * @throws std::invalid_argument if the entries are empty
     */
    virtual void fill(const std::vector<std::pair<Key, Val>>& entries) = 0;

    /**
     * Insert a single key into an already-initialized filter
     * @param key The key to insert
     */
    virtual void insert(Key key) = 0;

    /**
     * Check whether the key may be present
     * @param key The key to probe
     * @returns True if key may be present, false otherwise
     */
    virtual bool contains(Key key) const = 0;

    /**
     * Get the number of bits in the filter
     * @returns The number of bits in the filter
     */
    virtual int get_nbits() const = 0;

    /**
     * Get the number of hash functions used by the filter
     * @returns The number of hash functions used by the filter
     */
    virtual int get_nhashes() const = 0;

    /**
     * Get the filter bitmap
     * @returns The filter bitmap
     */
    virtual std::vector<uint8_t> get_filter() const = 0;

    /**
     * Set the number of bits in the filter
     * @param nbits The number of bits in the filter
     */
    virtual void set_nbits(int nbits) = 0;

    /**
     * Set the number of hash functions used by the filter
     * @param nhashes The number of hash functions
     */
    virtual void set_nhashes(int nhashes) = 0;

    /**
     * Set the filter bitmap
     * @param filter The filter bitmap
     */
    virtual void set_filter(const std::vector<uint8_t>& filter) = 0;
};