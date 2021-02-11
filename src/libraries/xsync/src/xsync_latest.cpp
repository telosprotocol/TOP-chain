// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_latest.h"
#include "xsync/xsync_util.h"
#include "xsync/xsync_log.h"

NS_BEG2(top, sync)

static const uint32_t TIMER_INTERVAl = 300;

xsync_latest_t::xsync_latest_t(const std::string &vnode_id, const observer_ptr<base::xvcertauth_t> &certauth, xsync_store_face_t *sync_store, xrole_chains_mgr_t *role_chains_mgr, xsync_sender_t *sync_sender):
m_vnode_id(vnode_id),
m_certauth(certauth),
m_sync_store(sync_store),
m_role_chains_mgr(role_chains_mgr),
m_sync_sender(sync_sender) {

    m_last_send_time = base::xtime_utl::gmttime_ms();
}

void xsync_latest_t::add_role(const vnetwork::xvnode_address_t& addr) {

    common::xnode_type_t type = real_part_type(addr.type());
    if (type == common::xnode_type_t::frozen || type == common::xnode_type_t::edge)
        return;

    xsync_dbg("xsync_latest_t::add_role %s", addr.to_string().c_str());

    xsync_roles_t roles = m_role_chains_mgr->get_roles();
    for (const auto &role_it: roles) {
        if (real_part_type(role_it.first.type()) == type) {
            const vnetwork::xvnode_address_t &self_addr = role_it.first;
            const std::shared_ptr<xrole_chains_t> &role_chains = role_it.second;

            std::vector<xlatest_block_info_t> info_list;

            const map_chain_info_t &chains = role_chains->get_chains_wrapper().get_chains();
            for (const auto &it: chains) {

                base::xauto_ptr<base::xvblock_t> latest_block = m_sync_store->get_latest_cert_block(it.second.address);
                if (latest_block == nullptr)
                    continue;

                xlatest_block_info_t info;
                info.address = it.second.address;
                info.height = latest_block->get_height();
                info.view_id = latest_block->get_viewid();
                info.hash = latest_block->get_block_hash();
                info_list.push_back(info);
            }

            m_sync_sender->send_latest_block_info(info_list, self_addr, 2, enum_latest_block_info_target_type::neighbor);
            m_sync_sender->send_latest_block_info(info_list, self_addr, 2, enum_latest_block_info_target_type::archive);
            return;
        }
    }
}

void xsync_latest_t::on_timer() {

    int64_t now = base::xtime_utl::gmttime_ms();
    if ((now - m_last_send_time) < TIMER_INTERVAl*1000)
        return;

    xsync_dbg("xsync_latest_t::on_timer");

    m_last_send_time = now;

    std::vector<xlatest_block_info_t> info_list;

    xsync_roles_t roles = m_role_chains_mgr->get_roles();
    for (const auto &role_it: roles) {

        const vnetwork::xvnode_address_t &self_addr = role_it.first;
        const std::shared_ptr<xrole_chains_t> &role_chains = role_it.second;

        common::xnode_type_t role_type = real_part_type(self_addr.type());

        if (common::has<common::xnode_type_t::rec>(role_type) || common::has<common::xnode_type_t::zec>(role_type) || 
            common::has<common::xnode_type_t::validator>(role_type) || common::has<common::xnode_type_t::auditor>(role_type) || common::has<common::xnode_type_t::archive>(role_type)) {

            std::vector<xlatest_block_info_t> info_list;

            const map_chain_info_t &chains = role_chains->get_chains_wrapper().get_chains();
            for (const auto &it: chains) {

                base::xauto_ptr<base::xvblock_t> latest_block = m_sync_store->get_latest_cert_block(it.second.address);
                if (latest_block == nullptr)
                    continue;

                xlatest_block_info_t info;
                info.address = it.second.address;
                info.height = latest_block->get_height();
                info.view_id = latest_block->get_viewid();
                info.hash = latest_block->get_block_hash();
                info_list.push_back(info);
            }

            // neighbor only
            m_sync_sender->send_latest_block_info(info_list, self_addr, 1, enum_latest_block_info_target_type::neighbor);
        }
    }  
}

void xsync_latest_t::handle_latest_block_info(const std::vector<xlatest_block_info_t> &info_list, const vnetwork::xvnode_address_t &from_address, const vnetwork::xvnode_address_t &network_self) {

    //xsync_dbg("xsync_latest_t handle_latest_block_info from %s count(%d)", from_address.to_string().c_str(), info_list.size());

    std::vector<xlatest_block_info_t> info_list_rsp;

    for (auto &it: info_list) {
        const xlatest_block_info_t &info = it;
        const std::string &address = info.address;

        if (!m_role_chains_mgr->exists(address))
            continue;

        base::xauto_ptr<base::xvblock_t> latest_block = m_sync_store->get_latest_cert_block(address);

        if (latest_block == nullptr)
            continue;

        if (info.view_id > latest_block->get_viewid()) {
            xsync_info("xsync_latest::handle_latest_block_info local is lower %s, local:%lu,%lu peer:%lu,%lu %s %s", 
                info.address.c_str(), latest_block->get_height(), latest_block->get_viewid(), info.height, info.view_id,
                network_self.to_string().c_str(), from_address.to_string().c_str());

            xlatest_block_item_t item;
            item.height = info.height;
            item.hash = info.hash;

            std::vector<xlatest_block_item_t> list;
            list.push_back(item);

            m_sync_sender->send_get_latest_blocks(address, list, network_self, from_address);
        } else if (info.view_id < latest_block->get_viewid()) {

            xsync_info("xsync_latest::handle_latest_block_info local is higher %s, local:%lu,%lu peer:%lu,%lu %s %s", 
                info.address.c_str(), latest_block->get_height(), latest_block->get_viewid(), info.height, info.view_id,
                network_self.to_string().c_str(), from_address.to_string().c_str());

            xlatest_block_info_t info_rsp;
            info_rsp.address = info.address;
            info_rsp.height = latest_block->get_height();
            info_rsp.view_id = latest_block->get_viewid();
            info_rsp.hash = latest_block->get_block_hash();
            info_list_rsp.push_back(info_rsp);
        }
    }

    if (!info_list_rsp.empty())
        m_sync_sender->send_latest_block_info_to_target(info_list_rsp, network_self, from_address);
}

void xsync_latest_t::handle_latest_blocks(std::vector<data::xblock_ptr_t> &blocks, const vnetwork::xvnode_address_t &from_address) {

    data::xblock_ptr_t &block = blocks[0];

    if (!m_role_chains_mgr->exists(block->get_account()))
        return;

    xsync_info("xsync_latest::handle_latest_block recv from %s", from_address.to_string().c_str());

    base::xauto_ptr<base::xvblock_t> latest_block = m_sync_store->get_latest_cert_block(block->get_account());

    if (block->get_height() > latest_block->get_height() || block->get_viewid() > latest_block->get_viewid()) {

        if (check_auth(m_certauth, block)) {
            if (m_sync_store->store_block(block.get())) {
                xsync_info("xsync_latest::handle_latest_block store block succ %s", block->dump().c_str());
            } else {
                xsync_info("xsync_latest::handle_latest_block store block failed %s", block->dump().c_str());
            }
        } else {
            xsync_info("xsync_latest::handle_latest_block auth failed %s", block->dump().c_str());
        }
    } else {
        xsync_info("xsync_latest::handle_latest_block ignore block %s", block->dump().c_str());
    }
}

NS_END2