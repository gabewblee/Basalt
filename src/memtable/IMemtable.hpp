#pragma once

#include <optional>
#include <vector>

#include "../Types.hpp"

class IMemtable {
public:
    /**
     * ~IMemtable - Destructor for the IMemtable class.
     */
    virtual ~IMemtable() = default;

    /**
     * full - Check if the IMemtable is full.
     * Returns: True if full, false otherwise.
     */
    virtual bool full() const = 0;

    /**
     * get - Retrieve the value associated with a key.
     * @key: The key to search for.
     * Returns: The associated value, or std::nullopt if not present.
     */
    virtual std::optional<Val> get(const Key& key) const = 0;

    /**
     * put - Insert/update a key-value pair.
     * @key: The key to insert/update.
     * @val: The value to insert/update.
     * Returns: 0 on success, -1 if inserting into a full IMemtable.
     */
    virtual int put(const Key& key, const Val& val) = 0;

    /**
     * scan - Retrieve all key-value pairs whose keys fall within [start, end].
     * @start: The start of the key range.
     * @end: The end of the key range.
     * Returns: A sorted vector of key-value pairs within the specified range.
     */
    virtual std::vector<std::pair<Key, Val>> scan(const Key start, const Key end) const = 0;

    /**
     * flush - Flush all entries from the IMemtable.
     * Returns: A sorted vector of key-value pairs that were flushed.
     */
    virtual std::vector<std::pair<Key, Val>> flush() = 0;
};
