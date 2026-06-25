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
     * ExtendibleBufferPool - Constructor for the ExtendibleBufferPool.
     */
    ExtendibleBufferPool();

    /**
     * ~ExtendibleBufferPool - Destructor for the ExtendibleBufferPool.
     */
    ~ExtendibleBufferPool() override;

    /**
     * get - Retrieve a frame from the Extendible Buffer Pool.
     * @pid: The page ID.
     * Returns: The frame, or std::nullopt if not found.
     */
    std::optional<Frame*> get(const std::string& pid) const override;

    /**
     * put - Insert/update a frame in the Extendible Buffer Pool.
     * @pid: The page ID.
     * @data: The page data.
     * Returns: The frame.
     */
    Frame* put(const std::string& pid, const uint8_t* data) override;

    /**
     * del - Delete a frame from the Extendible Buffer Pool.
     * @pid: The page ID.
     */
    void del(const std::string& pid) override;

private:
    int                  global_depth; /* Extendible Buffer Pool global depth  */
    int                  maximum;      /* Extendible Buffer Pool maximum depth */
    std::vector<Bucket*> directory;    /* Extendible Buffer Pool directory     */
};
