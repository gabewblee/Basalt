#include "LRU.hpp"

LRU::LRU() : head(nullptr), tail(nullptr) {}

LRU::~LRU() {
    LRUNode* current = head;
    while (current) {
        LRUNode* next = current->next;
        delete current;
        current = next;
    }
}

void LRU::put(Frame* frame) {
    LRUNode* node = new LRUNode();
    node->frame = frame;
    node->prev = nullptr;
    node->next = head;

    if (head)
        head->prev = node;
    else
        tail = node;

    head = node;
}

void LRU::del(Frame* frame) {
    LRUNode* node = head;
    while (node && node->frame != frame)
        node = node->next;

    if (node == nullptr)
        return;

    if (node->prev)
        node->prev->next = node->next;
    else
        head = node->next;

    if (node->next)
        node->next->prev = node->prev;
    else
        tail = node->prev;

    delete node;
}

void LRU::touch(Frame* frame) {
    del(frame);
    put(frame);
}

Frame* LRU::evict() {
    if (tail == nullptr)
        return nullptr;

    Frame* const frame = tail->frame;
    del(frame);
    return frame;
}
