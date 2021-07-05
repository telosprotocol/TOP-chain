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

//vnetwork::xvnode_address_t build_address_from_vnode(const xvip2_t &group_xip2, const std::vector<base::xvnode_t*> &nodes, int32_t slot_id) {
//
//    assert(slot_id >= 0);
//    assert((uint32_t)slot_id < nodes.size());
//
//    common::xzone_id_t zone_id = common::xzone_id_t{uint8_t(get_zone_id_from_xip2(group_xip2))};
//    common::xcluster_id_t cluster_id = common::xcluster_id_t{uint8_t(get_cluster_id_from_xip2(group_xip2))};
//    common::xgroup_id_t group_id = common::xgroup_id_t{uint8_t(get_group_id_from_xip2(group_xip2))};
//    common::xversion_t ver = common::xversion_t{uint8_t(get_network_ver_from_xip2(group_xip2))};
//    common::xnetwork_id_t nid = common::xnetwork_id_t{uint32_t(get_network_id_from_xip2(group_xip2))};
//    uint64_t elect_height = get_network_height_from_xip2(group_xip2);
//
//    uint32_t group_size = nodes.size();
//
//    auto const cluster_addr = common::xsharding_address_t{
//                                    nid,
//                                    zone_id,
//                                    cluster_id,
//                                    group_id
//                                };
//
//    base::xvnode_t* node = nodes[slot_id];
//    common::xaccount_election_address_t account_election_address{ common::xnode_id_t { node->get_account() }, common::xslot_id_t{(uint16_t)slot_id} };
//    vnetwork::xvnode_address_t addr = vnetwork::xvnode_address_t{cluster_addr, account_election_address, ver, (uint16_t)group_size, elect_height};
//
//    return addr;
//}

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
