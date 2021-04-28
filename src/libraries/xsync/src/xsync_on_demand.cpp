// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_on_demand.h"
#include "xsync/xsync_log.h"
#include "xmbus/xevent_behind.h"
#include "xmbus/xevent_sync.h"
#include "xsync/xsync_util.h"
#include "xdata/xfull_tableblock.h"

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
    auto bme = dynamic_xobject_ptr_cast<mbus::xevent_behind_on_demand_t>(e);
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

    std::map<std::string, std::string> context;
    context["src"] = self_addr.to_string();
    context["dst"] = target_addr.to_string();
    context["consensus"] = std::to_string(is_consensus);
    bool permit = m_download_tracer.apply(address, std::make_pair(start_height, start_height + count), context);
    if (permit) {
        m_sync_sender->send_get_on_demand_blocks(address, start_height, count, is_consensus, self_addr, target_addr);
    } else {
        xsync_info("xsync_on_demand_t::on_behind_event is not permit because of overflow or during downloading, account: %s",
        address.c_str());
    }
}

void xsync_on_demand_t::handle_blocks_response(const std::vector<data::xblock_ptr_t> &blocks,
    const vnetwork::xvnode_address_t &to_address, const vnetwork::xvnode_address_t &network_self) {

    if (blocks.empty()) {
        m_download_tracer.expire();
        return;
    }

    std::string account = blocks[0]->get_account();
    int ret = check(account);
    if (ret != 0) {
        xsync_warn("xsync_on_demand_t::on_response_event check failed %s,ret=%d", account.c_str(), ret);
        return;
    }

    ret = check(account, to_address, network_self);
    if (ret != 0) {
        xsync_warn("xsync_on_demand_t::on_response_event check the source of message failed %s,ret=%d", account.c_str(), ret);
        return;
    }

    for (uint32_t i = 0; i< blocks.size(); i++) {
        xblock_ptr_t block = blocks[i];

        if (block->get_account() != account) {
            xsync_warn("xsync_handler receive on_demand_blocks(address error) (%s, %s)",
                block->get_account().c_str(), account.c_str());
            return;
        }

        if (!check_auth(m_certauth, block)) {
            xsync_info("xsync_on_demand_t::on_demand_blocks auth_failed %s,height=%lu,viewid=%lu,",
                block->get_account().c_str(), block->get_height(), block->get_viewid());
            return;
        }

        base::xvblock_t* vblock = dynamic_cast<base::xvblock_t*>(block.get());
        if (m_sync_store->store_block(vblock)) {
            xsync_info("xsync_on_demand_t::on_demand_blocks succ %s,height=%lu,viewid=%lu,",
                block->get_account().c_str(), block->get_height(), block->get_viewid());
        } else {
            xsync_info("xsync_on_demand_t::on_demand_blocks failed %s,height=%lu,viewid=%lu,",
                block->get_account().c_str(), block->get_height(), block->get_viewid());
        }
    }

    if (!m_download_tracer.refresh(account, blocks.rbegin()->get()->get_height())) {
        return;
    }

    base::xauto_ptr<base::xvblock_t> table_block = m_sync_store->get_latest_start_block(account, enum_chain_sync_pocliy_fast);
    if (table_block != nullptr){
        data::xblock_ptr_t current_block = autoptr_to_blockptr(table_block);
        xsync_message_chain_snapshot_meta_t chain_snapshot_meta{account, table_block->get_height()};
        if(!current_block->is_full_state_block()){
            xsync_warn("xsync_handler::on_demand_blocks request account(%s)'s snapshot, height is %llu",
                current_block->get_account().c_str(), current_block->get_height());
            m_sync_sender->send_chain_snapshot_meta(chain_snapshot_meta, xmessage_id_sync_ondemand_chain_snapshot_request, network_self, to_address);
            return;
        }
    }

    xsync_download_tracer tracer;
    if (!m_download_tracer.get(account, tracer)){
        return;
    }

    std::map<std::string, std::string> context = tracer.context();
    bool is_consensus = std::stoi(context["consensus"]);
    int32_t count = tracer.height_interval().second - tracer.trace_height();
    if (count > 0) {
        m_sync_sender->send_get_on_demand_blocks(account, tracer.trace_height(), count, is_consensus, network_self, to_address);
    } else {
        on_response_event(account);
    }
}


