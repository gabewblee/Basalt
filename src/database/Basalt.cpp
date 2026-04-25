#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>

#include "Basalt.hpp"
#include "Config.hpp"

#include "../buf/ExtendibleBufferPool.hpp"
#include "../disk/Writer.hpp"
#include "../filter/BloomFilter.hpp"
#include "../memtable/Memtable.hpp"
#include "../storage/BPlusTree.hpp"

static void sst_write(const std::string& path, const std::vector<BTreeNode>& tree, const BloomFilter& filter) {
    std::ofstream out(path, std::ios::binary);
    if (!out)
        throw std::runtime_error("Error: failed to open file " + path + " for writing");

    int nbits = filter.get_nbits();
    int nhashes = filter.get_nhashes();
    const std::vector<uint8_t>& bitmap = filter.get_filter();
    int bitmap_sz = bitmap.size();

    int nleafs = 0;
    for (const auto& node : tree)
        if (node.leaf)
            nleafs++;

    out.write(reinterpret_cast<const char*>(&nbits), sizeof(int));
    out.write(reinterpret_cast<const char*>(&nhashes), sizeof(int));
    out.write(reinterpret_cast<const char*>(&bitmap_sz), sizeof(int));
    out.write(reinterpret_cast<const char*>(&nleafs), sizeof(int));
    out.write(reinterpret_cast<const char*>(bitmap.data()), bitmap_sz);

    for (const auto& node : tree)
        out.write(reinterpret_cast<const char*>(&node), sizeof(BTreeNode));
}

