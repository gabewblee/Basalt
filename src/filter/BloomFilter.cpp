#include <algorithm>
#include <cstddef>
#include <cstring>
#include <stdexcept>

#include "BloomFilter.hpp"

#include "../libs/xxhash.h"

void BloomFilter::init(int n_entries) {
    double bits_per_entry = -std::log(FILTER_TARGET_FPR) / (std::log(2.0) * std::log(2.0));
    nbits = static_cast<int>(std::ceil(n_entries * bits_per_entry));
    nhashes = std::max(static_cast<int>(std::round(bits_per_entry * std::log(2.0))), 1);
    filter.assign((nbits + 7) / 8, 0);
}

void BloomFilter::clear() {
    nbits = 0;
    nhashes = 0;
    filter.clear();
}

void BloomFilter::fill(const std::vector<std::pair<Key, Val>>& entries) {
    if (entries.empty())
        throw std::invalid_argument("BloomFilter::fill: empty entries");

    init(entries.size());
    for (const auto& [key, _] : entries)
        insert(key);
}

void BloomFilter::insert(Key key) {
    uint64_t h1 = hash(key, 0);
    uint64_t h2 = hash(key, 1);
    for (int i = 0; i < nhashes; i++)
        set(nth(h1, h2, i));
}

bool BloomFilter::contains(Key key) const {
    if (nbits == 0 || nhashes == 0 || filter.empty())
        return false;

    uint64_t h1 = hash(key, 0);
    uint64_t h2 = hash(key, 1);
    for (int i = 0; i < nhashes; i++)
        if (!get(nth(h1, h2, i)))
            return false;
    return true;
}

int BloomFilter::get_nbits() const {
    return nbits;
}

int BloomFilter::get_nhashes() const {
    return nhashes;
}

std::vector<uint8_t> BloomFilter::get_filter() const {
    return filter;
}

void BloomFilter::set_nbits(int nbits) {
    this->nbits = nbits;
}

void BloomFilter::set_nhashes(int nhashes) {
    this->nhashes = nhashes;
}

void BloomFilter::set_filter(const std::vector<uint8_t>& filter) {
    this->filter = filter;
}

uint64_t BloomFilter::hash(Key key, uint64_t seed) const {
    return XXH3_64bits_withSeed(&key, sizeof(Key), seed);
}

int BloomFilter::nth(uint64_t h1, uint64_t h2, int i) const {
    const uint64_t m = static_cast<uint64_t>(nbits);
    return static_cast<int>((h1 + static_cast<uint64_t>(i) * h2) % m);
}

void BloomFilter::set(int i) {
    const std::size_t byte = static_cast<std::size_t>(i) >> 3;
    const unsigned shift = static_cast<unsigned>(i) & 7u;
    filter[byte] |= uint8_t(1u << shift);
}

bool BloomFilter::get(int i) const {
    const std::size_t byte = static_cast<std::size_t>(i) >> 3;
    const unsigned shift = static_cast<unsigned>(i) & 7u;
    return filter[byte] & uint8_t(1u << shift);
}