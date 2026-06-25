#pragma once

#include <fstream>
#include <vector>

#include "../Types.hpp"

class IFilter {
public:
    /**
     * init - Size the filter for num_entries and zero the bitmap.
     * @num_entries: The expected number of keys to insert.
     */
    virtual void init(int num_entries) = 0;

    /**
     * clear - Clear the filter.
     */
    virtual void clear() = 0;

    /**
     * fill - Fill the filter with the given entries.
     * @entries: The flushed Memtable entries.
     * Throws std::invalid_argument if the entries are empty.
     */
    virtual void fill(const std::vector<std::pair<Key, Val>>& entries) = 0;

    /**
     * insert - Insert a single key into an already-initialized filter.
     * @key: The key to insert.
     */
    virtual void insert(Key key) = 0;

    /**
     * contains - Check whether the key may be present.
     * @key: The key to probe.
     * Returns: True if key may be present, false otherwise.
     */
    virtual bool contains(Key key) const = 0;

    /**
     * get_nbits - Get the number of bits in the filter.
     * Returns: The number of bits in the filter.
     */
    virtual int get_nbits() const = 0;

    /**
     * get_nhashes - Get the number of hash functions used by the filter.
     * Returns: The number of hash functions used by the filter.
     */
    virtual int get_nhashes() const = 0;

    /**
     * get_filter - Get the filter bitmap.
     * Returns: The filter bitmap.
     */
    virtual std::vector<uint8_t> get_filter() const = 0;

    /**
     * set_nbits - Set the number of bits in the filter.
     * @nbits: The number of bits in the filter.
     */
    virtual void set_nbits(int nbits) = 0;

    /**
     * set_nhashes - Set the number of hash functions used by the filter.
     * @nhashes: The number of hash functions.
     */
    virtual void set_nhashes(int nhashes) = 0;

    /**
     * set_filter - Set the filter bitmap.
     * @filter: The filter bitmap.
     */
    virtual void set_filter(const std::vector<uint8_t>& filter) = 0;
};
