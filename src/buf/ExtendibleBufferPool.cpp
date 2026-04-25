#include <cmath>
#include <cstring>

#include "ExtendibleBufferPool.hpp"

#include "../libs/xxhash.h"

static uint64_t digest(const std::string& pid) {
    return XXH64(pid.data(), pid.size(), 0);
}

static size_t slot(uint64_t signature, size_t length) {
    return signature & (length - 1);
}

static void prepend(Bucket* bucket, Frame* frame) {
    frame->prev = nullptr;
    frame->next = bucket->head;
    if (bucket->head)
        bucket->head->prev = frame;
    else
        bucket->tail = frame;

    bucket->head = frame;
    bucket->occupancy++;
}

static void unlink(Bucket* bucket, Frame* frame) {
    if (frame->prev)
        frame->prev->next = frame->next;
    else
        bucket->head = frame->next;

    if (frame->next)
        frame->next->prev = frame->prev;
    else
        bucket->tail = frame->prev;

    bucket->occupancy--;
}

static Frame* drain(Bucket* bucket) {
    Frame* head = bucket->head;
    bucket->head = nullptr;
    bucket->tail = nullptr;
    bucket->occupancy = 0;
    return head;
}

static void grow(std::vector<Bucket*>& directory, int* global_depth) {
    const size_t half = directory.size();
    directory.resize(half * 2);
    for (size_t index = 0; index < half; index++)
        directory[half + index] = directory[index];

    (*global_depth)++;
}

static void partition(int* global_depth, std::vector<Bucket*>& directory, Bucket* bucket) {
    const int separator = bucket->depth;
    if (separator == *global_depth)
        grow(directory, global_depth);

    Bucket* partner = new Bucket();
    partner->depth = separator + 1;
    partner->occupancy = 0;
    partner->capacity = bucket->capacity;
    partner->head = nullptr;
    partner->tail = nullptr;

    bucket->depth = separator + 1;

    const size_t width = directory.size();
    for (size_t index = 0; index < width; index++) {
        if (directory[index] != bucket)
            continue;

        if ((index >> separator) & 1u)
            directory[index] = partner;
    }

    for (Frame* frame = drain(bucket); frame;) {
        Frame* rest = frame->next;
        frame->next = frame->prev = nullptr;
        bucket->lru.del(frame);
        Bucket* destination = directory[slot(digest(frame->pid), directory.size())];
        prepend(destination, frame);
        destination->lru.put(frame);
        frame = rest;
    }
}

ExtendibleBufferPool::ExtendibleBufferPool() {
    maximum = static_cast<int>(std::ceil(std::log2(BUFFER_POOL_LIMIT)));
    global_depth = 0;

    directory.resize(1);
    Bucket* bucket = new Bucket();
    bucket->depth = 0;
    bucket->occupancy = 0;
    bucket->capacity = BUFFER_POOL_CAPACITY;
    bucket->head = nullptr;
    bucket->tail = nullptr;
    directory[0] = bucket;
}

ExtendibleBufferPool::~ExtendibleBufferPool() {
    for (size_t index = 0; index < directory.size(); index++) {
        Bucket* bucket = directory[index];
        bool visited = false;
        for (size_t earlier = 0; earlier < index; earlier++) {
            if (directory[earlier] == bucket) {
                visited = true;
                break;
            }
        }

        if (visited)
            continue;

        for (Frame* frame = bucket->head; frame;) {
            Frame* rest = frame->next;
            bucket->lru.del(frame);
            unlink(bucket, frame);
            delete frame;
            frame = rest;
        }
        delete bucket;
    }
}

std::optional<Frame*> ExtendibleBufferPool::get(const std::string& pid) const {
    Bucket* bucket = directory[slot(digest(pid), directory.size())];
    for (Frame* frame = bucket->head; frame; frame = frame->next) {
        if (frame->pid != pid)
            continue;

        bucket->lru.touch(frame);
        return frame;
    }

    return std::nullopt;
}

Frame* ExtendibleBufferPool::put(const std::string& pid, const uint8_t* data) {
    while (1) {
        const uint64_t signature = digest(pid);
        Bucket* bucket = directory[slot(signature, directory.size())];
        for (Frame* frame = bucket->head; frame; frame = frame->next) {
            if (frame->pid != pid)
                continue;

            std::memcpy(frame->data, data, PG_DEF_SZ);
            bucket->lru.touch(frame);
            return frame;
        }

        if (bucket->occupancy < bucket->capacity)
            break;

        if (bucket->depth >= maximum) {
            Frame* victim = bucket->lru.evict();
            if (!victim)
                return nullptr;

            unlink(bucket, victim);
            delete victim;
            break;
        }

        partition(&global_depth, directory, bucket);
    }

    const uint64_t signature = digest(pid);
    Bucket* bucket = directory[slot(signature, directory.size())];

    Frame* frame = new Frame();
    frame->pid = pid;
    frame->next = frame->prev = nullptr;
    std::memcpy(frame->data, data, PG_DEF_SZ);

    prepend(bucket, frame);
    bucket->lru.put(frame);
    return frame;
}

void ExtendibleBufferPool::del(const std::string& pid) {
    Bucket* bucket = directory[slot(digest(pid), directory.size())];
    for (Frame* frame = bucket->head; frame; frame = frame->next) {
        if (frame->pid != pid)
            continue;

        bucket->lru.del(frame);
        unlink(bucket, frame);
        delete frame;
        return;
    }
}
