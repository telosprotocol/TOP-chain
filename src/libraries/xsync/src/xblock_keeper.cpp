#include "xsync/xblock_keeper.h"

NS_BEG2(top, sync)

bool xblock_keeper_t::update(const data::xblock_ptr_t &block) {

    std::string address = block->get_block_owner();
    uint64_t height = block->get_height();
    uint64_t view_id = block->get_viewid();

    std::unique_lock<std::mutex> lock(m_lock);
    auto it = m_blocks.find(address);
    if (it == m_blocks.end()) {
        height_view_info_t info;
        info.height = height;
        info.view_id = view_id;
        m_blocks[address] = info;
    } else {

        if (height < it->second.height) {
            return false;
        } else if (height > it->second.height) {
            height_view_info_t info;
            info.height = height;
            info.view_id = view_id;
            m_blocks[address] = info;
        } else {
            if (view_id > it->second.view_id) {
                it->second.view_id = view_id;
            } else {
                return false;
            }
        }
    }

    return true;
}

bool xblock_keeper_t::get_info(const std::string &address, uint64_t &height, uint64_t &view_id) const {
    std::unique_lock<std::mutex> lock(m_lock);
    auto it = m_blocks.find(address);
    if (it == m_blocks.end()) {
        return false;
    }

    height = it->second.height;
    view_id = it->second.view_id;
    return true;
}

NS_END2