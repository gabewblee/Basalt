#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "Config.hpp"
#include "IBufferPool.hpp"
#include "LRU.hpp"

class ExtendibleBufferPool : public IBufferPool {
public:
    /**
     * Constructor for the ExtendibleBufferPool
     */
    ExtendibleBufferPool();

    /**
     * Destructor for the ExtendibleBufferPool
     */
    ~ExtendibleBufferPool() override;

    /**
     * Retrieves a frame from the Extendible Buffer Pool
     * @param pid The page ID
     * @return The frame, or std::nullopt if not found
     */
    std::optional<Frame*> get(const std::string& pid) const override;

    /**
     * Inserts/updates a frame in the Extendible Buffer Pool
     * @param pid The page ID
     * @param data The page data
     * @return The frame
     */
    Frame* put(const std::string& pid, const uint8_t* data) override;

    /**
     * Deletes a frame from the Extendible Buffer Pool
     * @param pid The page ID
     */
    void del(const std::string& pid) override;

private:
    int global_depth;               /* Extendible Buffer Pool global depth */
    int maximum;                    /* Extendible Buffer Pool maximum depth */
    std::vector<Bucket*> directory; /* Extendible Buffer Pool directory */
};
