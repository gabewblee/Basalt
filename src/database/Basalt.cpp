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

static bool get_sst_lvl(const std::filesystem::directory_entry& entry, int& level) {
    if (!entry.is_regular_file())
        return false;

    std::string file = entry.path().filename().string();
    if (file.rfind("SST", 0) != 0)
        return false;

    if (file.find('.') != std::string::npos)
        return false;

    std::string suffix = file.substr(3);
    if (suffix.empty())
        return false;

    bool digits = true;
    for (char cursor : suffix) {
        if (cursor < '0' || cursor > '9') {
            digits = false;
            break;
        }
    }
    if (!digits)
        return false;

    level = std::stoi(suffix);
    return true;
}

static std::vector<int> get_sst_lvls(const std::string& name) {
    std::vector<int> levels;
    std::filesystem::path dir = name + "/sstables";
    if (!std::filesystem::exists(dir))
        return levels;

    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        int level = -1;
        if (get_sst_lvl(entry, level))
            levels.push_back(level);
    }

    std::sort(levels.begin(), levels.end());
    return levels;
}

static bool read_btree_node(std::ifstream& in, IBufferPool* buf, const std::string& path, std::streamoff base, int idx,BTreeNode& node) {
    std::streamoff offset = static_cast<std::streamoff>(idx * static_cast<int>(sizeof(BTreeNode)));
    std::string pid = path + ":" + std::to_string(static_cast<long long>(offset));

    auto cached = buf->get(pid);
    if (cached) {
        std::memcpy(&node, (*cached)->data, sizeof(BTreeNode));
        return true;
    }

    std::array<uint8_t, PG_DEF_SZ> pg{};
    in.seekg(base + offset);
    in.read(reinterpret_cast<char*>(pg.data()), PG_DEF_SZ);
    if (in.gcount() != PG_DEF_SZ)
        return false;

    buf->put(pid, pg.data());
    std::memcpy(&node, pg.data(), sizeof(BTreeNode));
    return true;
}

static bool read_sst_metadata(std::ifstream& in, int& bitmap_sz, int& nleafs) {
    int nbits = 0;
    int nhashes = 0;
    in.read(reinterpret_cast<char*>(&nbits), sizeof(int));
    in.read(reinterpret_cast<char*>(&nhashes), sizeof(int));
    in.read(reinterpret_cast<char*>(&bitmap_sz), sizeof(int));
    in.read(reinterpret_cast<char*>(&nleafs), sizeof(int));
    if (!in)
        return false;

    in.seekg(bitmap_sz, std::ios::cur);
    return static_cast<bool>(in);
}

static int num_leaf_keys(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in)
        return 0;

    int bitmap_sz = 0;
    int nleafs = 0;
    if (!read_sst_metadata(in, bitmap_sz, nleafs))
        return 0;

    int total = 0;
    BTreeNode node(true);
    for (int i = 0; i < nleafs; i++) {
        in.read(reinterpret_cast<char*>(&node), sizeof(BTreeNode));
        if (in.gcount() != sizeof(BTreeNode))
            break;
        total += node.cnt;
    }

    return total;
}

static bool load(std::ifstream& in, int& rem, BTreeNode& node) {
    if (rem <= 0)
        return false;

    in.read(reinterpret_cast<char*>(&node), sizeof(BTreeNode));
    if (in.gcount() != sizeof(BTreeNode))
        return false;

    rem--;
    return true;
}

static void advance(std::ifstream& in, int& rem, BTreeNode& node, int& key_idx, bool& has_leaf) {
    key_idx++;
    if (key_idx >= (int)node.cnt) {
        key_idx = 0;
        has_leaf = load(in, rem, node);
    }
}

static void append(BloomFilter& filter, Writer& writer, bool deepest, Key key, Val val) {
    if (deepest && val == (Val)DELETED_DEF_VAL)
        return;

    filter.insert(key);
    writer.add(key, val);
}

static int get_max_sst_id(const std::string& name) {
    std::vector<int> levels = get_sst_lvls(name);
    if (levels.empty())
        return -1;

    return levels.back();
}