void xsync_on_demand_t::handle_blocks_request(const xsync_message_get_on_demand_blocks_t &block,
    const vnetwork::xvnode_address_t &to_address, const vnetwork::xvnode_address_t &network_self) {
    std::string address = block.address;
    uint64_t start_height = block.start_height;
    uint64_t end_height = 0;
    uint32_t heights = block.count;
    bool is_consensus = block.is_consensus;

    if (heights == 0)
        return;

    std::vector<data::xblock_ptr_t> blocks;

    if (is_consensus) {
        base::xauto_ptr<base::xvblock_t> latest_full_block = m_sync_store->get_latest_full_block(address);
        if (latest_full_block != nullptr && latest_full_block->get_height() > start_height) {
            start_height = latest_full_block->get_height() + 1;
            xblock_ptr_t block_ptr = autoptr_to_blockptr(latest_full_block);
            blocks.push_back(block_ptr);
        }

        end_height = start_height + (uint64_t)heights;
    } else {
        end_height = start_height + (uint64_t)heights;
    }

    for (uint64_t height = start_height, i = 0; (height <= end_height) && (i < max_request_block_count); height++) {
        auto need_blocks = m_sync_store->load_block_objects(address, height);
        for (uint32_t j = 0; j < need_blocks.size(); j++, i++){
            blocks.push_back(xblock_t::raw_vblock_to_object_ptr(need_blocks[j].get()));
        }
    }

    m_sync_sender->send_on_demand_blocks(blocks, network_self, to_address);
}

void xsync_on_demand_t::handle_chain_snapshot_meta(xsync_message_chain_snapshot_meta_t &chain_meta,
    const vnetwork::xvnode_address_t &to_address, const vnetwork::xvnode_address_t &network_self) {

    std::string account = chain_meta.m_account_addr;

    base::xauto_ptr<base::xvblock_t> blk = m_sync_store->load_block_object(account, chain_meta.m_height_of_fullblock);
    if (blk != nullptr) {
        xfull_tableblock_t* full_block_ptr = dynamic_cast<xfull_tableblock_t*>(xblock_t::raw_vblock_to_object_ptr(blk.get()).get());
        if ((full_block_ptr != nullptr) && (full_block_ptr->is_full_state_block())) {
            base::xvboffdata_t* _offdata = full_block_ptr->get_offdata();
            xobject_ptr_t<base::xvboffdata_t> offdata_ptr;
            offdata_ptr.attach(_offdata);
            _offdata->add_ref();
            xsync_message_chain_snapshot_t chain_snapshot(chain_meta.m_account_addr,
                offdata_ptr, chain_meta.m_height_of_fullblock);
            m_sync_sender->send_chain_snapshot(chain_snapshot, xmessage_id_sync_ondemand_chain_snapshot_response, network_self, to_address);
        } else {
            xsync_info("xsync_handler receive ondemand_chain_snapshot_request, account:%s, height:%llu, block_type:%d",
                account.c_str(), chain_meta.m_height_of_fullblock, blk->get_block_class());
        }
    } else {
        xsync_info("xsync_handler receive ondemand_chain_snapshot_request, and the full block is not exist,account:%s, height:%llu",
                account.c_str(), chain_meta.m_height_of_fullblock);
    }
}


void xsync_on_demand_t::handle_chain_snapshot(xsync_message_chain_snapshot_t &chain_snapshot,
    const vnetwork::xvnode_address_t &to_address, const vnetwork::xvnode_address_t &network_self) {
    std::string account = chain_snapshot.m_tbl_account_addr;

    int32_t ret = check(account);
    if (ret != 0) {
        xsync_warn("xsync_on_demand_t::on_response_event check failed %s,ret=%d", account.c_str(), ret);
        return;
    }

    ret = check(account, to_address, network_self);
    if (ret != 0) {
        xsync_warn("xsync_on_demand_t::on_response_event check the source of message failed %s,ret=%d", account.c_str(), ret);
        return;
    }

    base::xauto_ptr<base::xvblock_t> current_vblock = m_sync_store->load_block_object(account, chain_snapshot.m_height_of_fullblock);
    data::xblock_ptr_t current_block = autoptr_to_blockptr(current_vblock);
    if (current_block->is_fullblock() && !current_block->is_full_state_block()) {
        current_block->reset_block_offdata(chain_snapshot.m_chain_snapshot.get());
        m_sync_store->store_block(current_block.get());
    }

    xsync_download_tracer tracer;
    if (!m_download_tracer.get(account, tracer)) {
        return;
    }

    std::map<std::string, std::string> context = tracer.context();
    bool is_consensus = std::stoi(context["consensus"]);
    int32_t count = tracer.height_interval().second - tracer.trace_height();
    if (count > 0) {
        m_download_tracer.refresh(account);
        m_sync_sender->send_get_on_demand_blocks(account, tracer.trace_height(), count, is_consensus, network_self, to_address);
    } else {
        on_response_event(account);
    }
}

void xsync_on_demand_t::on_response_event(const std::string account) {
    mbus::xevent_ptr_t e = make_object_ptr<mbus::xevent_sync_complete_t>(account);
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

int xsync_on_demand_t::check(const std::string &account_address,
    const vnetwork::xvnode_address_t &to_address, const vnetwork::xvnode_address_t &network_self) {
    xsync_download_tracer tracer;
    if (!m_download_tracer.get(account_address, tracer)) {
        return -1;
    }
    std::map<std::string, std::string> context = tracer.context();
    if (context["src"] != network_self.to_string()) {
        return -1;
    }
    if (context["dst"] != to_address.to_string()) {
        return -1;
    }
    return 0;
}

xsync_download_tracer_mgr* xsync_on_demand_t::download_tracer_mgr() {
    return &m_download_tracer;
}

NS_END2
