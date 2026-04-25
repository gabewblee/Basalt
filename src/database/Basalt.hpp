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
     * Constructor for the Basalt class
     */
    Basalt(const std::string& name);

    /**
     * Destructor for the Basalt class
     */
    ~Basalt();

    /**
     * Retrieve a val associated with a key
     * @param key The key to search for
     * @return The val if found, std::nullopt otherwise
     */
    std::optional<Val> get(const Key& key) override;

    /**
     * Insert/update a key-val pair in the database
     * @param key The key to insert/update
     * @param val The val associated with the key
     * @return 0 on success, -1 on failure
     */
    int put(const Key& key, const Val& val) override;

    /**
     * Delete a key-val pair from the database
     * @param key The key to delete
     * @return 0 on success, -1 on failure
     */
    int del(const Key& key) override;

    /**
     * Retrieve all key-val pairs whose keys fall within [start, end]
     * @param start Start of the key range
     * @param end End of the key range
     * @return Sorted vector of non-deleted key-val pairs in the range
     */
    std::vector<std::pair<Key, Val>> scan(const Key& start, const Key& end) override;

private:
    std::string name;       /* Database name */
    IBufferPool* buf;       /* Buffer pool */
    IMemtable* memtable;    /* In-memory table */
    IBTree* btree;          /* B-Tree builder */
};