static void sst_write(const std::string& path, const std::vector<BTreeNode>& tree, const BloomFilter& filter) {
    std::ofstream out(path, std::ios::binary);
    if (!out)
        throw std::runtime_error("Error: failed to open file " + path + " for writing");

    int nbits = filter.get_nbits();
    int nhashes = filter.get_nhashes();
    const std::vector<uint8_t>& bitmap = filter.get_filter();
    int bitmap_sz = bitmap.size();
    int nleafs = 0;
    for (const auto& node : tree) {
        if (node.leaf)
            nleafs++;
        else
            break;
    }

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

    int bitmap_sz1 = 0;
    int rem1 = 0;
    if (!read_sst_metadata(in1, bitmap_sz1, rem1))
        return -1;

    int bitmap_sz2 = 0;
    int rem2 = 0;
    if (!read_sst_metadata(in2, bitmap_sz2, rem2))
        return -1;

    std::ofstream out(outsst2, std::ios::binary);
    if (!out)
        return -1;
        
    int total_num_keys = num_leaf_keys(insst1) + num_leaf_keys(insst2);

    BloomFilter filter;
    filter.init(total_num_keys);

    int nbits = filter.get_nbits();
    int nhashes = filter.get_nhashes();
    int bitmap_sz = filter.get_filter().size();
    int nleafs = 0;

    out.write(reinterpret_cast<const char*>(&nbits), sizeof(int));
    out.write(reinterpret_cast<const char*>(&nhashes), sizeof(int));
    out.write(reinterpret_cast<const char*>(&bitmap_sz), sizeof(int));
    out.write(reinterpret_cast<const char*>(&nleafs), sizeof(int));

    std::vector<uint8_t> bitmap(bitmap_sz, 0);
    out.write(reinterpret_cast<const char*>(bitmap.data()), bitmap_sz);

    Writer writer(out);
    BTreeNode n1(true), n2(true);

    bool has1 = load(in1, rem1, n1);
    bool has2 = load(in2, rem2, n2);

    int ki1 = 0, ki2 = 0;
    bool deepest = (outsst2 == outsst1) && (lvl + 1 >= get_max_sst_id(name));
    while (has1 && has2) {
        Key k1 = n1.keys[ki1], k2 = n2.keys[ki2];
        if (k1 == k2) {
            append(filter, writer, deepest, k2, n2.vals[ki2]);
            advance(in1, rem1, n1, ki1, has1);
            advance(in2, rem2, n2, ki2, has2);
        } else if (k1 < k2) {
            append(filter, writer, deepest, k1, n1.vals[ki1]);
            advance(in1, rem1, n1, ki1, has1);
        } else {
            append(filter, writer, deepest, k2, n2.vals[ki2]);
            advance(in2, rem2, n2, ki2, has2);
        }
    }

    while (has1) {
        append(filter, writer, deepest, n1.keys[ki1], n1.vals[ki1]);
        advance(in1, rem1, n1, ki1, has1);
    }

    while (has2) {
        append(filter, writer, deepest, n2.keys[ki2], n2.vals[ki2]);
        advance(in2, rem2, n2, ki2, has2);
    }

    nleafs = writer.finish();
    bitmap = filter.get_filter();

    out.seekp(0, std::ios::beg);
    out.write(reinterpret_cast<const char*>(&nbits), sizeof(int));
    out.write(reinterpret_cast<const char*>(&nhashes), sizeof(int));
    out.write(reinterpret_cast<const char*>(&bitmap_sz), sizeof(int));
    out.write(reinterpret_cast<const char*>(&nleafs), sizeof(int));
    out.write(reinterpret_cast<const char*>(bitmap.data()), bitmap_sz);
    out.close();

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

    std::vector<int> levels = get_sst_lvls(name);
    for (int lvl : levels) {
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
            if (!read_btree_node(in, buf, path, base, idx, node))
                break;

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

    std::vector<int> levels = get_sst_lvls(name);
    for (int lvl : levels) {
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
            if (!read_btree_node(cursor->in, buf, cursor->path, cursor->base, idx, cursor->node))
                break;

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
                if (!read_btree_node(cursor->in, buf, cursor->path, cursor->base, cursor->lidx, cursor->node)) {
                    ok = false;
                    break;
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

        for (auto& cursor : cursors) {
            if (!cursor->valid || cursor->node.keys[cursor->kidx] != min_key) {
                continue;
            }

            if (cursor->lvl < chosen_lvl) {
                chosen = cursor->node.vals[cursor->kidx];
                chosen_lvl = cursor->lvl;
            }

            cursor->kidx++;
            while (cursor->kidx >= (int)cursor->node.cnt) {
                cursor->lidx++;
                if (cursor->lidx >= cursor->nleafs) {
                    cursor->valid = false;
                    break;
                }
                if (!read_btree_node(cursor->in, buf, cursor->path, cursor->base, cursor->lidx, cursor->node)) {
                    cursor->valid = false;
                    break;
                }

                cursor->kidx = 0;
            }

            if (cursor->valid && cursor->node.keys[cursor->kidx] > end)
                cursor->valid = false;
        }

        if (chosen != (Val)DELETED_DEF_VAL)
            result.push_back({min_key, chosen});
    }

    return result;
}

