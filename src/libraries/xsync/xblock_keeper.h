#pragma once

#include <mutex>
#include <unordered_map>
#include "xbasic/xns_macro.h"
#include "xbasic/xmemory.hpp"
#include "xdata/xblock.h"

NS_BEG2(top, sync)

class height_view_info_t {
public:
    uint64_t height;
    uint64_t view_id;
};

class xblock_keeper_t {
public:
    bool update(const data::xblock_ptr_t &block);
    bool get_info(const std::string &address, uint64_t &height, uint64_t &view_id) const;

private:
    mutable std::mutex m_lock;
    std::map<std::string, height_view_info_t> m_blocks;
};

NS_END2