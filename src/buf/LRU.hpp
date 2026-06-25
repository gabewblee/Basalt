#pragma once

struct Frame;

typedef struct LRUNode {
    Frame* frame;  /* Frame         */
    LRUNode* next; /* Next node     */
    LRUNode* prev; /* Previous node */
} LRUNode;

class LRU {
public:
    /**
     * LRU - Constructor for the LRU.
     */
    LRU();

    /**
     * ~LRU - Destructor for the LRU.
     */
    ~LRU();

    /**
     * put - Insert a frame into the LRU.
     * @frame: The frame to insert.
     */
    void put(Frame* frame);

    /**
     * del - Delete a frame from the LRU.
     * @frame: The frame to delete.
     */
    void del(Frame* frame);

    /**
     * touch - Touch a frame in the LRU.
     * @frame: The frame to touch.
     */
    void touch(Frame* frame);

    /**
     * evict - Evict a frame from the LRU.
     * Returns: The evicted frame.
     */
    Frame* evict();

private:
    LRUNode* head; /* LRU Head */
    LRUNode* tail; /* LRU Tail */
};
