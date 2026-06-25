#include "Writer.hpp"

Writer::Writer(std::ofstream& out) : out(out), leaf(true), written(0) {}

void Writer::add(Key key, Val val) {
    if (leaf.cnt == BTREE_NODE_DEF_SZ)
        seal();

    leaf.keys[leaf.cnt] = key;
    leaf.vals[leaf.cnt] = val;
    leaf.cnt++;
}

int Writer::finish() {
    seal();
    if (spine.empty())
        return 0;

    std::vector<std::pair<Key, int>> cur = spine;
    int nxt_idx = written;

    while (cur.size() > 1) {
        std::vector<std::pair<Key, int>> up;
        size_t j = 0;
        while (j < cur.size()) {
            BTreeNode internal(false);
            int idx = nxt_idx++;
            while (j < cur.size() && internal.cnt < BTREE_NODE_DEF_SZ) {
                internal.keys[internal.cnt] = cur[j].first;
                internal.vals[internal.cnt] = cur[j].second;
                internal.cnt++;
                j++;
            }
            up.push_back({internal.keys[0], idx});
            out.write(reinterpret_cast<const char*>(&internal), sizeof(BTreeNode));
        }
        cur = up;
    }
    return written;
}

void Writer::seal() {
    if (leaf.cnt == 0)
        return;

    spine.push_back({leaf.keys[0], written++});
    out.write(reinterpret_cast<const char*>(&leaf), sizeof(BTreeNode));
    leaf = BTreeNode(true);
}
