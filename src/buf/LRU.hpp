#pragma once

struct Frame;

typedef struct LRUNode {
    Frame* frame;  /* Frame */
    LRUNode* next; /* Next node */
    LRUNode* prev; /* Previous node */
} LRUNode;

class LRU {
public:
    /**
     * Constructor for the LRU
     */
    LRU();

    /**
     * Destructor for the LRU
     */
    ~LRU();

    /**
     * Inserts a frame into the LRU
     * @param frame The frame to insert
     */
    void put(Frame* frame);

    /**
     * Deletes a frame from the LRU
     * @param frame The frame to delete
     */
    void del(Frame* frame);

    /**
     * Touches a frame in the LRU
     * @param frame The frame to touch
     */
    void touch(Frame* frame);

    /**
     * Evicts a frame from the LRU
     * @return The evicted frame
     */
    Frame* evict();

private:
    LRUNode* head; /* LRU Head */
    LRUNode* tail; /* LRU Tail */
};