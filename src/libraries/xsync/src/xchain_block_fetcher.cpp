// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xchain_block_fetcher.h"

#include "xsync/xblock_fetcher.h"
#include "xmbus/xevent_blockfetcher.h"
#include "xmbus/xevent_executor.h"
#include "xsync/xsync_log.h"
#include "xsync/xsync_util.h"

NS_BEG2(top, sync)

using namespace metrics;
using namespace mbus;
using namespace data;

const uint32_t ARRIVE_TIMEOUT = 400;
// 5s
const uint32_t FETCHER_TIMEOUT = 5000;

xchain_block_fetcher_t::xchain_block_fetcher_t(std::string vnode_id,
        const std::string &address,
        const observer_ptr<base::xvcertauth_t> &certauth,
        xsync_store_face_t *sync_store,
        xsync_sender_t *sync_sender):
m_vnode_id(vnode_id),
m_address(address),
m_certauth(certauth),
m_sync_store(sync_store),
m_sync_sender(sync_sender) {
}

void xchain_block_fetcher_t::on_timer() {
    // clear timeout
    int64_t now = get_time();
    for (auto it = m_fetching.begin(); it!=m_fetching.end();) {
        if ((now - it->second->tm) > FETCHER_TIMEOUT) {
            xsync_info("chain_fetcher remove timeout %s", to_hex_str(it->first.c_str()).c_str());
            m_fetching.erase(it++);
        } else {
            ++it;
        }
    }

    // send request
    std::set<std::string> forget_hashes;
    std::list<xsync_block_announce_ptr_t> requests;

    for (auto &it: m_announced) {
        // const std::string &hash = it.first;
        std::vector<xsync_block_announce_ptr_t> &lists = it.second;
        if (lists.size() == 0)
            continue;

        uint32_t idx = RandomUint32()%(uint32_t)lists.size();
        xsync_block_announce_ptr_t ptr = lists[idx];

        // must wait for fixed time
        if ((now-ptr->tm) > ARRIVE_TIMEOUT) {

            forget_hashes.insert(ptr->hash);

            base::xauto_ptr<base::xvblock_t> blk = m_sync_store->query_block(m_address, ptr->height, ptr->hash);
            if (blk == nullptr) {
                requests.push_back(ptr);
            }
        }
    }

    for (auto &it: forget_hashes) {
        forget_hash(it);
    }

    for (auto &request: requests) {
        m_fetching[request->hash] = request;
        request_sync_blocks(request);
    }
}

void xchain_block_fetcher_t::on_newblock(data::xblock_ptr_t & block, const vnetwork::xvnode_address_t & network_self, const vnetwork::xvnode_address_t & from_address) {

    auto exist_block = m_sync_store->existed(block->get_account(), block->get_height(), block->get_viewid());
    if (exist_block) {
        XMETRICS_GAUGE(metrics::xsync_recv_duplicate_block, 1);
        xsync_warn("xsync_handler_t::on_newblock exist_block %s", block->dump().c_str());
        return;
    }

    if (enum_result_code::success != check_auth(m_certauth, block)) {
        xsync_warn("xsync_handler_t::on_newblock fail-auth failed %s", block->dump().c_str());
        XMETRICS_GAUGE(metrics::xsync_recv_invalid_block, 1);
        return;
    }

    import_block(block);

    if (common::has<common::xnode_type_t::storage_archive>(network_self.type())) {
        uint64_t latest_end_block_height = m_sync_store->get_latest_end_block_height(m_address, enum_chain_sync_policy_full);
        xsync_info("chain_fetcher on_newblock %s,%llu",
                   block->dump().c_str(),
                   latest_end_block_height);
        xchain_state_info_t info;
        info.address = m_address;
        info.end_height = latest_end_block_height;
        std::vector<xchain_state_info_t> info_list;
        info_list.push_back(std::move(info));
        m_sync_sender->send_archive_height_list(info_list, network_self, from_address);
    }
}

