#include "xsync/xsync_util.h"
#include "xdata/xnative_contract_address.h"

NS_BEG2(top, sync)

data::xblock_ptr_t autoptr_to_blockptr(base::xauto_ptr<base::xvblock_t> &autoptr) {
    if (autoptr == nullptr)
        return nullptr;

    autoptr->add_ref();
    base::xvblock_t *vblock = autoptr.get();
    data::xblock_t *block = (data::xblock_t*)vblock;
    data::xblock_ptr_t block_ptr = nullptr;
    block_ptr.attach(block);

    return block_ptr;
}

bool is_beacon_table(const std::string &address) {

    std::string account_prefix;
    uint32_t table_id = 0;
    if (!data::xdatautil::extract_parts(address, account_prefix, table_id)) {
        return false;
    }

    if (account_prefix != sys_contract_beacon_table_block_addr)
        return false;

    return true;
}

bool check_auth(const observer_ptr<base::xvcertauth_t> &certauth, data::xblock_ptr_t &block) {
    XMETRICS_TIME_RECORD("xsync_store_check_auth");
    
    //No.1 safe rule: clean all flags first when sync/replicated one block
    block->reset_block_flags();

    base::enum_vcert_auth_result result = certauth->verify_muti_sign(block.get());
    if (result != base::enum_vcert_auth_result::enum_successful) {
        return false;
    }

    block->set_block_flag(base::enum_xvblock_flag_authenticated);

    return true;
}

uint32_t vrf_value(const std::string& hash) {

    uint32_t value = 0;
    const uint8_t* data = (const uint8_t*)hash.data();
    for (size_t i = 0; i < hash.size(); i++) {
        value += data[i];
    }

    return value;
}
 
uint64_t derministic_height(uint64_t my_height, std::pair<uint64_t, uint64_t> neighbor_heights) {
    uint64_t height = my_height;

    if (my_height >= neighbor_heights.first + xsync_store_t::m_undeterministic_heights) {
        height = my_height - xsync_store_t::m_undeterministic_heights;
    } else {
        height = neighbor_heights.first;
    }
    return height;
}

NS_END2
