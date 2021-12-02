// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_on_demand.h"
#include "xsync/xsync_log.h"
#include "xmbus/xevent_behind.h"
#include "xmbus/xevent_sync.h"
#include "xsync/xsync_util.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xtable_bstate.h"
#include "xvledger/xvblockstore.h"
#include "xchain_fork/xchain_upgrade_center.h"
#include "xvledger/xunit_proof.h"

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
    bool unit_proof = bme->unit_proof;

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
        address.c_str(), start_height, start_height + count -1, is_consensus, target_addr.to_string().c_str());

    std::map<std::string, std::string> context;
    context["src"] = self_addr.to_string();
    context["dst"] = target_addr.to_string();
    context["consensus"] = std::to_string(is_consensus);
    bool permit = m_download_tracer.apply(address, std::make_pair(start_height, start_height + count - 1), context);
    if (permit) {
        if (!m_sync_store->remove_empty_unit_forked()) {
            m_sync_sender->send_get_on_demand_blocks(address, start_height, count, is_consensus, self_addr, target_addr);
        } else {
            m_sync_sender->send_get_on_demand_blocks_with_proof(address, start_height, count, is_consensus, unit_proof, self_addr, target_addr);
            if (unit_proof) {
                XMETRICS_GAUGE(metrics::xsync_unit_proof_sync_req_send, 1);
            }
        }
        XMETRICS_COUNTER_INCREMENT("xsync_on_demand_download_request_remote", 1);
    } else {
        xsync_info("xsync_on_demand_t::on_behind_event is not permit because of overflow or during downloading, account: %s",
        address.c_str());
        XMETRICS_COUNTER_INCREMENT("xsync_on_demand_download_overflow", 1);
    }
}

