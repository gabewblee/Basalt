#pragma once

#include <optional>

#include "../Types.hpp"

class IBufferPool {
public:
    /**
     * Destructor for the IBufferPool class
     */
    virtual ~IBufferPool() = default;

    /**
     * Retrieve a frame from the buffer pool
     * @param pid The page ID
     * @return The frame, or std::nullopt if not found
     */
    virtual std::optional<Frame*> get(const std::string& pid) const = 0;

    /**
     * Insert/update a frame in the buffer pool
     * @param pid The page ID
     * @param data The page data
     * @return The frame
     */
    virtual Frame* put(const std::string& pid, const uint8_t* data) = 0;
    
    /**
     * Delete a frame from the buffer pool
     * @param pid The page ID
     */
    virtual void del(const std::string& pid) = 0;
};