// push_newblockhash and broadcast_newblockhash
void xchain_block_fetcher_t::on_newblockhash(uint64_t height, const std::string &hash,
        const vnetwork::xvnode_address_t &network_self, const vnetwork::xvnode_address_t &from_address) {

    xsync_info("chain_fetcher on_newblockhash %s,height=%lu,hash=%s,", m_address.c_str(), height, to_hex_str(hash).c_str());

    if (m_announces.find(from_address) == m_announces.end()) {
        m_announces[from_address] = 1;
    } else {
        m_announces[from_address] += 1;
    }

    if (m_announces[from_address] % 100 == 0) {
        xdbg("chain_fetcher on_newblockhash self: %s, recv count: %d, from node: %s", network_self.to_string().c_str(), m_announces[from_address], from_address.to_string().c_str());
    }

    if (height==0 || hash=="")
            return;

    if ((m_fetching.find(hash) != m_fetching.end()) || (m_completing.find(hash) != m_completing.end())) {
            return;
        }

    xsync_block_announce_ptr_t announce = std::make_shared<xsync_block_announce_t>(height, hash, network_self, from_address, get_time());

    // if is push_newblockhash, process it immediately
    common::xnode_type_t self_node_type = network_self.type();
    common::xnode_type_t from_node_type = from_address.type();
    if (common::has<common::xnode_type_t::storage>(self_node_type) &&
        (
            common::has<common::xnode_type_t::rec>(from_node_type) ||
            common::has<common::xnode_type_t::zec>(from_node_type) ||
            common::has<common::xnode_type_t::consensus>(from_node_type)
        )
    ) {
        base::xauto_ptr<base::xvblock_t> blk = m_sync_store->query_block(m_address, height, hash);
        if (blk != nullptr) {
            forget_hash(hash);
            xsync_dbg("chain_fetcher on_newblockhash(exist) %s,height=%lu,hash=%s,", m_address.c_str(), height, to_hex_str(hash).c_str());
            return;
        }

        forget_hash(hash);
        m_fetching[hash] = announce;
        request_sync_blocks(announce);
        return;
    }

    if (m_announced.find(hash) == m_announced.end()) {
        m_announced[hash] = std::vector<xsync_block_announce_ptr_t>{announce};
    } else {
        m_announced[hash].push_back(announce);
    }

}

void xchain_block_fetcher_t::on_response_blocks(xblock_ptr_t &block, const vnetwork::xvnode_address_t &network_self, const vnetwork::xvnode_address_t &from_address) {
    XMETRICS_GAUGE(metrics::xsync_blockfetcher_response, 1);

    const std::string &hash = block->get_block_hash();
    {
        auto it = m_fetching.find(hash);
        if (it == m_fetching.end()) {
            //assert(0);
            return;
        }

        xsync_block_announce_ptr_t ptr = it->second;
        forget_hash(hash);

        if (enum_result_code::success != check_auth(m_certauth, block)) {
            xsync_warn("chain_fetcher on_response_event(auth failed) : %s %s", block->dump().c_str(), from_address.to_string().c_str());
            return;
        }

        // TODO auth failed??
        m_completing[hash] = ptr;
        insert_block(block);
        add_blocks();
    }

    xsync_info("chain_fetcher on_response_event success : %s %s",
        block->dump().c_str(), from_address.to_string().c_str());
}

void xchain_block_fetcher_t::add_blocks() {
    for (auto &it: m_blocks) {
        import_block(it.second);
        forget_hash(it.second->get_block_hash());
    }

    m_blocks.clear();
}

void xchain_block_fetcher_t::import_block(xblock_ptr_t &block) {

    xsync_info("chain_fetcher handle_block %s,height=%lu,viewid=%lu,hash=%s,",
            m_address.c_str(), block->get_height(), block->get_viewid(), to_hex_str(block->get_block_hash()).c_str());


    base::xvblock_t* vblock = dynamic_cast<base::xvblock_t*>(block.get());
    m_sync_store->store_block(vblock);

}

// need vector?
void xchain_block_fetcher_t::request_sync_blocks(const xsync_block_announce_ptr_t &announce) {

    uint64_t height = announce->height;
    const std::string &hash = announce->hash;
    const vnetwork::xvnode_address_t &network_self = announce->network_self;
    const vnetwork::xvnode_address_t &target_address = announce->from_address;

    XMETRICS_GAUGE(metrics::xsync_blockfetcher_request, 1);

    xsync_info("chain_fetcher send sync request(block_by_hash). %s,heigth=%lu,hash=%s, %s",
                m_address.c_str(), height, to_hex_str(hash).c_str(), target_address.to_string().c_str());

    std::vector<xblock_hash_t> hashes;
    xblock_hash_t info;
    info.address = m_address;
    info.height = height;
    info.hash = hash;
    hashes.push_back(info);

    m_sync_sender->send_get_blocks_by_hashes(hashes, network_self, target_address);
}

void xchain_block_fetcher_t::insert_block(const xblock_ptr_t &block) {
    m_blocks.insert(std::make_pair(block->get_block_hash(), block));
}

void xchain_block_fetcher_t::forget_hash(const std::string &hash) {

    m_announced.erase(hash);
    m_fetching.erase(hash);
    m_completing.erase(hash);
}

void xchain_block_fetcher_t::forget_block(const std::string &hash) {
    m_blocks.erase(hash);
}

int64_t xchain_block_fetcher_t::get_time() {
    return base::xtime_utl::gmttime_ms();
}

NS_END2
