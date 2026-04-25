#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

#include "Memtable.hpp"

#include "../Config.hpp"

static void inorder(Node* node, Key start, Key end, std::vector<std::pair<Key, Val>>& nodes) {
    if (node) {
        if (node->key > start) 
            inorder(node->left, start, end, nodes);

        if (node->key >= start && node->key <= end) 
            nodes.emplace_back(node->key, node->val);

        if (node->key < end) 
            inorder(node->right, start, end, nodes);
    }
}

static void inorder_sst(Node* node, std::vector<std::pair<Key, Val>>& nodes) {
    if (node) {
        inorder_sst(node->left, nodes);
        nodes.emplace_back(node->key, node->val);
        inorder_sst(node->right, nodes);
    }
}

Memtable::Memtable() : memtable_sz(MEMTABLE_DEF_SZ), curr_sz(0), root(nullptr) {}

Memtable::~Memtable() {
    cleanup(root);
}

bool Memtable::full() const {
    return curr_sz >= memtable_sz;
}

std::optional<Val> Memtable::get(const Key& key) const {
    Node* curr = root;
    while (curr) {
        if (key == curr->key)
            return curr->val;
        else if (key < curr->key)
            curr = curr->left;
        else
            curr = curr->right;
    }
    return std::nullopt;
}

int Memtable::put(const Key& key, const Val& val) {
    bool exists = get(key).has_value();
    if (full() && !exists)
        return -1;
    
    root = insert(root, key, val);
    if (!exists)
        curr_sz++;
    
    return 0;
}

std::vector<std::pair<Key, Val>> Memtable::scan(const Key start, const Key end) const {
    if (start > end) 
        return {};

    std::vector<std::pair<Key, Val>> nodes;
    inorder(root, start, end, nodes);
    return nodes;
}

std::vector<std::pair<Key, Val>> Memtable::flush() {
    std::vector<std::pair<Key, Val>> nodes;
    inorder_sst(root, nodes);
    cleanup(root);
    curr_sz = 0;
    root = nullptr;
    return nodes;
}

int Memtable::height(const Node* node) const {
    return (node == nullptr) ? 0 : node->height;
}

Node* Memtable::rotate_right(Node* node) {
    Node* new_root = node->left;
    Node* temp = new_root->right;

    new_root->right = node;
    node->left = temp;

    node->height = std::max(height(node->left), height(node->right)) + 1;
    new_root->height = std::max(height(new_root->left), height(new_root->right)) + 1;

    return new_root;
}

Node* Memtable::rotate_left(Node* node) {
    Node* new_root = node->right;
    Node* temp = new_root->left;

    new_root->left = node;
    node->right = temp;

    node->height = std::max(height(node->left), height(node->right)) + 1;
    new_root->height = std::max(height(new_root->left), height(new_root->right)) + 1;

    return new_root;
}

int Memtable::balance(const Node* node) const {
    return (node == nullptr) ? 0 : height(node->left) - height(node->right);
}

Node* Memtable::insert(Node* node, const Key& key, const Val& val) {
    if (node == nullptr)
        return new Node(key, val);

    if (key < node->key)
        node->left = insert(node->left, key, val);
    else if (key > node->key)
        node->right = insert(node->right, key, val);
    else {
        node->val = val;
        return node;
    }

    node->height = std::max(height(node->left), height(node->right)) + 1;
    int bal = balance(node);

    if (bal > 1 && key < node->left->key)
        return rotate_right(node);
    
    if (bal < -1 && key > node->right->key)
        return rotate_left(node);   

    if (bal > 1 && key > node->left->key) {
        node->left = rotate_left(node->left);
        return rotate_right(node);
    }

    if (bal < -1 && key < node->right->key) {
        node->right = rotate_right(node->right);
        return rotate_left(node);
    }

    return node;
}

void Memtable::cleanup(Node* node) {
    if (node) {
        cleanup(node->left);
        cleanup(node->right);
        delete node;
    }
}