// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_object.h"
#include "xconfig/xconfig_register.h"
#include "xmbus/xevent_role.h"
#include "xsyncbase/xsync_policy.h"

NS_BEG2(top, sync)

xtop_sync_object::xtop_sync_object(observer_ptr<mbus::xmessage_bus_face_t> const & bus,
                                   observer_ptr<store::xstore_face_t> const & store,
                                   observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                                   xobject_ptr_t<base::xvblockstore_t> &blockstore,
                                   xobject_ptr_t<base::xvnodesrv_t> &nodesvr_ptr,
                                   xobject_ptr_t<base::xvcertauth_t> &cert_ptr,
                                   observer_ptr<base::xiothread_t> const & sync_thread,
                                   std::vector<observer_ptr<base::xiothread_t>> const & sync_account_thread_pool,
                                   std::vector<observer_ptr<base::xiothread_t>> const & sync_handler_thread_pool):
    m_bus{ bus },
    m_instance(vhost->host_node_id().c_str()),
    m_store_shadow(top::make_unique<sync::xsync_store_shadow_t>()),
    m_sync_store(top::make_unique<sync::xsync_store_t>(m_instance, make_observer(blockstore), m_store_shadow.get())),
    m_blacklist(top::make_unique<sync::xdeceit_node_manager_t>()),
    m_session_mgr(top::make_unique<sync::xsession_manager_t>(XGET_CONFIG(executor_max_sessions))),
    m_role_chains_mgr(top::make_unique<sync::xrole_chains_mgr_t>(m_instance)),
    m_role_xips_mgr(top::make_unique<sync::xrole_xips_manager_t>(m_instance)),
    m_sync_sender(top::make_unique<sync::xsync_sender_t>(m_instance, vhost, m_role_xips_mgr.get())),
    m_sync_ratelimit(top::make_unique<sync::xsync_ratelimit_t>(sync_thread, (uint32_t)100)),
    m_peerset(top::make_unique<sync::xsync_peerset_t>(m_instance)),
    m_sync_pusher(top::make_unique<sync::xsync_pusher_t>(m_instance, m_role_xips_mgr.get(), m_sync_sender.get())),
    m_sync_broadcast(top::make_unique<sync::xsync_broadcast_t>(m_instance, m_peerset.get(), m_sync_sender.get())),
    m_downloader(top::make_unique<sync::xdownloader_t>(m_instance, m_sync_store.get(), bus, make_observer(cert_ptr), m_role_chains_mgr.get(),
        m_sync_sender.get(), sync_account_thread_pool, m_sync_ratelimit.get(), m_store_shadow.get())),
    m_block_fetcher(top::make_unique<sync::xblock_fetcher_t>(m_instance, sync_thread, bus, make_observer(cert_ptr), m_role_chains_mgr.get(), m_sync_store.get(),
        m_sync_broadcast.get(), m_sync_sender.get())),
    m_sync_gossip(top::make_unique<sync::xsync_gossip_t>(m_instance, m_bus, m_sync_store.get(), m_role_chains_mgr.get(), m_role_xips_mgr.get(), m_sync_sender.get())),
    m_sync_on_demand(top::make_unique<sync::xsync_on_demand_t>(m_instance, m_bus, make_observer(cert_ptr), m_sync_store.get(), m_role_chains_mgr.get(), m_role_xips_mgr.get(), m_sync_sender.get())),
    m_peer_keeper(top::make_unique<sync::xsync_peer_keeper_t>(m_instance, m_sync_store.get(), m_role_chains_mgr.get(), m_role_xips_mgr.get(), m_sync_sender.get(), m_peerset.get())),
    m_behind_checker(top::make_unique<sync::xsync_behind_checker_t>(m_instance, m_sync_store.get(), m_role_chains_mgr.get(), m_peerset.get(), m_downloader.get())),
    m_cross_cluster_chain_state(top::make_unique<sync::xsync_cross_cluster_chain_state_t>(m_instance, m_sync_store.get(), m_role_chains_mgr.get(), m_role_xips_mgr.get(), m_sync_sender.get(), m_downloader.get())),
    m_sync_handler(top::make_unique<sync::xsync_handler_t>(
                m_instance,
                m_sync_store.get(),
                make_observer(cert_ptr),
                m_session_mgr.get(),
                m_blacklist.get(),
                m_role_chains_mgr.get(),
                m_role_xips_mgr.get(),
                m_downloader.get(),
                m_block_fetcher.get(),
                m_sync_gossip.get(),
                m_sync_pusher.get(),
                m_sync_broadcast.get(),
                m_sync_sender.get(),
                m_sync_on_demand.get(),
                m_peerset.get(),
                m_peer_keeper.get(),
                m_behind_checker.get(),
                m_cross_cluster_chain_state.get())),
    m_sync_event_dispatcher(make_object_ptr<sync::xsync_event_dispatcher_t>(
            sync_thread,
            m_instance,
            bus,
            m_sync_handler.get())),
    m_sync_netmsg_dispatcher(top::make_unique<sync::xsync_netmsg_dispatcher_t>(m_instance, sync_handler_thread_pool, bus, vhost, m_sync_handler.get())){
}