void xsync_on_demand_t::handle_blocks_response(const std::vector<data::xblock_ptr_t> &blocks,
    const vnetwork::xvnode_address_t &to_address, const vnetwork::xvnode_address_t &network_self) {

    xsync_dbg("xsync_on_demand_t::handle_blocks_response receive blocks(on_demand) %s, %s, count %d",
        network_self.to_string().c_str(), to_address.to_string().c_str(), blocks.size());

    if (!basic_check(blocks, to_address, network_self)) {
        return;
    }

    auto & account = blocks[0]->get_account();
    bool is_table_address = data::is_table_address(common::xaccount_address_t{account});

    if (is_table_address) {
        store_on_demand_sync_blocks(blocks, "");
    } else {
        std::vector<data::xblock_ptr_t> validated_blocks;
        if (!check_unit_blocks(blocks, validated_blocks)) {
            return;
        }
        store_on_demand_sync_blocks(validated_blocks, "");
    }

    if (!m_download_tracer.refresh(account, blocks.rbegin()->get()->get_height())) {
        return;
    }

    base::xauto_ptr<base::xvblock_t> current_vblock = m_sync_store->get_latest_start_block(account, enum_chain_sync_policy_fast);
    if (current_vblock != nullptr){
        data::xblock_ptr_t current_block = autoptr_to_blockptr(current_vblock);
        xsync_message_chain_snapshot_meta_t chain_snapshot_meta{account, current_vblock->get_height()};
        if(current_block->is_tableblock() && !current_block->is_full_state_block()){
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
        m_sync_sender->send_get_on_demand_blocks(account, tracer.trace_height() + 1, count, is_consensus, network_self, to_address);
        XMETRICS_COUNTER_INCREMENT("xsync_on_demand_download_request_remote", 1);
    } else {
        m_download_tracer.expire(account);
        on_response_event(account);
    }
}

void xsync_on_demand_t::handle_blocks_response_with_proof(const std::vector<data::xblock_ptr_t> &blocks, const std::string& unit_proof_str,
    const vnetwork::xvnode_address_t &to_address, const vnetwork::xvnode_address_t &network_self) {

    xsync_dbg("xsync_on_demand_t::handle_blocks_response_with_proof receive blocks(on_demand) %s, %s, count %d",
        network_self.to_string().c_str(), to_address.to_string().c_str(), blocks.size());

    if (!basic_check(blocks, to_address, network_self)) {
        return;
    }

    auto & account = blocks[0]->get_account();
    bool is_table_address = data::is_table_address(common::xaccount_address_t{account});

    if (is_table_address) {
        if (!unit_proof_str.empty()) {
            xsync_error("xsync_on_demand_t::on_response_event table sync never need unit proof!addr:%s", account.c_str());
            return;
        }
        store_on_demand_sync_blocks(blocks, unit_proof_str);
    } else {
        if (!unit_proof_str.empty()) {
            if (!check_unit_blocks(blocks, unit_proof_str)) {
                return;
            }
            store_on_demand_sync_blocks(blocks, unit_proof_str);
        } else {
            std::vector<data::xblock_ptr_t> validated_blocks;
            if (!check_unit_blocks(blocks, validated_blocks)) {
                if (blocks.size() < 3) {
                    return;
                } else {
                    validated_blocks.clear();
                    validated_blocks.insert(validated_blocks.begin(), blocks.begin(), blocks.end() - 2);
                }
            }
            store_on_demand_sync_blocks(validated_blocks, "");
        }
    }

    if (!m_download_tracer.refresh(account, blocks.rbegin()->get()->get_height())) {
        return;
    }

    base::xauto_ptr<base::xvblock_t> current_vblock = m_sync_store->get_latest_start_block(account, enum_chain_sync_policy_fast);
    if (current_vblock != nullptr){
        data::xblock_ptr_t current_block = autoptr_to_blockptr(current_vblock);
        xsync_message_chain_snapshot_meta_t chain_snapshot_meta{account, current_vblock->get_height()};
        if(current_block->is_tableblock() && !current_block->is_full_state_block()){
            xsync_warn("xsync_handler::handle_blocks_response_with_proof request account(%s)'s snapshot, height is %llu",
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
        m_sync_sender->send_get_on_demand_blocks_with_proof(account, tracer.trace_height() + 1, count, is_consensus, false, network_self, to_address);
        XMETRICS_COUNTER_INCREMENT("xsync_on_demand_download_request_remote", 1);
    } else {
        m_download_tracer.expire(account);
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

    xsync_dbg("xsync_on_demand_t::handle_blocks_request receive request of account %s, start_height %llu, count %u",
        address.c_str(), start_height, heights);

    std::vector<data::xblock_ptr_t> blocks;

    end_height = start_height + (uint64_t)heights - 1;
    if (is_consensus) {
        base::xauto_ptr<base::xvblock_t> latest_full_block = m_sync_store->get_latest_full_block(address);
        if (latest_full_block != nullptr && latest_full_block->get_height() >= start_height &&
            end_height >= latest_full_block->get_height()) {
                xblock_ptr_t block_ptr = autoptr_to_blockptr(latest_full_block);
                blocks.push_back(block_ptr);
                start_height = latest_full_block->get_height() + 1;
        }
    }

    for (uint64_t height = start_height, i = 0; (height <= end_height) && (i < max_request_block_count); height++) {
        auto need_blocks = m_sync_store->load_block_objects(address, height);
        if (need_blocks.empty()) {
            break;
        }
        for (uint32_t j = 0; j < need_blocks.size(); j++, i++){
            blocks.push_back(xblock_t::raw_vblock_to_object_ptr(need_blocks[j].get()));
        }
    }

    if (blocks.size() != 0){
        xsync_info("xsync_on_demand_t::handle_blocks_request %s range[%llu,%llu]", address.c_str(),
            blocks.front()->get_height(), blocks.back()->get_height());
    }

    m_sync_sender->send_on_demand_blocks(blocks, xmessage_id_sync_on_demand_blocks, "on_demand_blocks", network_self, to_address);
}

void xsync_on_demand_t::handle_blocks_request_with_proof(const xsync_message_get_on_demand_blocks_with_proof_t &block,
    const vnetwork::xvnode_address_t &to_address, const vnetwork::xvnode_address_t &network_self) {
    std::string address = block.address;
    uint64_t start_height = block.start_height;
    uint64_t end_height = 0;
    uint32_t heights = block.count;
    bool is_consensus = block.is_consensus;
    bool unit_proof = block.unit_proof;

    if (unit_proof) {
        XMETRICS_GAUGE(metrics::xsync_unit_proof_sync_req_recv, 1);
    }

    if (heights == 0)
        return;

    xsync_dbg("xsync_on_demand_t::handle_blocks_request receive request of account %s, start_height %llu, count %u,unit_proof:%d",
        address.c_str(), start_height, heights, unit_proof);

    std::vector<data::xblock_ptr_t> blocks;

    end_height = start_height + (uint64_t)heights - 1;
    if (is_consensus) {
        base::xauto_ptr<base::xvblock_t> latest_full_block = m_sync_store->get_latest_full_block(address);
        if (latest_full_block != nullptr && latest_full_block->get_height() >= start_height &&
            end_height >= latest_full_block->get_height()) {
                xblock_ptr_t block_ptr = autoptr_to_blockptr(latest_full_block);
                blocks.push_back(block_ptr);
                start_height = latest_full_block->get_height() + 1;
        }
    }

    for (uint64_t height = start_height, i = 0; (height <= end_height) && (i < max_request_block_count); height++) {
        auto need_blocks = m_sync_store->load_block_objects(address, height);
        if (need_blocks.empty()) {
            break;
        }
        for (uint32_t j = 0; j < need_blocks.size(); j++, i++){
            blocks.push_back(xblock_t::raw_vblock_to_object_ptr(need_blocks[j].get()));
        }
    }

    std::string unit_proof_str;
    if (blocks.size() != 0){
        xsync_info("xsync_on_demand_t::handle_blocks_request %s range[%llu,%llu]", address.c_str(),
            blocks.front()->get_height(), blocks.back()->get_height());
        bool is_table_address = data::is_table_address(common::xaccount_address_t{address});
        if (unit_proof) {
            if (is_table_address) {
                xsync_error("xsync_on_demand_t::handle_blocks_request table sync never need unit proof!addr:%s", address.c_str());
            } else {
                base::xvaccount_t unit_account(address);
                unit_proof_str = m_sync_store->get_unit_proof(unit_account, blocks.back()->get_height());
                if(unit_proof_str.empty()){
                    xsync_info(
                        "xsync_on_demand_t::handle_blocks_request %s range[%llu,%llu] get proof fail", address.c_str(), blocks.front()->get_height(), blocks.back()->get_height());
                }
            }
        }
    }

    m_sync_sender->send_on_demand_blocks_with_proof(blocks, xmessage_id_sync_on_demand_blocks_with_proof, "on_demand_blocks", network_self, to_address, unit_proof_str);
}

void xsync_on_demand_t::handle_chain_snapshot_meta(xsync_message_chain_snapshot_meta_t &chain_meta,
    const vnetwork::xvnode_address_t &to_address, const vnetwork::xvnode_address_t &network_self) {

    std::string account = chain_meta.m_account_addr;

    xsync_dbg("xsync_on_demand_t::handle_chain_snapshot_meta receive snapshot request of account %s, height %llu",
        account.c_str(), chain_meta.m_height_of_fullblock);

    base::xauto_ptr<base::xvblock_t> blk = m_sync_store->load_block_object(account, chain_meta.m_height_of_fullblock, false);
    if ((blk != nullptr) && (blk->get_block_level() == base::enum_xvblock_level_table)) {
        if (blk->get_block_level() == base::enum_xvblock_level_table && blk->get_block_class() == base::enum_xvblock_class_full) {
            if (base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_full_block_offsnapshot(blk.get(), metrics::statestore_access_from_sync_handle_chain_snapshot_meta)) {
                std::string property_snapshot = blk->get_full_state();
                xsync_message_chain_snapshot_t chain_snapshot(chain_meta.m_account_addr,
                    property_snapshot, chain_meta.m_height_of_fullblock);
                m_sync_sender->send_chain_snapshot(chain_snapshot, xmessage_id_sync_ondemand_chain_snapshot_response, network_self, to_address);
            } else {
                xsync_warn("xsync_handler receive ondemand_chain_snapshot_request, and the full block state is not exist,account:%s, height:%llu, block_type:%d",
                    account.c_str(), chain_meta.m_height_of_fullblock, blk->get_block_class());
            }
        } else {
            xsync_error("xsync_handler receive ondemand_chain_snapshot_request, and it is not full table,account:%s, height:%llu",
                    account.c_str(), chain_meta.m_height_of_fullblock);
        }
    } else {
        xsync_info("xsync_handler receive ondemand_chain_snapshot_request, and the full block is not exist,account:%s, height:%llu",
                account.c_str(), chain_meta.m_height_of_fullblock);
    }
}


void xsync_on_demand_t::handle_chain_snapshot(xsync_message_chain_snapshot_t &chain_snapshot,
    const vnetwork::xvnode_address_t &to_address, const vnetwork::xvnode_address_t &network_self) {
    std::string account = chain_snapshot.m_tbl_account_addr;

    xsync_dbg("xsync_on_demand_t::handle_chain_snapshot_meta receive snapshot response of account %s, height %llu",
        account.c_str(), chain_snapshot.m_height_of_fullblock);

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

    base::xauto_ptr<base::xvblock_t> current_vblock = m_sync_store->load_block_object(account, chain_snapshot.m_height_of_fullblock, true);
    data::xblock_ptr_t current_block = autoptr_to_blockptr(current_vblock);
    if (current_block->is_tableblock() && current_block->is_fullblock() && !current_block->is_full_state_block()) {
        if (false == xtable_bstate_t::set_block_offsnapshot(current_vblock.get(), chain_snapshot.m_chain_snapshot)) {
            xsync_error("xsync_on_demand_t::handle_chain_snapshot invalid snapshot. block=%s", current_vblock->dump().c_str());
            return;
        }
        xsync_dbg("xsync_on_demand_t::handle_chain_snapshot valid snapshot. block=%s", current_vblock->dump().c_str());
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
        if (!m_sync_store->remove_empty_unit_forked()) {
            xdbg("xsync_on_demand_t::handle_chain_snapshot old version");
            m_sync_sender->send_get_on_demand_blocks(account, tracer.trace_height() + 1, count, is_consensus, network_self, to_address);
        } else {
            xdbg("xsync_on_demand_t::handle_chain_snapshot new version");
            m_sync_sender->send_get_on_demand_blocks_with_proof(account, tracer.trace_height() + 1, count, is_consensus, false, network_self, to_address);
        }
        XMETRICS_COUNTER_INCREMENT("xsync_on_demand_download_request_remote", 1);
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


void xsync_on_demand_t::on_behind_by_hash_event(const mbus::xevent_ptr_t &e) {
    auto bme = dynamic_xobject_ptr_cast<mbus::xevent_behind_on_demand_by_hash_t>(e);
    std::string address = bme->address;
    std::string hash = bme->hash;
    const std::string &reason = bme->reason;

    int ret = check(address);
    if (ret != 0) {
        xsync_warn("xsync_on_demand_t::on_behind_by_hash_event check failed %s,ret=%d", address.c_str(), ret);
        return;
    }

    vnetwork::xvnode_address_t self_addr;
    if (!m_role_xips_mgr->get_self_addr(self_addr)) {
        xsync_warn("xsync_on_demand_t::on_behind_by_hash_event get self addr failed %s,reason:%s", address.c_str(), reason.c_str());
        return;
    }

    // only one archive node
    std::vector<vnetwork::xvnode_address_t> archive_list = m_role_xips_mgr->get_rand_archives(1);
    if (archive_list.size() == 0) {
        xsync_warn("xsync_on_demand_t::on_behind_by_hash_event no archive node %s,", address.c_str());
        return;
    }

    const vnetwork::xvnode_address_t &target_addr = archive_list[0];

    xsync_info("xsync_on_demand_t::on_behind_by_hash_event send sync request(on_demand) %s,hash(%s)",
        address.c_str(), data::to_hex_str(hash).c_str());

    std::map<std::string, std::string> context;
    context["src"] = self_addr.to_string();
    context["dst"] = target_addr.to_string();
    bool permit = m_download_tracer.apply(address, std::make_pair(0, 20), context);
    if (permit) {
        m_sync_sender->send_get_on_demand_by_hash_blocks(address, hash, self_addr, target_addr);
    } else {
        xsync_info("xsync_on_demand_t::on_behind_by_hash_event is not permit because of overflow or during downloading, account: %s",
        address.c_str());
    }
}

void xsync_on_demand_t::handle_blocks_by_hash_response(const std::vector<data::xblock_ptr_t> &blocks,
    const vnetwork::xvnode_address_t &to_address, const vnetwork::xvnode_address_t &network_self) {

    xsync_dbg("xsync_on_demand_t::handle_blocks_by_hash_response from %s to %s, block size %d", to_address.to_string().c_str(), network_self.to_string().c_str(), blocks.size());
    if (blocks.empty()) {
        m_download_tracer.expire();
        return;
    }

    std::string account = blocks[0]->get_account();
    if (!basic_check(blocks, to_address, network_self)) {
        xsync_warn("xsync_on_demand_t::handle_blocks_by_hash_response check failed %s", account.c_str());
        return;
    }

    bool is_table_address = data::is_table_address(common::xaccount_address_t{account});
    if (!is_table_address) {
        xsync_error("xsync_on_demand_t::handle_blocks_by_hash_response synced unit blocks by hash:%s", blocks[0]->dump().c_str());
        return;
    }
    store_on_demand_sync_blocks(blocks, "");
    on_response_event(account);
    m_download_tracer.expire(account);
}

void xsync_on_demand_t::handle_blocks_by_hash_request(const xsync_message_get_on_demand_by_hash_blocks_t &block,
    const vnetwork::xvnode_address_t &to_address, const vnetwork::xvnode_address_t &network_self) {
    std::string address = block.address;
    std::string hash = block.hash;

    xsync_dbg("xsync_on_demand_t::handle_blocks_by_hash_request from %s to %s, account:%s, hash:%s", to_address.to_string().c_str(),
        network_self.to_string().c_str(), address.c_str(), data::to_hex_str(hash).c_str());
    if (hash.empty())
        return;

    std::vector<data::xblock_ptr_t> blocks;
    auto xvblocks = m_sync_store -> load_block_objects(hash, enum_transaction_subtype_recv);

    for (uint32_t i = 0; i < xvblocks.size(); i++){
        blocks.push_back(xblock_t::raw_vblock_to_object_ptr(xvblocks[i].get()));
    }
    m_sync_sender->send_on_demand_blocks(blocks, xmessage_id_sync_on_demand_by_hash_blocks, "on_demand_by_hash_blocks", network_self, to_address);
}

bool xsync_on_demand_t::basic_check(const std::vector<data::xblock_ptr_t> &blocks, const vnetwork::xvnode_address_t &to_address, const vnetwork::xvnode_address_t &network_self) {
    if (blocks.empty()) {
        m_download_tracer.expire();
        return false;
    }

    auto & account = blocks[0]->get_account();
    xsync_dbg("xsync_on_demand_t::handle_blocks_response receive blocks of account %s, count %d",
        account.c_str(), blocks.size());
    int ret = check(account, to_address, network_self);
    if (ret != 0) {
        xsync_warn("xsync_on_demand_t::on_response_event check the source of message failed %s,ret=%d", account.c_str(), ret);
        return false;
    }

    ret = check(account);
    if (ret != 0) {
        xsync_warn("xsync_on_demand_t::store_unit_blocks_without_proof check failed %s,ret=%d", account.c_str(), ret);
        return false;
    }
    
    xblock_ptr_t last_block = blocks[blocks.size() -1];

    if (!check_auth(m_certauth, last_block)) {
        xsync_error("xsync_on_demand_t::store_unit_blocks_without_proof auth_failed %s,height=%lu,viewid=%lu,",
            account.c_str(), last_block->get_height(), last_block->get_viewid());
        return false;
    }

    auto it = blocks.begin();
    auto last_hash = it->get()->get_block_hash();
    it++;
    for (; it != blocks.end(); it++) {
        auto block = it->get();
        if (block->get_account() != account || block->get_last_block_hash() != last_hash) {
            xsync_error("xsync_on_demand_t::store_unit_blocks_without_proof receive on_demand_blocks(address or hash error)(%s,%s)(%s,%s)",
                block->get_account().c_str(), account.c_str(), block->get_last_block_hash().c_str(), last_hash.c_str());
            return false;
        }
        last_hash = block->get_block_hash();
    }
    return true;
}

// check unit blocks without unit proof, proof highest block by next block
bool xsync_on_demand_t::check_unit_blocks(const std::vector<data::xblock_ptr_t> & blocks) {
    auto highest_sync_unit_block = blocks.back();
    auto & account = highest_sync_unit_block->get_account();
    uint64_t next_height = highest_sync_unit_block->get_height() + 1;
    auto next_block = m_sync_store->load_block_object(account, next_height);
    if (next_block == nullptr) {
        xsync_warn("xsync_on_demand_t::check_unit_blocks load unit fail,account:%s,h:%llu", account.c_str(), next_height);
        return false;
    }
    if (next_block->get_last_block_hash() != highest_sync_unit_block->get_block_hash()) {
        xsync_error("xsync_on_demand_t::check_unit_blocks sync unit(%s) can not connect with local unit(%s)", highest_sync_unit_block->dump().c_str(), next_block->dump().c_str());
        return false;
    }
    return true;
}

// check unit blocks with unit proof
bool xsync_on_demand_t::check_unit_blocks(const std::vector<data::xblock_ptr_t> & blocks, const std::string & unit_proof_str) {
    base::xunit_proof_t unit_proof;
    auto len = unit_proof.serialize_from(unit_proof_str);
    if (len <= 0) {
        xerror("xsync_on_demand_t::check_unit_blocks deserialize unit proof fail");
        return false;
    }

    auto highest_sync_unit_block = blocks.back();

    if (highest_sync_unit_block->get_height() != unit_proof.get_height() || highest_sync_unit_block->get_viewid() != unit_proof.get_viewid() ||
        !unit_proof.verify_unit_block(m_certauth.get(), highest_sync_unit_block)) {
        xsync_warn("xsync_on_demand_t::handle_blocks_response unit proof check fail,unit:%s,proof h:%llu,v:%llu",
                highest_sync_unit_block->dump().c_str(),
                unit_proof.get_height(),
                unit_proof.get_viewid());
        return false;
    }
    xsync_dbg("xsync_on_demand_t::check_unit_blocks sync unit proof succ,unit:%s,proof h:%llu,v:%llu",
            highest_sync_unit_block->dump().c_str(),
            unit_proof.get_height(),
            unit_proof.get_viewid());
    return true;
}

// check unit blocks for old version, might contain cert blocks.
bool xsync_on_demand_t::check_unit_blocks(const std::vector<data::xblock_ptr_t> &blocks, std::vector<data::xblock_ptr_t> & validated_blocks) {
    auto & account = blocks[0]->get_account();

    auto table_addr = account_address_to_block_address(common::xaccount_address_t(account));

    auto latest_committed_block = base::xvchain_t::instance().get_xblockstore()->get_latest_committed_block(table_addr, metrics::blockstore_access_from_txpool_refresh_table);
    base::xauto_ptr<base::xvbstate_t> bstate =
            base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(latest_committed_block.get(), metrics::statestore_access_from_txpool_refreshtable);
    if (bstate == nullptr) {
        xwarn("xsync_on_demand_t::store_unit_blocks_without_proof get table state fail.table latest commit block:%s", latest_committed_block->dump().c_str());
        return false;
    }

    xtablestate_ptr_t tablestate = std::make_shared<xtable_bstate_t>(bstate.get());

    base::xaccount_index_t account_index;
    bool result = tablestate->get_account_index(account, account_index);
    if (!result) {
        xwarn("xsync_on_demand_t::store_unit_blocks_without_proof get account index fail account:%s", account.c_str());
        return false;
    }

    uint64_t height = account_index.get_latest_unit_height();
    uint64_t view_id = account_index.get_latest_unit_viewid();

    bool validate = false;
    for (auto & block : blocks) {
        if (block->get_height() == height) {
            if (block->get_viewid() == view_id) {
                validated_blocks.push_back(block);
                validate = true;
                break;
            } else {
                xsync_warn("xsync_on_demand_t::store_unit_blocks_without_proof block(%s) not match with acccount state(viewid:%llu)",
                    block->dump().c_str(), view_id);
                return false;
            }
        } else if (block->get_height() > height) {
            xwarn("xsync_on_demand_t::store_unit_blocks_without_proof synced a unit which height is higher than state:%s,state height:%llu", block->dump().c_str(), height);
            return false;
        } else {
            validated_blocks.push_back(block);
        }
    }

    if (validated_blocks.empty()) {
        xsync_warn("xsync_on_demand_t::store_unit_blocks_without_proof no validate blocks, account:%s", account.c_str());
        return false;
    }

    if (!validate) {
        return check_unit_blocks(validated_blocks);
    }
    return true;
}

void xsync_on_demand_t::store_on_demand_sync_blocks(const std::vector<data::xblock_ptr_t> & blocks, const std::string & unit_proof_str) {
    for (auto & block : blocks) {
        // No.1 safe rule: clean all flags first when sync/replicated one block
        block->reset_block_flags();
        // XTODO,here need check hash to connect the prev authorized block,then set enum_xvblock_flag_authenticated
        block->set_block_flag(enum_xvblock_flag_authenticated);

        base::xvblock_t * vblock = dynamic_cast<base::xvblock_t *>(block.get());
        if (m_sync_store->store_block(vblock)) {
            xsync_info("xsync_on_demand_t::store_on_demand_sync_blocks succ %s,height=%lu,viewid=%lu,", block->get_account().c_str(), block->get_height(), block->get_viewid());
        } else {
            xsync_info("xsync_on_demand_t::store_on_demand_sync_blocks failed %s,height=%lu,viewid=%lu,", block->get_account().c_str(), block->get_height(), block->get_viewid());
        }
    }

    if (!unit_proof_str.empty()) {
        auto & account = blocks[0]->get_account();
        auto height = blocks[blocks.size() - 1]->get_height();
        if (!m_sync_store->set_unit_proof(account, unit_proof_str, height)) {
            xsync_error("xsync_on_demand_t::store_on_demand_sync_blocks account %s,fail to writed unit proof into db,height=%llu", account.c_str(), height);
        }
    }
}

NS_END2
