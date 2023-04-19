// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvnode/xcomponents/xblock_sniffing/xsniffer_action.h"

#include "xbasic/xmodule_type.h"
#include "xdata/xtx_factory.h"
#include "xcommon/xmessage_id.h"
#include "xvnetwork/xmessage.h"
#include "xstatestore/xstatestore_face.h"

NS_BEG4(top, vnode, components, sniffing)

void xtop_sniffer_action::call(observer_ptr<xtxpool_service_v2::xtxpool_proxy_face> const & txpool,
                               common::xaccount_address_t const & address,
                               std::string const & action_name,
                               std::string const & action_params,
                               const uint64_t timestamp) {                              
    base::xaccount_index_t accountindex;
    int32_t ret_pushtx = xsuccess;
    bool ret = statestore::xstatestore_hub_t::instance()->get_accountindex(LatestConnectBlock, address, accountindex);
    if (ret) {
        auto tx = data::xtx_factory::create_v2_run_contract_tx(address,
                                                        accountindex.get_latest_tx_nonce(),
                                                        action_name,
                                                        action_params,
                                                        timestamp);
        ret_pushtx = txpool->request_transaction_consensus(tx, true);                        
        xinfo("[xrole_context_t] call_contract in consensus mode with return code : %d, %s, %s %ld, %lld",
            ret_pushtx,
            tx->get_digest_hex_str().c_str(),
            address.to_string().c_str(),
            accountindex.get_latest_tx_nonce(),
            timestamp);
    }
}

void xtop_sniffer_action::call(observer_ptr<xtxpool_service_v2::xtxpool_proxy_face> const & txpool,
                               common::xaccount_address_t const & source_address,
                               common::xaccount_address_t const & target_address,
                               std::string const & action_name,
                               std::string const & action_params,
                               uint64_t timestamp) {
    base::xaccount_index_t accountindex;
    // int32_t ret_pushtx = xsuccess;
    bool ret = statestore::xstatestore_hub_t::instance()->get_accountindex(LatestConnectBlock, source_address, accountindex);
    if (ret) {
        auto tx = data::xtx_factory::create_v2_run_contract_tx(source_address,
                                                        target_address,
                                                        accountindex.get_latest_tx_nonce(),
                                                        action_name,
                                                        action_params,
                                                        timestamp);

        int32_t r = txpool->request_transaction_consensus(tx, true);
        xinfo("[xrole_context_t::fulltableblock_event] call_contract in consensus mode with return code : %d, %s, %s %ld, %lld",
                r,
                tx->get_digest_hex_str().c_str(),
                source_address.to_string().c_str(),
                accountindex.get_latest_tx_nonce(),
                timestamp);
    }
}

void xtop_sniffer_action::broadcast(observer_ptr<vnode::xvnode_face_t> const & vnode, data::xblock_ptr_t const & block_ptr, common::xnode_type_t types) {
    assert(block_ptr != nullptr);
    base::xstream_t stream(base::xcontext_t::instance());
    block_ptr->full_block_serialize_to(stream);
    auto message = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_block_broadcast_id);

    std::error_code ec;
    if (common::has<common::xnode_type_t::all_types>(types)) {
        common::xip2_t broadcast_xip{vnode->address().network_id()};
        vnode->broadcast(broadcast_xip, message, ec);
        if (ec) {
            xwarn("[xtop_sniffer_action::broadcast] broadcast to ALL failed. block owner %s height %lu", block_ptr->get_block_owner().c_str(), block_ptr->get_height());
            assert(false);
        } else {
            xdbg("[xtop_sniffer_action::broadcast] broadcast to ALL. block owner %s height %lu", block_ptr->get_block_owner().c_str(), block_ptr->get_height());
        }
    } else {
        if (common::has<common::xnode_type_t::committee>(types)) {
            common::xnode_address_t dest{common::build_committee_sharding_address(vnode->address().network_id())};
            vnode->broadcast(dest.xip2().group_xip2(), message, ec);
            if (ec) {
                xwarn("[xtop_sniffer_action::broadcast] broadcast to beacon failed. block owner %s", block_ptr->get_block_owner().c_str());
                assert(false);
            } else {
                xdbg("[xtop_sniffer_action::broadcast] broadcast to beacon. block owner %s", block_ptr->get_block_owner().c_str());
            }
        }

        if (common::has<common::xnode_type_t::zec>(types)) {
            common::xnode_address_t dest{common::build_zec_sharding_address(vnode->address().network_id())};
            vnode->broadcast(dest.xip2().group_xip2(), message, ec);
            if (ec) {
                xwarn("[xtop_sniffer_action::broadcast] broadcast to zec. block owner %s", block_ptr->get_block_owner().c_str());
                assert(false);
            } else {
                xdbg("[xtop_sniffer_action::broadcast] broadcast to zec. block owner %s", block_ptr->get_block_owner().c_str());
            }
        }

        if (common::has<common::xnode_type_t::storage>(types)) {
            common::xnode_address_t dest{};
            if (common::has<common::xnode_type_t::storage_archive>(types)) {
                dest = common::xnode_address_t{common::build_archive_sharding_address(common::xarchive_group_id, vnode->address().network_id())};
            } else if (common::has<common::xnode_type_t::storage_exchange>(types)) {
                dest = common::xnode_address_t{common::build_exchange_sharding_address(vnode->address().network_id())};
            } else {
                xassert(false);
            }
            vnode->broadcast(dest.xip2().group_xip2(), message, ec);
            if (ec) {
                xwarn("[xtop_sniffer_action::broadcast] broadcast to archive failed. block owner %s", block_ptr->get_block_owner().c_str());
                assert(false);
            } else {
                xdbg("[xtop_sniffer_action::broadcast] broadcast to archive. block owner %s", block_ptr->get_block_owner().c_str());
            }
        }
    }
}

NS_END4