static int compact(const std::string& name, int lvl) {
    std::string insst1 = name + "/sstables/SST" + std::to_string(lvl);
    std::string insst2 = insst1 + ".tmp";
    std::string outsst1 = name + "/sstables/SST" + std::to_string(lvl + 1);
    std::string outsst2 = std::filesystem::exists(outsst1) ? outsst1 + ".tmp" : outsst1;
    std::ifstream in1(insst1, std::ios::binary);
    std::ifstream in2(insst2, std::ios::binary);
    if (!in1 || !in2)
        return -1;

    int nbits_hdr, nhashes_hdr, bitmap_sz_hdr, nleafs_hdr;
    in1.read(reinterpret_cast<char*>(&nbits_hdr), sizeof(int));
    in1.read(reinterpret_cast<char*>(&nhashes_hdr), sizeof(int));
    in1.read(reinterpret_cast<char*>(&bitmap_sz_hdr), sizeof(int));
    in1.read(reinterpret_cast<char*>(&nleafs_hdr), sizeof(int));
    in1.seekg(bitmap_sz_hdr, std::ios::cur);
    int rem1 = nleafs_hdr;

    in2.read(reinterpret_cast<char*>(&nbits_hdr), sizeof(int));
    in2.read(reinterpret_cast<char*>(&nhashes_hdr), sizeof(int));
    in2.read(reinterpret_cast<char*>(&bitmap_sz_hdr), sizeof(int));
    in2.read(reinterpret_cast<char*>(&nleafs_hdr), sizeof(int));
    in2.seekg(bitmap_sz_hdr, std::ios::cur);
    int rem2 = nleafs_hdr;
    int total_entries = 0;
    {
        std::ifstream c1(insst1, std::ios::binary), c2(insst2, std::ios::binary);
        c1.read(reinterpret_cast<char*>(&nbits_hdr), sizeof(int));
        c1.read(reinterpret_cast<char*>(&nhashes_hdr), sizeof(int));
        c1.read(reinterpret_cast<char*>(&bitmap_sz_hdr), sizeof(int));
        c1.read(reinterpret_cast<char*>(&nleafs_hdr), sizeof(int));
        c1.seekg(bitmap_sz_hdr, std::ios::cur);
        int lc1 = nleafs_hdr;

        c2.read(reinterpret_cast<char*>(&nbits_hdr), sizeof(int));
        c2.read(reinterpret_cast<char*>(&nhashes_hdr), sizeof(int));
        c2.read(reinterpret_cast<char*>(&bitmap_sz_hdr), sizeof(int));
        c2.read(reinterpret_cast<char*>(&nleafs_hdr), sizeof(int));
        c2.seekg(bitmap_sz_hdr, std::ios::cur);
        int lc2 = nleafs_hdr;

        BTreeNode node(true);
        for (int i = 0; i < lc1; i++) {
            c1.read(reinterpret_cast<char*>(&node), sizeof(BTreeNode));
            total_entries += node.cnt;
        }
        for (int i = 0; i < lc2; i++) {
            c2.read(reinterpret_cast<char*>(&node), sizeof(BTreeNode));
            total_entries += node.cnt;
        }
    }

    std::ofstream out(outsst2, std::ios::binary);
    if (!out)
        return -1;
    
    int ph = 0;
    for (int i = 0; i < 4; i++)
        out.write(reinterpret_cast<const char*>(&ph), sizeof(int));
    
    BloomFilter filter;
    filter.init(total_entries);

    Writer writer(out);
    BTreeNode n1(true), n2(true);

    bool has1 = false;
    if (rem1 > 0) {
        in1.read(reinterpret_cast<char*>(&n1), sizeof(BTreeNode));
        if (in1.gcount() == sizeof(BTreeNode)) {
            rem1--;
            has1 = true;
        }
    }

    bool has2 = false;
    if (rem2 > 0) {
        in2.read(reinterpret_cast<char*>(&n2), sizeof(BTreeNode));
        if (in2.gcount() == sizeof(BTreeNode)) {
            rem2--;
            has2 = true;
        }
    }

    int ki1 = 0, ki2 = 0; 
    bool deepest = (outsst2 == outsst1) && !std::filesystem::exists(name + "/sstables/SST" + std::to_string(lvl + 2));
    while (has1 && has2) {
        Key k1 = n1.keys[ki1], k2 = n2.keys[ki2];
        if (k1 == k2) {
            if (!(deepest && n2.vals[ki2] == (Val)DELETED_DEF_VAL)) {
                filter.insert(k2);
                writer.add(k2, n2.vals[ki2]);
            }

            ki1++;
            if (ki1 >= (int)n1.cnt) {
                ki1 = 0;
                has1 = false;
                if (rem1 > 0) {
                    in1.read(reinterpret_cast<char*>(&n1), sizeof(BTreeNode));
                    if (in1.gcount() == sizeof(BTreeNode)) {
                        rem1--;
                        has1 = true;
                    }
                }
            }

            ki2++;
            if (ki2 >= (int)n2.cnt) {
                ki2 = 0;
                has2 = false;
                if (rem2 > 0) {
                    in2.read(reinterpret_cast<char*>(&n2), sizeof(BTreeNode));
                    if (in2.gcount() == sizeof(BTreeNode)) {
                        rem2--;
                        has2 = true;
                    }
                }
            }
        } else if (k1 < k2) {
            if (!(deepest && n1.vals[ki1] == (Val)DELETED_DEF_VAL)) {
                filter.insert(k1);
                writer.add(k1, n1.vals[ki1]);
            }

            ki1++;
            if (ki1 >= (int)n1.cnt) {
                ki1 = 0;
                has1 = false;
                if (rem1 > 0) {
                    in1.read(reinterpret_cast<char*>(&n1), sizeof(BTreeNode));
                    if (in1.gcount() == sizeof(BTreeNode)) {
                        rem1--;
                        has1 = true;
                    }
                }
            }
        } else {
            if (!(deepest && n2.vals[ki2] == (Val)DELETED_DEF_VAL)) {
                filter.insert(k2);
                writer.add(k2, n2.vals[ki2]);
            }

            ki2++;
            if (ki2 >= (int)n2.cnt) {
                ki2 = 0;
                has2 = false;
                if (rem2 > 0) {
                    in2.read(reinterpret_cast<char*>(&n2), sizeof(BTreeNode));
                    if (in2.gcount() == sizeof(BTreeNode)) {
                        rem2--;
                        has2 = true;
                    }
                }
            }
        }
    }

    while (has1) {
        if (!(deepest && n1.vals[ki1] == (Val)DELETED_DEF_VAL)) {
            filter.insert(n1.keys[ki1]);
            writer.add(n1.keys[ki1], n1.vals[ki1]);
        }

        ki1++;
        if (ki1 >= (int)n1.cnt) {
            ki1 = 0;
            has1 = false;
            if (rem1 > 0) {
                in1.read(reinterpret_cast<char*>(&n1), sizeof(BTreeNode));
                if (in1.gcount() == sizeof(BTreeNode)) {
                    rem1--;
                    has1 = true;
                }
            }
        }
    }

    while (has2) {
        if (!(deepest && n2.vals[ki2] == (Val)DELETED_DEF_VAL)) {
            filter.insert(n2.keys[ki2]);
            writer.add(n2.keys[ki2], n2.vals[ki2]);
        }

        ki2++;
        if (ki2 >= (int)n2.cnt) {
            ki2 = 0;
            has2 = false;
            if (rem2 > 0) {
                in2.read(reinterpret_cast<char*>(&n2), sizeof(BTreeNode));
                if (in2.gcount() == sizeof(BTreeNode)) {
                    rem2--;
                    has2 = true;
                }
            }
        }
    }

    int nleafs = writer.finish();
    const std::vector<uint8_t>& bitmap = filter.get_filter();
    int nbits = filter.get_nbits();
    int nhashes = filter.get_nhashes();
    int bitmap_sz = bitmap.size();

    out.seekp(0, std::ios::beg);
    out.write(reinterpret_cast<const char*>(&nbits), sizeof(int));
    out.write(reinterpret_cast<const char*>(&nhashes), sizeof(int));
    out.write(reinterpret_cast<const char*>(&bitmap_sz), sizeof(int));
    out.write(reinterpret_cast<const char*>(&nleafs), sizeof(int));
    out.close();

    {
        std::ifstream tmp(outsst2, std::ios::binary);
        
        tmp.seekg(4 * sizeof(int));
        std::vector<BTreeNode> nodes;
        while (tmp.peek() != EOF) {
            BTreeNode node(false);
            tmp.read(reinterpret_cast<char*>(&node), sizeof(BTreeNode));
            if (tmp.gcount() == sizeof(BTreeNode))
                nodes.push_back(node);
        }
        tmp.close();

        std::ofstream fixed(outsst2, std::ios::binary | std::ios::trunc);
        fixed.write(reinterpret_cast<const char*>(&nbits), sizeof(int));
        fixed.write(reinterpret_cast<const char*>(&nhashes), sizeof(int));
        fixed.write(reinterpret_cast<const char*>(&bitmap_sz), sizeof(int));
        fixed.write(reinterpret_cast<const char*>(&nleafs), sizeof(int));
        fixed.write(reinterpret_cast<const char*>(bitmap.data()), bitmap_sz);
        for (const auto& node : nodes)
            fixed.write(reinterpret_cast<const char*>(&node), sizeof(BTreeNode));
    }

    std::filesystem::remove(insst1);
    std::filesystem::remove(insst2);

    if (outsst2 == outsst1 + ".tmp")
        compact(name, lvl + 1);

    return 0;
}

