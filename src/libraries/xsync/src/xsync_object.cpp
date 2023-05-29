// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_object.h"

#include "xconfig/xconfig_register.h"
#include "xdata/xnative_contract_address.h"
#include "xmbus/xevent_role.h"
#include "xsyncbase/xsync_policy.h"

NS_BEG2(top, sync)

xtop_sync_object::xtop_sync_object(observer_ptr<mbus::xmessage_bus_face_t> const & bus,
                                   observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                                   xobject_ptr_t<base::xvblockstore_t> &blockstore,
                                   xobject_ptr_t<base::xvcertauth_t> &cert_ptr,
                                   observer_ptr<base::xiothread_t> const & sync_thread,
                                   std::vector<observer_ptr<base::xiothread_t>> const & sync_account_thread_pool,
                                   std::vector<observer_ptr<base::xiothread_t>> const & sync_handler_thread_pool):
    m_bus{ bus }, m_instance(vhost->account_address().to_string())
  ,
    m_store_shadow(top::make_unique<sync::xsync_store_shadow_t>()),
    m_sync_store(top::make_unique<sync::xsync_store_t>(m_instance, make_observer(blockstore), m_store_shadow.get())),
    m_blacklist(top::make_unique<sync::xdeceit_node_manager_t>()),
    m_session_mgr(top::make_unique<sync::xsync_session_manager_t>(XGET_CONFIG(executor_max_sessions), SYNC_SESSION_TIMEOUT)),
    //m_session_mgr(top::make_unique<sync::xsession_manager_t>(XGET_CONFIG(executor_max_sessions))),
    m_role_chains_mgr(top::make_unique<sync::xrole_chains_mgr_t>(m_instance)),
    m_role_xips_mgr(top::make_unique<sync::xrole_xips_manager_t>(m_instance)),
    m_sync_sender(top::make_unique<sync::xsync_sender_t>(m_instance, vhost, m_role_xips_mgr.get(), m_sync_store.get(), m_session_mgr.get())),
    m_sync_ratelimit(top::make_unique<sync::xsync_ratelimit_t>(sync_thread, (uint32_t)200)),
    m_peerset(top::make_unique<sync::xsync_peerset_t>(m_instance)),
    m_sync_pusher(top::make_unique<sync::xsync_pusher_t>(m_instance, m_role_xips_mgr.get(), m_sync_sender.get(), m_role_chains_mgr.get(), m_sync_store.get())),
    m_downloader(top::make_unique<sync::xdownloader_t>(m_instance, m_sync_store.get(), make_observer(cert_ptr), m_role_xips_mgr.get(), m_role_chains_mgr.get(),
        m_sync_sender.get(), sync_account_thread_pool, m_sync_ratelimit.get(), m_store_shadow.get())),
    m_block_fetcher(top::make_unique<sync::xblock_fetcher_t>(m_instance, sync_thread, make_observer(cert_ptr), m_role_chains_mgr.get(), m_sync_store.get(),
        m_sync_sender.get())),
    m_sync_gossip(top::make_unique<sync::xsync_gossip_t>(m_instance, m_sync_store.get(), m_role_chains_mgr.get(), m_role_xips_mgr.get(), m_sync_sender.get())),
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
                m_sync_sender.get(),
                m_sync_on_demand.get(),
                m_peerset.get(),
                m_peer_keeper.get(),
                m_behind_checker.get(),
                m_cross_cluster_chain_state.get())),
    m_sync_event_dispatcher(make_object_ptr<sync::xsync_event_dispatcher_t>(
            sync_thread,
            m_instance,
            m_bus,
            m_sync_handler.get())),
    m_sync_netmsg_dispatcher(top::make_unique<sync::xsync_netmsg_dispatcher_t>(m_instance, sync_handler_thread_pool, m_bus, vhost, m_sync_handler.get())){
        xtop_sync_out_object::instance().set_xsync_shadow(m_store_shadow.get());
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
    uint64_t peer_max_height;
    float rate;
};

