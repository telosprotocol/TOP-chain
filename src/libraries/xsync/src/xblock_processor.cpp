#include "xsync/xblock_processor.h"
#include "xmbus/xevent_store.h"
#include "xdata/xgenesis_data.h"

NS_BEG2(top, sync)

using namespace data;

xblock_processor_t::xblock_processor_t(std::string vnode_id,
                                       const observer_ptr<mbus::xmessage_bus_face_t> &mbus):
m_mbus(mbus) {

}

void xblock_processor_t::handle_block(const mbus::xevent_ptr_t& e) {

#if 0
    auto bme = std::static_pointer_cast<mbus::xevent_store_block_to_db_t>(e);

    const xblock_ptr_t & block = bme->block;
    const std::string &address = block->get_account();

    bool is_unit_address = data::is_unit_address(common::xaccount_address_t{m_chain_info.address});
    bool is_table_address = data::is_table_address(common::xaccount_address_t{m_chain_info.address});

    // xsync_dbg("[account] finish range=%s %lu", m_address.c_str(), block->get_height());

    if (is_unit_address) {
        if (m_is_history)
            m_sync_tableunit.on_history_unit(block, m_data_mgr);
    } else if (is_table_address) {
        m_sync_tableunit.on_active_tableblock(block);
        if (m_is_history)
            m_sync_tableunit.on_history_tableblock(block);
    }

    m_sync_range_mgr->on_data(block);
#endif
}

NS_END2
