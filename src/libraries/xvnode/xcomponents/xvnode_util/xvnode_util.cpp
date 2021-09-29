// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvnode/xcomponents/xvnode_util/xvnode_util.h"

#include "xdata/xtransaction_v2.h"
#include "xvm/manager/xmessage_ids.h"
#include "xvnetwork/xmessage.h"

NS_BEG2(top, vnode)

void xtop_vnode_util::call(observer_ptr<store::xstore_face_t> store,
                           observer_ptr<xtxpool_service_v2::xtxpool_proxy_face> const & txpool,
                           common::xaccount_address_t const & address,
                           std::string const & action_name,
                           std::string const & action_params,
                           const uint64_t timestamp) {
    auto tx = make_object_ptr<data::xtransaction_v2_t>();
    tx->make_tx_run_contract(action_name, action_params);
    tx->set_same_source_target_address(address.value());
    xaccount_ptr_t account = store->query_account(address.value());
    assert(account != nullptr);
    tx->set_last_trans_hash_and_nonce(account->account_send_trans_hash(), account->account_send_trans_number());
    tx->set_fire_timestamp(timestamp);
    tx->set_expire_duration(300);
    tx->set_digest();
    tx->set_len();

    int32_t r = txpool->request_transaction_consensus(tx, true);
    xinfo("[xrole_context_t] call_contract in consensus mode with return code : %d, %s, %s %s %ld, %lld",
          r,
          tx->get_digest_hex_str().c_str(),
          address.value().c_str(),
          data::to_hex_str(account->account_send_trans_hash()).c_str(),
          account->account_send_trans_number(),
          timestamp);
}

void xtop_vnode_util::call(observer_ptr<store::xstore_face_t> store,
                           observer_ptr<xtxpool_service_v2::xtxpool_proxy_face> const & txpool,
                           common::xaccount_address_t const & source_address,
                           common::xaccount_address_t const & target_address,
                           std::string const & action_name,
                           std::string const & action_params,
                           uint64_t timestamp) {
    auto tx = make_object_ptr<xtransaction_v2_t>();
    tx->make_tx_run_contract(action_name, action_params);
    tx->set_different_source_target_address(source_address.value(), target_address.value());
    xaccount_ptr_t account = store->query_account(source_address.value());
    assert(account != nullptr);
    tx->set_last_trans_hash_and_nonce(account->account_send_trans_hash(), account->account_send_trans_number());
    tx->set_fire_timestamp(timestamp);
    tx->set_expire_duration(300);
    tx->set_digest();
    tx->set_len();

    int32_t r = txpool->request_transaction_consensus(tx, true);
    xinfo("[xrole_context_t::fulltableblock_event] call_contract in consensus mode with return code : %d, %s, %s %s %ld, %lld",
            r,
            tx->get_digest_hex_str().c_str(),
            source_address.c_str(),
            data::to_hex_str(account->account_send_trans_hash()).c_str(),
            account->account_send_trans_number(),
            timestamp);
}

void xtop_vnode_util::broadcast(observer_ptr<vnetwork::xvnetwork_driver_face_t> const & driver, xblock_ptr_t const & block_ptr, common::xnode_type_t types) {
    assert(block_ptr != nullptr);
    base::xstream_t stream(base::xcontext_t::instance());
    block_ptr->full_block_serialize_to(stream);
    auto message = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, contract::xmessage_block_broadcast_id);

    if (common::has<common::xnode_type_t::all>(types)) {
        common::xnode_address_t dest{common::xcluster_address_t{driver->network_id()}};
        driver->broadcast_to(dest, message);
        xdbg("[xrole_context_t] broadcast to ALL. block owner %s height %lu", block_ptr->get_block_owner().c_str(), block_ptr->get_height());
    } else {
        if (common::has<common::xnode_type_t::committee>(types)) {
            common::xnode_address_t dest{common::build_committee_sharding_address(driver->network_id())};
            driver->forward_broadcast_message(message, dest);
            xdbg("[xrole_context_t] broadcast to beacon. block owner %s", block_ptr->get_block_owner().c_str());
        }

        if (common::has<common::xnode_type_t::zec>(types)) {
            common::xnode_address_t dest{common::build_zec_sharding_address(driver->network_id())};
            if (driver->address().cluster_address() == dest.cluster_address()) {
                driver->broadcast(message);
            } else {
                driver->forward_broadcast_message(message, dest);
            }
            xdbg("[xrole_context_t] broadcast to zec. block owner %s", block_ptr->get_block_owner().c_str());
        }

        if (common::has<common::xnode_type_t::storage>(types)) {
            for (auto archive_gid = common::xarchive_group_id_begin; archive_gid < common::xarchive_group_id_end; ++archive_gid) {
                common::xnode_address_t dest{
                    common::build_archive_sharding_address(archive_gid, driver->network_id()),
                };
                driver->forward_broadcast_message(message, dest);
            }
            xdbg("[xrole_context_t] broadcast to archive. block owner %s", block_ptr->get_block_owner().c_str());
        }
    }
}

NS_END2