static void dump_chains(std::string &result, const std::map<uint32_t, xsync_progress_t> &tables_progress) {
    for (auto &it: tables_progress) {
        result += "\t\t\t";
        result += std::to_string(it.first);
        result += "\t\t";
        result += std::to_string(it.second.cur_height);
        result += "\t\t";
        result += std::to_string(it.second.peer_max_height);
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
void xtop_sync_object::display_init(std::map<common::xenum_node_type, xsync_table_data>& table_display) const {
    xsync_table_data table_data;
    table_display[common::xnode_type_t::rec] = table_data;
    table_display[common::xnode_type_t::rec].display = true;
    table_display[common::xnode_type_t::zec] = table_data;
    table_display[common::xnode_type_t::consensus] = table_data;
    table_display[common::xnode_type_t::evm] = table_data;
    table_display[common::xnode_type_t::relay] = table_data;
}
common::xenum_node_type xtop_sync_object::get_table_type(const common::xtable_address_t& account) const {
    if (account.base_address() == common::rec_table_base_address) {
        return common::xnode_type_t::rec;
    } else if (account.base_address() == common::zec_table_base_address) {
        return common::xnode_type_t::zec;
    } else if (account.base_address() == common::con_table_base_address) {
        return common::xnode_type_t::consensus;
    } else if (account.base_address() == common::eth_table_base_address) {
        return common::xnode_type_t::evm;
    } else if (account.base_address() == common::relay_table_base_address) {
        return common::xnode_type_t::relay;
    }
    xdbg("unknown table type: %s", account.to_string().c_str());
    return common::xnode_type_t::invalid;
}
std::string xtop_sync_object::get_title(common::xenum_node_type type) const {
    if (type == common::xnode_type_t::rec) {
        return "root-beacon";
    } else if (type == common::xnode_type_t::zec) {
        return "sub-beacon";
    } else if (type == common::xnode_type_t::consensus) {
        return "shard";
    } else if (type == common::xnode_type_t::evm) {
        return "evm";
    } else if (type == common::xnode_type_t::relay) {
        return "relay";
    }
    return "";
}
std::string xtop_sync_object::status() const {
    std::string result;
    xsync_roles_t roles = m_role_chains_mgr->get_roles();

    for (int32_t i = enum_chain_sync_policy_checkpoint; i >= 0; i--) {
        uint64_t total_cur_height = 0;
        uint64_t total_max_height = 0;

        std::map<common::xnode_type_t, std::map<uint32_t, xsync_progress_t>> tables_progress;
        std::map<common::xnode_type_t, xsync_table_data> table_display;
        display_init(table_display);

        for (const auto &role_it: roles) {
            const vnetwork::xvnode_address_t &self_addr = role_it.first;
            if (common::has<common::xnode_type_t::zec>(self_addr.type()) ||
                common::has<common::xnode_type_t::edge>(self_addr.type()) ||
                common::has<common::xnode_type_t::consensus_auditor>(self_addr.type()) ||
                common::has<common::xnode_type_t::consensus_validator>(self_addr.type()) ||
                common::has<common::xnode_type_t::storage>(self_addr.type()) || 
                common::has<common::xnode_type_t::fullnode>(self_addr.type())
            ) {
                table_display[common::xnode_type_t::zec].display = true;
            }

            if (common::has<common::xnode_type_t::consensus_auditor>(self_addr.type()) ||
                common::has<common::xnode_type_t::consensus_validator>(self_addr.type()) ||
                common::has<common::xnode_type_t::storage>(self_addr.type()) || 
                common::has<common::xnode_type_t::fullnode>(self_addr.type())
            ) {
                table_display[common::xnode_type_t::consensus].display = true;
            }
            if (common::has<common::xnode_type_t::evm>(self_addr.type()))
                table_display[common::xnode_type_t::evm].display = true;
            if (common::has<common::xnode_type_t::relay>(self_addr.type()))
                table_display[common::xnode_type_t::relay].display = true;

            const std::shared_ptr<xrole_chains_t> &role_chains = role_it.second;
            const map_chain_info_t &chains = role_chains->get_chains_wrapper().get_chains();
            for (const auto &it: chains) {
                const std::string &address = it.first;
                common::xaccount_address_t _account(address);

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
                info.peer_max_height = info.max_height;
                uint64_t peer_start_height = 0;
                common::xnode_address_t peer_addr;
                m_peerset->get_newest_peer(self_addr, address, peer_start_height, info.peer_max_height, peer_addr);

                if (info.peer_max_height == 0) {
                    info.rate = 100;
                } else {
                    info.rate = (double)info.cur_height*100/(double)info.peer_max_height;
                }

                tables_progress[get_table_type(_account.table_address())].insert(std::make_pair(table_id, info));
                table_display[get_table_type(_account.table_address())].total_cur_height += info.cur_height;
                table_display[get_table_type(_account.table_address())].total_max_height += info.peer_max_height;
            }
        }
        for (auto it : table_display) {
            total_cur_height += it.second.total_cur_height;
            total_max_height += it.second.total_max_height;
        }
        xdbg("sync_status: %d, %llu,%llu", i, total_cur_height, total_max_height);

        if (tables_progress.empty())
            continue;

        // total
        if (i == enum_chain_sync_policy_fast) {
            result += "fast-sync-mode, total:";
        } else if (i == enum_chain_sync_policy_full) {
            result += "full-sync-mode, total:";
        } else if (i == enum_chain_sync_policy_checkpoint) {
            result += "checkpoint-sync-mode, total:";
        } else {
            continue;
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

        for (auto const &it:tables_progress) {
            if (it.second.empty())
                continue;

            result += get_title(it.first) + " chains\t\t\t\t\t\t";

            if (table_display[it.first].total_max_height == 0) {
                result += "100.00%";
            } else {
                char tmp[100] = {0};
                float f = (double)table_display[it.first].total_cur_height * 100 / (double)table_display[it.first].total_max_height;
                sprintf(tmp, "%.2f", f);
                result += tmp;
                result += "%";
            }
            result += "\n";

            dump_chains(result, it.second);
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

void xtop_sync_object::add_vnet(const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> &vnetwork_driver,
    const common::xminer_type_t miner_type, const bool genesis) {
    m_sync_netmsg_dispatcher->watch(vnetwork_driver.get());

    mbus::xevent_ptr_t ev = make_object_ptr<mbus::xevent_role_add_t>(vnetwork_driver, miner_type, genesis);
    m_bus->push_event(ev);
}

void xtop_sync_object::remove_vnet(const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> &vnetwork_driver,
    const common::xminer_type_t miner_type, const bool genesis) {
    m_sync_netmsg_dispatcher->unwatch(vnetwork_driver.get());

    mbus::xevent_ptr_t ev = make_object_ptr<mbus::xevent_role_remove_t>(vnetwork_driver, miner_type, genesis);
    m_bus->push_event(ev);
}

xtop_sync_out_object& xtop_sync_out_object::instance() {
    static xtop_sync_out_object __global_sync_out_instance;
    return __global_sync_out_instance;
}

void xtop_sync_out_object::set_xsync_shadow(top::sync::xsync_store_shadow_t * shadow) {
    m_store_shadow = shadow;
}

void xtop_sync_out_object::save_span() {
    if (m_store_shadow != NULL)
        m_store_shadow->save();
}
NS_END2
