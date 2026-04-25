#include <algorithm>

#include "BPlusTree.hpp"

std::vector<BTreeNode> BPlusTree::build(std::vector<std::pair<Key, Val>> pairs) const {
    if (pairs.empty())
        return {};

    std::vector<BTreeNode> btree, cur_lvl;

    size_t i = 0;
    while (i < pairs.size()) {
        BTreeNode leaf(true);
        while (i < pairs.size() && leaf.cnt < BTREE_NODE_DEF_SZ) {
            leaf.keys[leaf.cnt] = pairs[i].first;
            leaf.vals[leaf.cnt] = pairs[i].second;
            leaf.cnt++;
            i++;
        }
        cur_lvl.push_back(leaf);
    }
    
    btree.insert(btree.end(), cur_lvl.begin(), cur_lvl.end());
    while (cur_lvl.size() > 1) {
        std::vector<BTreeNode> nxt_lvl;
        
        size_t j = 0;
        while (j < cur_lvl.size()) {
            BTreeNode internal(false);
            while (j < cur_lvl.size() && internal.cnt < BTREE_NODE_DEF_SZ) {
                internal.keys[internal.cnt] = cur_lvl[j].keys[0];

                size_t child_offset = btree.size() - cur_lvl.size() + j;
                internal.vals[internal.cnt] = child_offset;
                internal.cnt++;
                j++;
            }
            
            nxt_lvl.push_back(internal);
        }
        
        btree.insert(btree.end(), nxt_lvl.begin(), nxt_lvl.end());
        cur_lvl = nxt_lvl;
    }
    
    return btree;
}