static void flush(const std::string& name, int lvl, const std::vector<BTreeNode>& tree, const BloomFilter& filter) {
    std::string path = name + "/sstables/SST" + std::to_string(lvl);
    if (std::filesystem::exists(path)) {
        sst_write(path + ".tmp", tree, filter);
        compact(name, lvl);
    } else {
        sst_write(path, tree, filter);
    }
}

Basalt::Basalt(const std::string& name) : name(name) {
    buf = new ExtendibleBufferPool();
    memtable = new Memtable();
    btree = new BPlusTree();

    std::filesystem::create_directories(name + "/sstables");
}

Basalt::~Basalt() {
    delete buf;
    delete memtable;
    delete btree;
}

std::optional<Val> Basalt::get(const Key& key) {
    std::optional<Val> val = memtable->get(key);
    if (val)
        return (*val == DELETED_DEF_VAL) ? std::nullopt : val;

    std::vector<int> lvls;
    std::filesystem::path dir = name + "/sstables";
    if (std::filesystem::exists(dir)) {
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (!entry.is_regular_file())
                continue;

            std::string file = entry.path().filename().string();
            if (file.rfind("SST", 0) != 0)
                continue;

            if (file.find('.') != std::string::npos)
                continue;

            std::string suffix = file.substr(3);
            if (suffix.empty())
                continue;

            bool digits = true;
            for (char c : suffix) {
                if (c < '0' || c > '9') {
                    digits = false;
                    break;
                }
            }

            if (digits)
                lvls.push_back(std::stoi(suffix));
        }
    }

    std::sort(lvls.begin(), lvls.end());
    for (int lvl : lvls) {
        std::string path = name + "/sstables/SST" + std::to_string(lvl);
        std::ifstream in(path, std::ios::binary);
        if (!in)
            continue;

        int nbits, nhashes, bitmap_sz, nleafs;
        in.read(reinterpret_cast<char*>(&nbits), sizeof(int));
        in.read(reinterpret_cast<char*>(&nhashes), sizeof(int));
        in.read(reinterpret_cast<char*>(&bitmap_sz), sizeof(int));
        in.read(reinterpret_cast<char*>(&nleafs), sizeof(int));

        std::vector<uint8_t> bitmap(bitmap_sz);
        in.read(reinterpret_cast<char*>(bitmap.data()), bitmap_sz);

        BloomFilter filter;
        filter.set_nbits(nbits);
        filter.set_nhashes(nhashes);
        filter.set_filter(bitmap);

        if (!filter.contains(key))
            continue;
        
        std::streamoff base = in.tellg();
        in.seekg(0, std::ios::end);
        int num_btree_nodes = (int)((in.tellg() - base) / sizeof(BTreeNode));
        if (num_btree_nodes == 0)
            continue;

        int idx = num_btree_nodes - 1;
        while (true) {
            BTreeNode node(false);

            std::streamoff offset = (std::streamoff)(idx * (int)sizeof(BTreeNode));
            std::string pid = path + std::to_string((long long)offset);

            auto cached = buf->get(pid);
            if (cached) {
                std::memcpy(&node, (*cached)->data, sizeof(BTreeNode));
            } else {
                std::array<uint8_t, PG_DEF_SZ> pg{};
                in.seekg(base + offset);
                in.read(reinterpret_cast<char*>(pg.data()), PG_DEF_SZ);
                buf->put(pid, pg.data());
                std::memcpy(&node, pg.data(), sizeof(BTreeNode));
            }

            if (node.leaf) {
                int L = 0, R = (int)node.cnt - 1;
                while (L <= R) {
                    int mid = (L + R) / 2;
                    if (node.keys[mid] == key)
                        return (node.vals[mid] == DELETED_DEF_VAL) ? std::nullopt : std::optional<Val>(node.vals[mid]);
                    else if (node.keys[mid] < key)
                        L = mid + 1;
                    else
                        R = mid - 1;
                }
                break;
            } else {
                idx = (int)node.vals[0];
                for (uint32_t i = 1; i < node.cnt; i++) {
                    if (key >= node.keys[i])
                        idx = (int)node.vals[i];
                    else
                        break;
                }
            }
        }
    }
    return std::nullopt;
}

