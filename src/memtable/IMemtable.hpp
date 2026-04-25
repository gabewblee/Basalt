#pragma once

#include <optional>
#include <vector>

#include "../Types.hpp"

class IMemtable {
public:
    /**
     * Destructor for the IMemtable class
     */
    virtual ~IMemtable() = default;

    /**
     * Check if the IMemtable is full
     * @return true if full, false otherwise
     */
    virtual bool full() const = 0;

    /**
     * Retrieve the value associated with a key
     * @param key The key to search for
     * @return The associated value, or std::nullopt if not present
     */
    virtual std::optional<Val> get(const Key& key) const = 0;

    /**
     * Insert/update a key-value pair
     * @param key The key to insert/update
     * @param val The value to insert/update
     * @return 0 on success, -1 if inserting into a full IMemtable
     */
    virtual int put(const Key& key, const Val& val) = 0;

    /**
     * Retrieve all key-value pairs whose keys fall within [start, end]
     * @param start The start of the key range
     * @param end The end of the key range
     * @return Sorted vector of key-value pairs within the specified range
     */
    virtual std::vector<std::pair<Key, Val>> scan(const Key start, const Key end) const = 0;

    /**
     * Flush all entries from the IMemtable
     * @return Sorted vector of key-value pairs that were flushed
     */
    virtual std::vector<std::pair<Key, Val>> flush() = 0;
};