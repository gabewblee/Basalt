#pragma once

#include <optional>
#include <vector>

#include "Types.hpp"

class IDatabase {
public:
    /**
     * ~IDatabase - Destructor for the IDatabase class.
     */
    virtual ~IDatabase() = default;

    /**
     * get - Retrieve the value associated with a key.
     * @key: The key to search for.
     * Returns: The value if found, std::nullopt otherwise.
     */
    virtual std::optional<Val> get(const Key& key) = 0;

    /**
     * put - Insert/update a key-value pair in the database.
     * @key: The key to insert/update.
     * @val: The value associated with the key.
     * Returns: 0 on success, -1 on failure.
     */
    virtual int put(const Key& key, const Val& val) = 0;

    /**
     * del - Delete a key-value pair from the database.
     * @key: The key to delete.
     * Returns: 0 on success, -1 on failure.
     */
    virtual int del(const Key& key) = 0;

    /**
     * scan - Retrieve all key-value pairs whose keys fall within [start, end].
     * @start: The start of the key range.
     * @end: The end of the key range.
     * Returns: A sorted vector of non-deleted key-value pairs in the range.
     */
    virtual std::vector<std::pair<Key, Val>> scan(const Key& start, const Key& end) = 0;
};
