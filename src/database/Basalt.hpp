#pragma once

#include "../buf/IBufferPool.hpp"
#include "../database/IDatabase.hpp"
#include "../filter/BloomFilter.hpp"
#include "../filter/IFilter.hpp"
#include "../memtable/IMemtable.hpp"
#include "../storage/IBTree.hpp"

class Basalt : public IDatabase {
public:
    /**
     * Basalt - Constructor for the Basalt class.
     * @name: The database name.
     */
    Basalt(const std::string& name);

    /**
     * ~Basalt - Destructor for the Basalt class.
     */
    ~Basalt();

    /**
     * get - Retrieve a value associated with @key.
     * @key: The key to search for.
     * Returns: The value if found, std::nullopt otherwise.
     */
    std::optional<Val> get(const Key& key) override;

    /**
     * put - Insert/update a key-value pair in the database.
     * @key: The key to insert/update.
     * @val: The value associated with the key.
     * Returns: 0 on success, -1 on failure.
     */
    int put(const Key& key, const Val& val) override;

    /**
     * del - Delete a key-value pair from the database.
     * @key: The key to delete.
     * Returns: 0 on success, -1 on failure.
     */
    int del(const Key& key) override;

    /**
     * scan - Retrieve all key-value pairs whose keys fall within [start, end].
     * @start: The start of the key range.
     * @end: The end of the key range.
     * Returns: A sorted vector of non-deleted key-value pairs in the range.
     */
    std::vector<std::pair<Key, Val>> scan(const Key& start, const Key& end) override;

private:
    std::string  name;     /* Database name   */
    IBufferPool* buf;      /* Buffer pool     */
    IMemtable*   memtable; /* In-memory table */
    IBTree*      btree;    /* B-Tree builder  */
};
