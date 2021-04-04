// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_v1_block_fetcher.h"
#include "xsync/xsync_log.h"
#include "xsync/xrole_chains_mgr.h"
#include "xsync/xsync_util.h"

NS_BEG2(top, sync)

xsync_v1_block_fetcher_t::xsync_v1_block_fetcher_t(std::string vnode_id,
        const observer_ptr<base::xvcertauth_t> &certauth,
        xsync_store_face_t *sync_store,
        xsync_sender_t *sync_sender):
m_vnode_id(vnode_id),
m_certauth(certauth),
m_sync_store(sync_store),
m_sync_sender(sync_sender) {

}

void xsync_v1_block_fetcher_t::handle_v1_newblockhash(const std::string &address, uint64_t height, uint64_t view_id, const vnetwork::xvnode_address_t &from_address, const vnetwork::xvnode_address_t &network_self) {

    std::string key = get_key(address, height);

    std::unique_lock<std::mutex> lock(m_lock);
    
    if (m_announce_list.size() > 10000) {
        xsync_warn("xsync_v1_block_fetcher_t out of limit : %s,height=%lu,viewid=%lu,", address.c_str(), height, view_id);
        return;
    }

    if (m_announce_list.find(key) != m_announce_list.end()) {
        return;
    }

    if (m_fetching_list.find(key) != m_fetching_list.end()) {
        return;
    }

    std::shared_ptr<xfetch_context_t> ctx = std::make_shared<xfetch_context_t>(address, height, network_self, from_address, get_time());
    m_announce_list[key] = ctx;
}

bool xsync_v1_block_fetcher_t::filter_block(xblock_ptr_t &block) {

    std::string key = get_key(block->get_account(), block->get_height());

    {
        std::unique_lock<std::mutex> lock(m_lock);
        if (m_fetching_list.find(key) == m_fetching_list.end()) {
            return false;
        }

        m_fetching_list.erase(key);
    }

    if (!check_auth(m_certauth, block)) {
        xsync_info("xsync_v1_block_fetcher_t on_response_event(auth failed) : %s", block->dump().c_str());
        return true;
    }

    m_sync_store->store_block(block.get());

    return true;
}

void xsync_v1_block_fetcher_t::on_timer_check_v1_newblockhash() {

    int64_t now = get_time();

    std::unique_lock<std::mutex> lock(m_lock);

    {
        for (auto it = m_announce_list.begin(); it != m_announce_list.end();) {

            std::shared_ptr<xfetch_context_t> ctx = it->second;
            if ((now-ctx->tm) > 100) {
                std::string key = it->first;
                m_announce_list.erase(it++);

                base::xauto_ptr<base::xvblock_t> blk = m_sync_store->get_latest_cert_block(ctx->address);
                uint64_t latest_height = blk->get_height();

                if (latest_height == ctx->height) {

                } else if (latest_height > ctx->height) {
                    base::xauto_ptr<base::xvblock_t> blk2 = m_sync_store->load_block_object(ctx->address, ctx->height);
                    if (blk2 == nullptr) {
                        m_fetching_list[key] = ctx;
                        m_sync_sender->send_get_blocks(ctx->address, ctx->height, 1, ctx->network_self, ctx->from_address);
                    }
                } else {
                    m_fetching_list[key] = ctx;
                    m_sync_sender->send_get_blocks(ctx->address, ctx->height, 1, ctx->network_self, ctx->from_address);
                }
            } else {
                it++;
            }
        }
    }

    // clean timeout
    {
        for (auto it = m_fetching_list.begin(); it != m_fetching_list.end(); ) {
            std::shared_ptr<xfetch_context_t> ctx = it->second;
            if ((now-ctx->tm) > 5000) {
                xsync_info("xsync_v1_block_fetcher_t timeout : %s,height=%lu, %s", ctx->address.c_str(), ctx->height, ctx->from_address.to_string().c_str());
                m_fetching_list.erase(it++);
            } else {
                it++;
            }
        }
    }
}

std::string xsync_v1_block_fetcher_t::get_key(const std::string &address, uint64_t height) {
    std::string key = address + "_" + std::to_string(height);
    return key;
}

int64_t xsync_v1_block_fetcher_t::get_time() {
    return base::xtime_utl::gmttime_ms();
}

NS_END2