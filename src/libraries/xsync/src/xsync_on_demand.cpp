// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_on_demand.h"
#include "xsync/xsync_log.h"
#include "xmbus/xevent_behind.h"
#include "xsync/xsync_util.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xtable_bstate.h"
#include "xvledger/xvblockstore.h"
#include "xchain_fork/xutility.h"
#include "xvledger/xunit_proof.h"
#include "xstatestore/xstatestore_face.h"

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
    // bool unit_proof = bme->unit_proof;
    auto & last_unit_hash = bme->last_unit_hash;

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
    std::vector<vnetwork::xvnode_address_t> archive_list;
    
    if (bme->is_consensus) {
        archive_list = m_role_xips_mgr->get_rand_full_nodes(1);
    } else {
        archive_list = m_role_xips_mgr->get_rand_archives(1);
    }
    
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
    bool permit = m_download_tracer.apply(address, std::make_pair(start_height, start_height + count - 1), context, self_addr, target_addr);
    if (permit) {
            m_sync_sender->send_get_on_demand_blocks_with_params(address, start_height, count, is_consensus,
                                                last_unit_hash, self_addr, target_addr);
    } else {
        xsync_info("xsync_on_demand_t::on_behind_event is not permit because of overflow or during downloading, account: %s",
        address.c_str());
    }
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
        xdbg("tracer check error.");
        return -1;
    }

    if (tracer.get_src_addr().xip2().group_xip2() != network_self.xip2().group_xip2()) {
        xdbg("xsync_on_demand_t::check, src check:%s,%s", tracer.get_src_addr().to_string().c_str(), network_self.to_string().c_str());
        return -1;
    }
    if (tracer.get_dst_addr().xip2().group_xip2() != to_address.xip2().group_xip2()) {
        xdbg("xsync_on_demand_t::check, dst check:%s,%s", tracer.get_dst_addr().to_string().c_str(), to_address.to_string().c_str());
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
    bool permit = m_download_tracer.apply(address, std::make_pair(0, 20), context, self_addr, target_addr);
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

    bool is_table_address = data::is_table_address(common::xaccount_address_t{account});
    if (!is_table_address) {
        xsync_error("xsync_on_demand_t::handle_blocks_by_hash_response synced unit blocks by hash:%s", blocks[0]->dump().c_str());
        return;
    }
    store_on_demand_sync_blocks(blocks, "");
    m_download_tracer.expire(account);
}


bool xsync_on_demand_t::basic_check(const std::vector<data::xblock_ptr_t> &blocks, const vnetwork::xvnode_address_t &to_address, const vnetwork::xvnode_address_t &network_self) {
    if (blocks.empty()) {
        m_download_tracer.expire();
        return false;
    }

    auto & account = blocks[0]->get_account();
    xsync_dbg("xsync_on_demand_t::basic_check receive blocks of account %s, count %d",
        account.c_str(), blocks.size());
    int ret = check(account, to_address, network_self);
    if (ret != 0) {
        xsync_warn("xsync_on_demand_t::basic_check check the source of message failed %s,ret=%d", account.c_str(), ret);
        return false;
    }

    ret = check(account);
    if (ret != 0) {
        xsync_warn("xsync_on_demand_t::basic_check check failed %s,ret=%d", account.c_str(), ret);
        return false;
    }

     if (false == sync_blocks_continue_check(blocks, account, true)) {
        return  false;
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
    base::xaccount_index_t account_index;
    if (false == statestore::xstatestore_hub_t::instance()->get_accountindex_from_table_block(common::xaccount_address_t{account}, latest_committed_block.get(), account_index)) {
        xwarn("xsync_on_demand_t::check_unit_blocks get accountindex fail.table latest commit block:%s", latest_committed_block->dump().c_str());
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
}

void xsync_on_demand_t::handle_blocks_response_with_params(const std::vector<data::xblock_ptr_t> &blocks, const std::string& unit_proof_str,
    const vnetwork::xvnode_address_t &to_address, const vnetwork::xvnode_address_t &network_self) {

    xsync_info("xsync_on_demand_t::handle_blocks_response_with_params receive blocks(on_demand) %s, %s, count %d",
        network_self.to_string().c_str(), to_address.to_string().c_str(), blocks.size());

    if (!basic_check(blocks, to_address, network_self)) {
        return;
    }

    auto & account = blocks[0]->get_account();
    xblock_ptr_t last_block = blocks[blocks.size() -1];
    if (enum_result_code::success != check_auth(m_certauth, last_block)) {
        xsync_warn("xsync_on_demand_t::handle_blocks_response_with_params auth_failed %s,height=%lu,viewid=%lu,",
            account.c_str(), last_block->get_height(), last_block->get_viewid());
        return;
    }

    bool is_table_address = data::is_table_address(common::xaccount_address_t{account});

    if (is_table_address) {
        if (!unit_proof_str.empty()) {
            xsync_error("xsync_on_demand_t::handle_blocks_response_with_params table sync never need unit proof!addr:%s", account.c_str());
            return;
        }
        store_on_demand_sync_blocks(blocks, "");
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

    xsync_download_tracer tracer;
    if (!m_download_tracer.get(account, tracer)){
        return;
    }

    std::map<std::string, std::string> context = tracer.context();
    bool is_consensus = std::stoi(context["consensus"]);
    int32_t count = tracer.height_interval().second - tracer.trace_height();
    if (count > 0) {
        m_sync_sender->send_get_on_demand_blocks_with_params(account,  tracer.trace_height() + 1, count, is_consensus, "", network_self, to_address);
    } else {
        m_download_tracer.expire(account);
    }
}

void xsync_on_demand_t::handle_blocks_response_with_hash(const xsync_msg_block_request_ptr_t& request_ptr, const std::vector<data::xblock_ptr_t> &blocks, 
    const vnetwork::xvnode_address_t &to_address, const vnetwork::xvnode_address_t &network_self) {

    xsync_info("xsync_on_demand_t::handle_blocks_response_with_hash receive blocks(on_demand) %s, %s, count %d",
        network_self.to_string().c_str(), to_address.to_string().c_str(), blocks.size());

    if (!basic_check(blocks, to_address, network_self)) {
        return;
    }

    auto& account = blocks[0]->get_account();
    auto & block_hash = blocks[blocks.size() - 1]->get_block_hash();
    if (block_hash == request_ptr->get_requeset_param_str() && account == request_ptr->get_address()) {
       store_on_demand_sync_blocks(blocks, "");
    } else {
        xsync_warn("xsync_on_demand_t::handle_blocks_response_with_hash receive blocks(on_demand)  hash not compare %s, %s, count %d",
        network_self.to_string().c_str(), to_address.to_string().c_str(), blocks.size());
    }
}

NS_END2
