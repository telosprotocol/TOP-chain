#include "xsync/xsync_behind_checker.h"
#include "xsync/xsync_log.h"
#include "xsync/xsync_util.h"
#include "xmetrics/xmetrics.h"
#include "xdata/xnative_contract_address.h"
NS_BEG2(top, sync)

xsync_behind_checker_t::xsync_behind_checker_t(std::string vnode_id, xsync_store_face_t *sync_store, xrole_chains_mgr_t *role_chains_mgr, xsync_peerset_t *peerset, xdownloader_face_t *downloader):
m_vnode_id(vnode_id),
m_sync_store(sync_store),
m_role_chains_mgr(role_chains_mgr),
m_peerset(peerset),
m_downloader(downloader) {

}

void xsync_behind_checker_t::on_timer() {

    if (m_time_rejecter.reject()){
        return;
    }
    
    m_counter++;
    if (m_counter %10 != 0)
        return;

    std::string reason = "timer_check";

    xsync_roles_t roles = m_role_chains_mgr->get_roles();
    for (const auto &role_it: roles) {

        const vnetwork::xvnode_address_t &self_addr = role_it.first;
        const std::shared_ptr<xrole_chains_t> &role_chains = role_it.second;

        const map_chain_info_t &chains = role_chains->get_chains_wrapper().get_chains();
        for (const auto &it: chains) {
            const std::string &address = it.first;
            enum_chain_sync_policy sync_policy = it.second.sync_policy;

            check_one(address, sync_policy, self_addr, reason);
        }
    }
}

void xsync_behind_checker_t::on_behind_check_event(const mbus::xevent_ptr_t &e) {
    auto bme = dynamic_xobject_ptr_cast<mbus::xevent_behind_check_t>(e);
    std::string address = bme->address;

    std::string account_prefix;
    uint32_t table_id = 0;

    bool is_table_address = data::is_table_address(common::xaccount_address_t{address});
    bool is_drand_address = data::is_drand_address(common::xaccount_address_t{address}); 
    if (!is_table_address && !is_drand_address)
        return;

    if (is_table_address) {
        if (!data::xdatautil::extract_parts(address, account_prefix, table_id))
            return;
    }

    std::string reason = "trigger_check";

    xsync_roles_t roles = m_role_chains_mgr->get_roles();
    for (const auto &role_it: roles) {

        const vnetwork::xvnode_address_t &self_addr = role_it.first;
        const std::shared_ptr<xrole_chains_t> &role_chains = role_it.second;

        const map_chain_info_t &chains = role_chains->get_chains_wrapper().get_chains();

        auto it2 = chains.find(address);
        if (it2 == chains.end())
            continue;

        enum_chain_sync_policy sync_policy = it2->second.sync_policy;

        check_one(address, sync_policy, self_addr, reason);
    }
}

void xsync_behind_checker_t::check_one(const std::string &address, enum_chain_sync_policy sync_policy, const vnetwork::xvnode_address_t &self_addr, const std::string &reason) {

    uint64_t latest_start_block_height = m_sync_store->get_latest_start_block_height(address, sync_policy);
    uint64_t latest_end_block_height = m_sync_store->get_latest_end_block_height(address, sync_policy);

    vnetwork::xvnode_address_t peer_addr;

    std::multimap<uint64_t, mbus::chain_behind_event_address> chain_behind_address_map{};

    if (m_peerset->get_peer_height_info_map(self_addr, address, latest_start_block_height, latest_end_block_height, chain_behind_address_map)) {
        
       if ((m_counter % 120) == 0) {
            std::string sync_mode;
            std::string gap_metric_tag_name;
            if (sync_policy == enum_chain_sync_policy_fast) {
                sync_mode = "fast";
                gap_metric_tag_name = "xsync_fast_mode_gap_" + address;
            } else if (sync_policy == enum_chain_sync_policy_full) {
                sync_mode = "full";
                gap_metric_tag_name = "xsync_full_mode_gap_" + address;
            } else if (sync_policy == enum_chain_sync_policy_checkpoint) {
                sync_mode = "cp";
                gap_metric_tag_name = "xsync_cp_mode_gap_" + address;
            }
#ifdef ENABLE_METRICS
            uint64_t gap_between_interval = chain_behind_address_map.crbegin()->first - latest_end_block_height;
            XMETRICS_COUNTER_SET(gap_metric_tag_name, gap_between_interval);
#endif
            XMETRICS_PACKET_INFO("xsync_interval",
                                 "mode",
                                 sync_mode,
                                 "table_address",
                                 address,
                                 "self_min",
                                 latest_start_block_height,
                                 "self_max",
                                 latest_end_block_height,
                                 "peer_min",
                                 chain_behind_address_map.crbegin()->second.start_height,
                                 "peer_max",
                                 chain_behind_address_map.crbegin()->first);
        }
        
        xsync_info("behind_checker notify %s, local(start_height=%lu,end_height=%lu) peer(start_height=%lu, " \
                  "end_height=%lu) sync_policy(%d) node_map_size %u reason=%s ", 
                  address.c_str(), latest_start_block_height, latest_end_block_height, chain_behind_address_map.crbegin()->second.start_height, 
                  chain_behind_address_map.crbegin()->first, chain_behind_address_map.size(), (int32_t)sync_policy, reason.c_str());
        mbus::xevent_ptr_t ev = make_object_ptr<mbus::xevent_behind_download_t>(address, sync_policy, chain_behind_address_map, reason);
        m_downloader->push_event(ev);
        return;
    }
    xsync_dbg("xsync_behind_checker_t::check_one end, %d, table_address %s sele_address %s ,%llu,%llu,%s", sync_policy, address.c_str(), self_addr.to_string().c_str(),
              latest_start_block_height, latest_end_block_height,peer_addr.to_string().c_str());
}

NS_END2