void
xtop_sync_object::start() {
    m_sync_ratelimit->start();
}

void
xtop_sync_object::stop() {
    m_sync_ratelimit->stop();
}

class xsync_progress_t {
public:
    uint64_t cur_height;
    uint64_t max_height;
    float rate;
};

static void dump_chains(std::string &result, std::map<uint32_t, xsync_progress_t> &tables_progress) {
    for (auto &it: tables_progress) {
        result += "\t\t\t";
        result += std::to_string(it.first);
        result += "\t\t";
        result += std::to_string(it.second.cur_height);
        result += "\t\t";
        result += std::to_string(it.second.max_height);
        result += "\t\t";

        char tmp[100] = {0};
        sprintf(tmp, "%.2f", it.second.rate);
        result += tmp;
        result += "%";
        result += "\n";
    }
    result += "\n";
}

std::string xtop_sync_object::help() const {
    std::string helpinfo = "\
NAME:\n\
    sync\n\n\
COMMANDS:\n\
    help                         Show a list of commands or help for one command.\n\
    status                     Get the Sync Status\n";

    return helpinfo;
}

std::string xtop_sync_object::status() const {
    std::string result;
    xsync_roles_t roles = m_role_chains_mgr->get_roles();

    for (int32_t i = enum_chain_sync_policy_full; i >= 0; i--) {
        bool display_zec = false;
        bool display_shard = false;        
        uint64_t total_beacon_cur_height = 0;
        uint64_t total_beacon_max_height = 0;

        uint64_t total_zec_cur_height = 0;
        uint64_t total_zec_max_height = 0;

        uint64_t total_shard_cur_height = 0;
        uint64_t total_shard_max_height = 0;

        uint64_t total_cur_height = 0;
        uint64_t total_max_height = 0;

        std::map<uint32_t, xsync_progress_t> beacon_tables_progress;
        std::map<uint32_t, xsync_progress_t> zec_tables_progress;
        std::map<uint32_t, xsync_progress_t> shard_tables_progress;
        for (const auto &role_it: roles) {
            const vnetwork::xvnode_address_t &self_addr = role_it.first;
            if (common::has<common::xnode_type_t::zec>(self_addr.type()) ||
                common::has<common::xnode_type_t::edge>(self_addr.type()) ||
                common::has<common::xnode_type_t::consensus_auditor>(self_addr.type()) ||
                common::has<common::xnode_type_t::consensus_validator>(self_addr.type()) ||
                common::has<common::xnode_type_t::storage>(self_addr.type())
            ) {
                display_zec = true;
            }

            if (common::has<common::xnode_type_t::consensus_auditor>(self_addr.type()) ||
                common::has<common::xnode_type_t::consensus_validator>(self_addr.type()) ||
                common::has<common::xnode_type_t::storage>(self_addr.type())
            ) {
                display_shard = true;
            }

            const std::shared_ptr<xrole_chains_t> &role_chains = role_it.second;
            const map_chain_info_t &chains = role_chains->get_chains_wrapper().get_chains();
            for (const auto &it: chains) {
                const std::string &address = it.first;

                std::string table_prefix;
                uint32_t table_id = 0;
                if (!data::xdatautil::extract_parts(address, table_prefix, table_id))
                    continue;

                if (it.second.sync_policy != i) {
                    continue;
                }

                base::xauto_ptr<base::xvblock_t> latest_block = m_sync_store->get_latest_cert_block(address);
                xsync_progress_t info;
                info.cur_height = m_sync_store->get_latest_end_block_height(address, (enum_chain_sync_policy)i);
                info.max_height = latest_block->get_height();

                if (info.max_height == 0) {
                    info.rate = 100;
                } else {
                    info.rate = (double)info.cur_height*100/(double)info.max_height;
                }

                if (table_prefix == sys_contract_beacon_table_block_addr) {
                    total_beacon_cur_height += info.cur_height;
                    total_beacon_max_height += info.max_height;
                    beacon_tables_progress.insert(std::make_pair(table_id, info));

                } else if (table_prefix == sys_contract_zec_table_block_addr) {

                    if (display_zec) {
                        total_zec_cur_height += info.cur_height;
                        total_zec_max_height += info.max_height;
                        zec_tables_progress.insert(std::make_pair(table_id, info));
                    }

                } else if (table_prefix == sys_contract_sharding_table_block_addr) {

                    if (display_shard) {
                        total_shard_cur_height += info.cur_height;
                        total_shard_max_height += info.max_height;
                        shard_tables_progress.insert(std::make_pair(table_id, info));
                    }
                }
            }
        }

        total_cur_height += total_beacon_cur_height;
        total_cur_height += total_zec_cur_height;
        total_cur_height += total_shard_cur_height;

        total_max_height += total_beacon_max_height;
        total_max_height += total_zec_max_height;
        total_max_height += total_shard_max_height;

        // time
        if ((shard_tables_progress.empty() && zec_tables_progress.empty() && beacon_tables_progress.empty())){
            continue;
        }

        // total
        if (i == enum_chain_sync_policy_fast) {
            result += "fast-sync-mode, total:";
        } else {
            result += "full-sync-mode, total:";
        }

        if (total_max_height == 0) {
            result += "100.00%";
        } else {
            char tmp[100] = {0};
            float f = (double)total_cur_height*100/(double)total_max_height;
            sprintf(tmp, "%.2f", f);
            result += tmp;
            result += "%";
        }
        result += "\n";

        result += "\t\t\t";
        result += "index\t\t";
        result += "cur_height\t";
        result += "max_height\n";

        if (!beacon_tables_progress.empty()) {
            result += "root-beacon chains\t\t\t\t\t\t\t";

            if (total_beacon_max_height == 0) {
                result += "100.00%";
            } else {
                char tmp[100] = {0};
                float f = (double)total_beacon_cur_height*100/(double)total_beacon_max_height;
                sprintf(tmp, "%.2f", f);
                result += tmp;
                result += "%";
            }
            result += "\n";

            dump_chains(result, beacon_tables_progress);
        }

        if (!zec_tables_progress.empty()) {
            result += "sub-beacon chains\t\t\t\t\t\t\t";

            if (total_zec_max_height == 0) {
                result += "100.00%";
            } else {
                char tmp[100] = {0};
                float f = (double)total_zec_cur_height*100/(double)total_zec_max_height;
                sprintf(tmp, "%.2f", f);
                result += tmp;
                result += "%";
            }
            result += "\n";

            dump_chains(result, zec_tables_progress);
        }

        if (!shard_tables_progress.empty()) {

            result += "shard chains\t\t\t\t\t\t\t\t";

            if (total_shard_max_height == 0) {
                result += "100.00%";
            } else {
                char tmp[100] = {0};
                float f = (double)total_shard_cur_height*100/(double)total_shard_max_height;
                sprintf(tmp, "%.2f", f);
                result += tmp;
                result += "%";
            }
            result += "\n";

            dump_chains(result, shard_tables_progress);
        }
    }

    return result;
}
std::string xtop_sync_object::auto_prune_data(const std::string& prune) const {
    std::string prune_enable = prune;
    top::base::xstring_utl::tolower_string(prune_enable);
    if (prune_enable == "on")
        base::xvchain_t::instance().enable_auto_prune(true);
    else
        base::xvchain_t::instance().enable_auto_prune(false);    
    return "";
}
std::map<std::string, std::vector<std::string>> xtop_sync_object::get_neighbors() const {
    return m_peerset->get_neighbors();
}

void xtop_sync_object::add_vnet(const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> &vnetwork_driver) {
    m_sync_netmsg_dispatcher->watch(vnetwork_driver.get());

    mbus::xevent_ptr_t ev = make_object_ptr<mbus::xevent_role_add_t>(vnetwork_driver);
    m_bus->push_event(ev);
}

void xtop_sync_object::remove_vnet(const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> &vnetwork_driver) {
    m_sync_netmsg_dispatcher->unwatch(vnetwork_driver.get());

    mbus::xevent_ptr_t ev = make_object_ptr<mbus::xevent_role_remove_t>(vnetwork_driver);
    m_bus->push_event(ev);
}

NS_END2
