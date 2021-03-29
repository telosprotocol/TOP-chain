// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_on_demand.h"
#include "xsync/xsync_log.h"
#include "xmbus/xevent_behind.h"
#include "xmbus/xevent_sync.h"
#include "xsync/xsync_util.h"

NS_BEG2(top, sync)

using namespace data;

xsync_on_demand_t::xsync_on_demand_t(std::string vnode_id, const observer_ptr<mbus::xmessage_bus_face_t> &mbus, const observer_ptr<base::xvcertauth_t> &certauth,
        xsync_store_face_t *sync_store, xrole_chains_mgr_t *role_chains_mgr, xrole_xips_manager_t *role_xips_mgr, xsync_sender_t *sync_sender):
m_vnode_id(vnode_id),
m_mbus(mbus),
m_certauth(certauth),
m_sync_store(sync_store),
m_role_chains_mgr(role_chains_mgr),
m_role_xips_mgr(role_xips_mgr),
m_sync_sender(sync_sender) {
}

void xsync_on_demand_t::on_behind_event(const mbus::xevent_ptr_t &e) {
    auto bme = std::static_pointer_cast<mbus::xevent_behind_on_demand_t>(e);
    std::string address = bme->address;
    uint64_t start_height = bme->start_height;
    uint32_t count = bme->count;
    bool is_consensus = bme->is_consensus;
    const std::string &reason = bme->reason;

    if (count == 0)
        return;

    int ret = check(address);
    if (ret != 0) {
        xsync_warn("xsync_on_demand_t::on_behind_event check failed %s,ret=%d", address.c_str(), ret);
        return;
    }

    vnetwork::xvnode_address_t self_addr;
    if (!m_role_xips_mgr->get_self_addr(self_addr)) {
        xsync_warn("xsync_on_demand_t::on_behind_event get self addr failed %s,reason:%s", address.c_str(), reason.c_str());
        return;
    }

    // only one archive node
    std::vector<vnetwork::xvnode_address_t> archive_list = m_role_xips_mgr->get_rand_archives(1);
    if (archive_list.size() == 0) {
        xsync_warn("xsync_on_demand_t::on_behind_event no archive node %s,", address.c_str());
        return;
    }

    const vnetwork::xvnode_address_t &target_addr = archive_list[0];

    xsync_info("xsync_on_demand_t::on_behind_event send sync request(on_demand) %s,range(%lu,%lu) is_consensus(%d) %s",
        address.c_str(), start_height, start_height+count, is_consensus, target_addr.to_string().c_str());

    m_sync_sender->send_get_on_demand_blocks(address, start_height, count, is_consensus, self_addr, target_addr);
}

void xsync_on_demand_t::on_response_event(const std::vector<data::xblock_ptr_t> &blocks) {

    std::string address = blocks[0]->get_account();

    int ret = check(address);
    if (ret != 0) {
        xsync_warn("xsync_on_demand_t::on_response_event check failed %s,ret=%d", address.c_str(), ret);
        return;
    }

    for (auto &it: blocks) {
        xblock_ptr_t block = it;

        if (!check_auth(m_certauth, block)) {
            xsync_info("xsync_on_demand_t::on_response_event auth_failed %s,height=%lu,viewid=%lu,",
                block->get_account().c_str(), block->get_height(), block->get_viewid());
            return;
        }

        base::xvblock_t* vblock = dynamic_cast<base::xvblock_t*>(block.get());

        if (m_sync_store->store_block(vblock)) {
            xsync_info("xsync_on_demand_t::on_response_event succ %s,height=%lu,viewid=%lu,",
                block->get_account().c_str(), block->get_height(), block->get_viewid());
        } else {
            xsync_info("xsync_on_demand_t::on_response_event failed %s,height=%lu,viewid=%lu,",
                block->get_account().c_str(), block->get_height(), block->get_viewid());
            return;
        }
    }

    mbus::xevent_ptr_t e = std::make_shared<mbus::xevent_sync_complete_t>(address);
    m_mbus->push_event(e);
}

int xsync_on_demand_t::check(const std::string &account_address) {

    std::string table_address;
    bool is_unit_address = data::is_unit_address(common::xaccount_address_t{account_address});
    if (is_unit_address) {
        table_address = account_address_to_block_address(top::common::xaccount_address_t{account_address});
    } else {
        table_address = account_address;
    }

    if (!m_role_chains_mgr->exists(table_address)) {
        return -1;
    }

    return 0;
}

NS_END2