#pragma once

#include <optional>
#include <vector>

#include "Config.hpp"
#include "IMemtable.hpp"
#include "Types.hpp"

struct Node {
    Key key;      /* Node key */
    Val val;      /* Node value */
    Node* left;   /* Left child */
    Node* right;  /* Right child */
    int height;   /* Tree height */

    /**
     * Constructor for the Node struct
     * @param key Node key
     * @param val Node value
     */
    Node(Key key, Val val) : key(key), val(val), left(nullptr), right(nullptr), height(1) {}
};

class Memtable : public IMemtable {
public:
    /**
     * Constructor for the Memtable class
     */
    Memtable();

    /**
     * Destructor for the Memtable class
     */
    ~Memtable() override;

    /**
     * Check if the Memtable is full
     * @return true if full, false otherwise
     */
    bool full() const override;

    /**
     * Retrieve the value associated with a key
     * @param key The key to search for
     * @return The associated value, or std::nullopt if not present
     */
    std::optional<Val> get(const Key& key) const override;

    /**
     * Insert/update a key-value pair
     * @param key The key to insert/update
     * @param val The value to insert/update
     * @return 0 on success, -1 if inserting into a full Memtable
     */
    int put(const Key& key, const Val& val) override;

    /**
     * Retrieve all key-value pairs whose keys fall within [start, end]
     * @param start The start of the key range
     * @param end The end of the key range
     * @return Sorted vector of key-value pairs within the specified range
     */
    std::vector<std::pair<Key, Val>> scan(const Key start, const Key end) const override;

    /**
     * Flush all entries from the IMemtable
     * @return Sorted vector of key-value pairs that were flushed
     */
    std::vector<std::pair<Key, Val>> flush() override;

private:
    int memtable_sz; /* Maximum number of entries */
    int curr_sz;     /* Current number of entries */
    Node* root;      /* Root of the AVL tree */

    /**
     * Return the height of a node
     * @param node Input root
     * @return Height of the node
     */
    int height(const Node* node) const;

    /**
     * Perform a right rotation around node and return the new root
     * @param node Input root
     * @return New root after rotation
     */
    Node* rotate_right(Node* node);

    /**
     * Perform a left rotation around node and return the new root
     * @param node Input root
     * @return New root after rotation
     */
    Node* rotate_left(Node* node);

    /**
     * Return the balance factor of a node
     * @param node Input root
     * @return Positive if left-heavy, negative if right-heavy, zero if balanced
     */
    int balance(const Node* node) const;

    /**
     * Recursively insert/update a key-value pair in the tree
     * @param node Input root
     * @param key Key to insert/update
     * @param val Value to insert/update
     * @return New root after insertion/update
     */
    Node* insert(Node* node, const Key& key, const Val& val);

    /**
     * Recursively free all nodes in the tree
     * @param node Input root
     */
    void cleanup(Node* node);
};
