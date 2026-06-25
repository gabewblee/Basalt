#pragma once

#include <optional>

#include "../Types.hpp"

class IBufferPool {
public:
    /**
     * ~IBufferPool - Destructor for the IBufferPool class.
     */
    virtual ~IBufferPool() = default;

    /**
     * get - Retrieve a frame from the buffer pool.
     * @pid: The page ID.
     * Returns: The frame, or std::nullopt if not found.
     */
    virtual std::optional<Frame*> get(const std::string& pid) const = 0;

    /**
     * put - Insert/update a frame in the buffer pool.
     * @pid: The page ID.
     * @data: The page data.
     * Returns: The frame.
     */
    virtual Frame* put(const std::string& pid, const uint8_t* data) = 0;
    
    /**
     * del - Delete a frame from the buffer pool.
     * @pid: The page ID.
     */
    virtual void del(const std::string& pid) = 0;
};
