#pragma once

#include <optional>
#include <vector>

#include "Config.hpp"
#include "IMemtable.hpp"
#include "Types.hpp"

struct Node {
    Key   key;    /* Node key    */
    Val   val;    /* Node value  */
    Node* left;   /* Left child  */
    Node* right;  /* Right child */
    int   height; /* Tree height */

    /**
     * Node - Constructor for the Node struct.
     * @key: The node key.
     * @val: The node value.
     */
    Node(Key key, Val val) : key(key), val(val), left(nullptr), right(nullptr), height(1) {}
};

class Memtable : public IMemtable {
public:
    /**
     * Memtable - Constructor for the Memtable class.
     */
    Memtable();

    /**
     * ~Memtable - Destructor for the Memtable class.
     */
    ~Memtable() override;

    /**
     * full - Check if the Memtable is full.
     * Returns: True if full, false otherwise.
     */
    bool full() const override;

    /**
     * get - Retrieve the value associated with a key.
     * @key: The key to search for.
     * Returns: The associated value, or std::nullopt if not present.
     */
    std::optional<Val> get(const Key& key) const override;

    /**
     * put - Insert/update a key-value pair.
     * @key: The key to insert/update.
     * @val: The value to insert/update.
     * Returns: 0 on success, -1 if inserting into a full Memtable.
     */
    int put(const Key& key, const Val& val) override;

    /**
     * scan - Retrieve all key-value pairs whose keys fall within [start, end].
     * @start: The start of the key range.
     * @end: The end of the key range.
     * Returns: A sorted vector of key-value pairs within the specified range.
     */
    std::vector<std::pair<Key, Val>> scan(const Key start, const Key end) const override;

    /**
     * flush - Flush all entries from the IMemtable.
     * Returns: A sorted vector of key-value pairs that were flushed.
     */
    std::vector<std::pair<Key, Val>> flush() override;

private:
    int   memtable_sz; /* Maximum number of entries */
    int   curr_sz;     /* Current number of entries */
    Node* root;        /* Root of the AVL tree      */

    /**
     * height - Return the height of a node.
     * @node: The input root.
     * Returns: The height of the node.
     */
    int height(const Node* node) const;

    /**
     * rotate_right - Perform a right rotation around node and return the new root.
     * @node: The input root.
     * Returns: The new root after rotation.
     */
    Node* rotate_right(Node* node);

    /**
     * rotate_left - Perform a left rotation around node and return the new root.
     * @node: The input root.
     * Returns: The new root after rotation.
     */
    Node* rotate_left(Node* node);

    /**
     * balance - Return the balance factor of a node.
     * @node: The input root.
     * Returns: Positive if left-heavy, negative if right-heavy, zero if balanced.
     */
    int balance(const Node* node) const;

    /**
     * insert - Recursively insert/update a key-value pair in the tree.
     * @node: The input root.
     * @key: The key to insert/update.
     * @val: The value to insert/update.
     * Returns: The new root after insertion/update.
     */
    Node* insert(Node* node, const Key& key, const Val& val);

    /**
     * cleanup - Recursively free all nodes in the tree.
     * @node: The input root.
     */
    void cleanup(Node* node);
};
