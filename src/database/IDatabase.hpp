#pragma once

#include <optional>
#include <vector>

#include "Types.hpp"

class IDatabase {
public:
    /**
     * Destructor for the IDatabase class
     */
    virtual ~IDatabase() = default;

    /**
     * Retrieve a val associated with a key
     * @param key The key to search for
     * @return The val if found, std::nullopt otherwise
     */
    virtual std::optional<Val> get(const Key& key) = 0;

    /**
     * Insert/update a key-val pair in the database
     * @param key The key to insert/update
     * @param val The val associated with the key
     * @return 0 on success, -1 on failure
     */
    virtual int put(const Key& key, const Val& val) = 0;

    /**
     * Delete a key-val pair from the database
     * @param key The key to delete
     * @return 0 on success, -1 on failure
     */
    virtual int del(const Key& key) = 0;

    /**
     * Retrieve all key-val pairs whose keys fall within [start, end]
     * @param start Start of the key range
     * @param end End of the key range
     * @return Sorted vector of non-deleted key-val pairs in the range
     */
    virtual std::vector<std::pair<Key, Val>> scan(const Key& start, const Key& end) = 0;
};
