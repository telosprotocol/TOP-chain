#include "xsync/xsync_mgr.h"
#include "xdata/xgenesis_data.h"
#include "xsync/xsync_message.h"
#include "xsync/xsync_log.h"

NS_BEG2(top, sync)

using namespace data;

xsync_mgr_t::xsync_mgr_t(std::string vnode_id, const std::string &address):
m_vnode_id(vnode_id),
m_address(address) {
}

xentire_block_request_ptr_t xsync_mgr_t::create_request(uint64_t start_height, uint32_t count) {

    std::string account_prefix;
    uint32_t table_id = 0;
    if (!data::xdatautil::extract_parts(m_address, account_prefix, table_id))
        return nullptr;

    xentire_block_request_ptr_t ptr = std::make_shared<xentire_block_request_t>(
                                m_address,
                                start_height,
                                count);

    xsync_info("[account][sync] create_sync_request %s range[%lu,%lu]",
            m_address.c_str(), start_height, start_height+count-1);

    return ptr;
}

NS_END2