int Basalt::put(const Key& key, const Val& val) {
    if (memtable->full()) {
        std::vector<std::pair<Key, Val>> pairs = memtable->flush();
        std::vector<BTreeNode> nodes = btree->build(pairs);

        BloomFilter filter;
        filter.fill(pairs);

        flush(name, 0, nodes, filter);
    }
    return memtable->put(key, val);
}

int Basalt::del(const Key& key) {
    return put(key, DELETED_DEF_VAL);
}

std::vector<std::pair<Key, Val>> Basalt::scan(const Key& start, const Key& end) {
    std::vector<std::pair<Key, Val>> flushed = memtable->scan(start, end);
    int memtable_kidx = 0;

    std::vector<std::unique_ptr<Cursor>> cursors;

    std::vector<int> lvls;
    std::filesystem::path dir = name + "/sstables";
    if (std::filesystem::exists(dir)) {
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (!entry.is_regular_file())
                continue;

            std::string file = entry.path().filename().string();
            if (file.rfind("SST", 0) != 0)
                continue;

            if (file.find('.') != std::string::npos)
                continue;

            std::string suffix = file.substr(3);
            if (suffix.empty())
                continue;

            bool digits = true;
            for (char c : suffix) {
                if (c < '0' || c > '9') {
                    digits = false;
                    break;
                }
            }

            if (digits)
                lvls.push_back(std::stoi(suffix));
        }
    }
    
    std::sort(lvls.begin(), lvls.end());
    for (int lvl : lvls) {
        std::string path = name + "/sstables/SST" + std::to_string(lvl);

        auto cursor = std::make_unique<Cursor>();
        cursor->lvl = lvl;
        cursor->path = path;
        cursor->in.open(path, std::ios::binary);
        if (!cursor->in)
            continue;

        int nbits, nhashes, bitmap_sz;
        cursor->in.read(reinterpret_cast<char*>(&nbits), sizeof(int));
        cursor->in.read(reinterpret_cast<char*>(&nhashes), sizeof(int));
        cursor->in.read(reinterpret_cast<char*>(&bitmap_sz),sizeof(int));
        cursor->in.read(reinterpret_cast<char*>(&cursor->nleafs), sizeof(int));
        cursor->in.seekg(bitmap_sz, std::ios::cur);
        cursor->base = cursor->in.tellg();
        if (cursor->nleafs == 0)
            continue;

        cursor->in.seekg(0, std::ios::end);
        int num_btree_nodes = (int)((cursor->in.tellg() - cursor->base) / sizeof(BTreeNode));
        if (num_btree_nodes == 0)
            continue;
        
        int idx = num_btree_nodes - 1;
        while (true) {
            std::streamoff offset = (std::streamoff)(idx * (int)sizeof(BTreeNode));
            std::string pid = cursor->path + ":" + std::to_string((long long)offset);

            auto cached = buf->get(pid);
            if (cached) {
                std::memcpy(&cursor->node, (*cached)->data, sizeof(BTreeNode));
            } else {
                std::array<uint8_t, PG_DEF_SZ> pg{};
                cursor->in.seekg(cursor->base + offset);
                cursor->in.read(reinterpret_cast<char*>(pg.data()), PG_DEF_SZ);
                buf->put(pid, pg.data());
                std::memcpy(&cursor->node, pg.data(), sizeof(BTreeNode));
            }

            if (cursor->node.leaf) {
                cursor->lidx = idx;
                break;
            }

            idx = (int)cursor->node.vals[0];
            for (uint32_t i = 1; i < cursor->node.cnt; i++) {
                if (start >= cursor->node.keys[i])
                    idx = (int)cursor->node.vals[i];
                else
                    break;
            }
        }
        
        cursor->kidx = 0;
        while (cursor->kidx < (int)cursor->node.cnt && cursor->node.keys[cursor->kidx] < start)
            cursor->kidx++;
        
        bool ok = true;
        while (ok) {
            if (cursor->kidx < (int)cursor->node.cnt) {
                if (cursor->node.keys[cursor->kidx] > end)
                    ok = false;
                else
                    break;
            } else {
                cursor->lidx++;
                if (cursor->lidx >= cursor->nleafs) {
                    ok = false;
                    break;
                }
                std::streamoff offset = (std::streamoff)(cursor->lidx * (int)sizeof(BTreeNode));
                std::string pid = cursor->path + ":" + std::to_string((long long)offset);

                auto cached = buf->get(pid);
                if (cached) {
                    std::memcpy(&cursor->node, (*cached)->data, sizeof(BTreeNode));
                } else {
                    std::array<uint8_t, PG_DEF_SZ> pg{};
                    cursor->in.seekg(cursor->base + offset);
                    cursor->in.read(reinterpret_cast<char*>(pg.data()), PG_DEF_SZ);
                    buf->put(pid, pg.data());
                    std::memcpy(&cursor->node, pg.data(), sizeof(BTreeNode));
                }

                cursor->kidx = 0;
            }
        }

        if (!ok)
            continue;

        cursor->valid = true;
        cursors.push_back(std::move(cursor));
    }
    
    std::vector<std::pair<Key, Val>> result;
    while (true) {
        Key min_key = std::numeric_limits<Key>::max();
        if (memtable_kidx < (int)flushed.size())
            min_key = flushed[memtable_kidx].first;

        for (auto& cursor : cursors) {
            if (cursor->valid)
                min_key = std::min(min_key, cursor->node.keys[cursor->kidx]);
        }
        
        if (min_key > end || min_key == std::numeric_limits<Key>::max())
            break;

        Val chosen = 0;
        int chosen_lvl = std::numeric_limits<int>::max();
        if (memtable_kidx < (int)flushed.size() && flushed[memtable_kidx].first == min_key) {
            chosen = flushed[memtable_kidx].second;
            chosen_lvl = -1;
            memtable_kidx++;
        }

        for (auto& c : cursors) {
            if (!c->valid || c->node.keys[c->kidx] != min_key) {
                continue;
            }

            if (c->lvl < chosen_lvl) {
                chosen = c->node.vals[c->kidx];
                chosen_lvl = c->lvl;
            }

            c->kidx++;
            while (c->kidx >= (int)c->node.cnt) {
                c->lidx++;
                if (c->lidx >= c->nleafs) {
                    c->valid = false;
                    break;
                }

                std::streamoff offset = (std::streamoff)(c->lidx * (int)sizeof(BTreeNode));
                std::string pid = c->path + ":" + std::to_string((long long)offset);

                auto cached = buf->get(pid);
                if (cached) {
                    std::memcpy(&c->node, (*cached)->data, sizeof(BTreeNode));
                } else {
                    std::array<uint8_t, PG_DEF_SZ> pg{};
                    c->in.seekg(c->base + offset);
                    c->in.read(reinterpret_cast<char*>(pg.data()), PG_DEF_SZ);
                    buf->put(pid, pg.data());
                    std::memcpy(&c->node, pg.data(), sizeof(BTreeNode));
                }

                c->kidx = 0;
            }

            if (c->valid && c->node.keys[c->kidx] > end)
                c->valid = false;
        }

        if (chosen != (Val)DELETED_DEF_VAL)
            result.push_back({min_key, chosen});
    }

    return result;
}

