// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvnode/xvnode_manager.h"

#include "xcodec/xmsgpack_codec.hpp"
#include "xcrypto/xckey.h"
#include "xdata/xcodec/xmsgpack/xelection/xelection_result_store_codec.hpp"
#include "xdata/xdata_common.h"
#include "xstore/xaccount_context.h"
#include "xstore/xstore_error.h"
#include "xvnode/xvnode.h"
#include "xvnode/xvnode_factory.h"
#include "xvnode/xvnode_role_proxy.h"
#include "xvnode/xvnode_sniff_proxy.h"

#include <algorithm>

NS_BEG2(top, vnode)

xtop_vnode_manager::xtop_vnode_manager(observer_ptr<elect::ElectMain> const & elect_main,
                                       observer_ptr<mbus::xmessage_bus_face_t> const & mbus,
                                       observer_ptr<base::xvblockstore_t> const & block_store,
                                       observer_ptr<base::xvtxstore_t> const & txstore,
                                       observer_ptr<time::xchain_time_face_t> const & logic_timer,
                                       observer_ptr<router::xrouter_face_t> const & router,
                                       xobject_ptr_t<base::xvcertauth_t> const & certauth,
                                       observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                                       observer_ptr<sync::xsync_object_t> const & sync_object,
                                       observer_ptr<xtxpool_service_v2::xtxpool_service_mgr_face> const & txpool_service_mgr,
                                       observer_ptr<xtxpool_v2::xtxpool_face_t> const & txpool,
                                       observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor,
                                       observer_ptr<base::xvnodesrv_t> const & nodesvr,
                                       observer_ptr<state_sync::xstate_downloader_t> const & downloader)
  : xtop_vnode_manager{logic_timer,
                       vhost,
                       top::make_unique<xvnode_factory_t>(elect_main,
                                                          mbus,
                                                          block_store,
                                                          txstore,
                                                          logic_timer,
                                                          router,
                                                          vhost,
                                                          sync_object,
                                                          txpool_service_mgr,
                                                          election_cache_data_accessor,
                                                          nodesvr),
                       top::make_unique<xvnode_role_proxy_t>(mbus, block_store, txstore, logic_timer, router, certauth, txpool, election_cache_data_accessor, downloader),
                       nullptr // top::make_unique<xvnode_sniff_proxy_t>(mbus)

    } {
}

xtop_vnode_manager::xtop_vnode_manager(observer_ptr<time::xchain_time_face_t> const & logic_timer,
                                       observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                                       std::unique_ptr<xvnode_factory_face_t> vnode_factory,
                                       std::unique_ptr<xvnode_role_proxy_face_t> vnode_proxy,
                                       std::unique_ptr<xvnode_sniff_proxy_face_t> sniff_proxy)
  : m_logic_timer{logic_timer}, m_vhost{vhost}, m_vnode_factory{std::move(vnode_factory)}, m_vnode_proxy{std::move(vnode_proxy)}, m_sniff_proxy{std::move(sniff_proxy)} {
    assert(m_vnode_factory != nullptr);
}

void xtop_vnode_manager::start() {
    assert(m_logic_timer != nullptr);
    m_logic_timer->watch(chain_timer_watch_name, 1, std::bind(&xtop_vnode_manager::on_timer, this, std::placeholders::_1));
    // m_sniff_proxy->start();

    assert(!running());
    running(true);
}

void xtop_vnode_manager::stop() {
    assert(running());
    running(false);

    assert(m_logic_timer != nullptr);
    m_logic_timer->unwatch(chain_timer_watch_name);
    // m_sniff_proxy->stop();
}

std::pair<std::vector<common::xip2_t>, std::vector<common::xip2_t>> xtop_vnode_manager::handle_election_data(
    std::unordered_map<common::xcluster_address_t, election::cache::xgroup_update_result_t> const & election_data) {
    // if (!running()) {
    //     xwarn("[vnode mgr] is not running");
    //     return {};
    // }

    assert(!election_data.empty());

    auto const & account_address = m_vhost->account_address();
    xdbg("[vnode mgr] host %s sees election data size %zu", account_address.to_string().c_str(), election_data.size());

    std::vector<common::xip2_t> purely_outdated_xips;
    std::vector<common::xip2_t> logical_outdated_xips;
    XLOCK(m_nodes_mutex);
    for (auto const & group_info : election_data) {
        auto const & cluster_address = top::get<common::xcluster_address_t const>(group_info);
        auto const & group_update_result = top::get<election::cache::xgroup_update_result_t>(group_info);

        auto const & outdated_group = group_update_result.outdated;
        auto const & faded_group = group_update_result.faded;
        auto const & added_group = group_update_result.added;

        assert(added_group != nullptr);

        bool vnode_outdated{false};
        if (outdated_group != nullptr && outdated_group->contains(account_address)) {
            auto const & address = outdated_group->node_element(account_address)->address();
            assert(!broadcast(address.slot_id()));

            auto const it = m_all_nodes.find(address);
            if (it != std::end(m_all_nodes)) {
                vnode_outdated = true;

                auto & vnode = top::get<std::shared_ptr<xvnode_face_t>>(*it);
                vnode->rotation_status(common::xrotation_status_t::outdated, outdated_group->outdate_time());

                xwarn("[vnode mgr] vnode (%p) at address %s will outdate at logic time %" PRIu64, vnode.get(), vnode->address().to_string().c_str(), vnode->outdate_time());
            }

            xwarn("[vnode mgr] vnode at address %s is outdated", cluster_address.to_string().c_str());
            common::xip2_t xip{
                cluster_address.network_id(),
                cluster_address.zone_id(),
                cluster_address.cluster_id(),
                cluster_address.group_id(),
                outdated_group->group_size(),
                outdated_group->associated_blk_height()};
            purely_outdated_xips.push_back(std::move(xip));
        }

        if (faded_group != nullptr && faded_group->contains(account_address)) {
            auto const & address = faded_group->node_element(account_address)->address();
            assert(!broadcast(address.slot_id()));

            auto const & it = m_all_nodes.find(address);
            if (it != std::end(m_all_nodes)) {
                vnode_outdated = false;

                auto & vnode = top::get<std::shared_ptr<xvnode_face_t>>(*it);
                vnode->rotation_status(common::xrotation_status_t::faded, faded_group->fade_time());

                xwarn("[vnode mgr] vnode (%p) at address %s will fade at logic time %" PRIu64, vnode.get(), vnode->address().to_string().c_str(), vnode->fade_time());
            }
        }

        if (added_group->contains(account_address)) {
            vnode_outdated = false;

            auto const & address = added_group->node_element(account_address)->address();
            assert(!broadcast(address.slot_id()));

            auto const it = m_all_nodes.find(address);
            if (it != std::end(m_all_nodes)) {
                continue;
            }

            assert(m_vnode_factory);
            auto vnode = m_vnode_factory->create_vnode_at(added_group);
            m_vnode_proxy->create(std::dynamic_pointer_cast<top::vnode::xvnode_t>(vnode)->vnetwork_driver());
            assert(vnode->address() == address);

            vnode->rotation_status(common::xrotation_status_t::started, added_group->start_time());
            xwarn("[vnode mgr] vnode (%p) at address %s will start at logic time %" PRIu64 " associated election blk height %" PRIu64 " miner_type %s genesis %s raw credit score %" PRIu64,
                  vnode.get(),
                  vnode->address().to_string().c_str(),
                  vnode->start_time(),
                  vnode->address().associated_blk_height(),
                  common::to_string(vnode->miner_type()).c_str(),
                  vnode->genesis() ? "true" : "false",
                  vnode->raw_credit_score());

            vnode->synchronize();
            xwarn("[vnode mgr] vnode (%p) at address %s starts synchronizing", vnode.get(), vnode->address().to_string().c_str());

            XATTRIBUTE_MAYBE_UNUSED auto ret = m_all_nodes.insert({address, vnode});
            assert(top::get<bool>(ret));
            (void)ret;
        }

        if (vnode_outdated) {
            xwarn("[vnode mgr] vnode at address %s is outdated", cluster_address.to_string().c_str());

            m_vnode_proxy->destroy(common::xnode_address_t{cluster_address});
            // m_vnode_proxy->destroy({xip.raw_low_part(), xip.raw_high_part()});

            common::xip2_t xip{cluster_address.network_id(), cluster_address.zone_id(), cluster_address.cluster_id(), cluster_address.group_id()};

            logical_outdated_xips.push_back(std::move(xip));
        }
    }

    return std::make_pair(purely_outdated_xips, logical_outdated_xips);
}

void xtop_vnode_manager::on_timer(common::xlogic_time_t time) {
    if (!running()) {
        xwarn("[vnode mgr] is not running");
        return;
    }

    assert(m_election_notify_thread_id != std::this_thread::get_id());

    XLOCK_GUARD(m_nodes_mutex) {
        // make sure:
        //  1. outdate first
        //  2. fade second
        //  3. start third

        stop_vnodes_with_lock_hold_outside(time);
        fade_vnodes_with_lock_hold_outside(time);
        start_vnodes_with_lock_hold_outside(time);

#if defined(ENABLE_METRICS)
        if (time % 6 == 0) {    // dump per one minute
            std::unordered_map<common::xnode_type_t, int32_t> metrics_vnode_status;
            metrics_vnode_status[common::xnode_type_t::storage_archive] = 0;
            metrics_vnode_status[common::xnode_type_t::storage_exchange] = 0;
            metrics_vnode_status[common::xnode_type_t::edge] = 0;
            metrics_vnode_status[common::xnode_type_t::rec] = 0;
            metrics_vnode_status[common::xnode_type_t::zec] = 0;
            metrics_vnode_status[common::xnode_type_t::consensus_auditor] = 0;
            metrics_vnode_status[common::xnode_type_t::consensus_validator] = 0;
            metrics_vnode_status[common::xnode_type_t::fullnode] = 0;
            metrics_vnode_status[common::xnode_type_t::evm_auditor] = 0;
            metrics_vnode_status[common::xnode_type_t::evm_validator] = 0;
            metrics_vnode_status[common::xnode_type_t::relay] = 0;

            for (auto it = std::begin(m_all_nodes); it != std::end(m_all_nodes); ++it) {
                auto const & vnode = top::get<std::shared_ptr<xvnode_face_t>>(*it);
                assert(vnode != nullptr);

                if (vnode->rotation_status(time) != common::xrotation_status_t::started) {
                    continue;
                }

                auto const vnode_real_part_type = common::real_part_type(vnode->type());
                switch (vnode_real_part_type) {
                case common::xnode_type_t::rec:
                    metrics_vnode_status[vnode_real_part_type] = -1;
                    break;

                case common::xnode_type_t::zec:
                    metrics_vnode_status[vnode_real_part_type] = -2;
                    break;

                case common::xnode_type_t::consensus_auditor:
                    // 1 , 2 
                    metrics_vnode_status[vnode_real_part_type] = static_cast<int32_t>(vnode->address().group_id().value());
                    break;

                case common::xnode_type_t::consensus_validator:
                    // 3 , 4 , 5 , 6
                    metrics_vnode_status[vnode_real_part_type] = static_cast<int32_t>(vnode->address().group_id().value() - common::xvalidator_group_id_value_begin + 3);
                    break;

                case common::xnode_type_t::storage_archive:
                    XATTRIBUTE_FALLTHROUGH;
                case common::xnode_type_t::storage_exchange:
                    metrics_vnode_status[vnode_real_part_type] = -3;
                    break;

                case common::xnode_type_t::edge:
                    metrics_vnode_status[vnode_real_part_type] = -4;
                    break;

                case common::xnode_type_t::fullnode:
                    metrics_vnode_status[vnode_real_part_type] = -5;
                    break;

                case common::xnode_type_t::frozen:
                    // do nothing.
                    break;

                case common::xnode_type_t::evm_auditor:
                    metrics_vnode_status[vnode_real_part_type] = -6;
                    break;

                case common::xnode_type_t::evm_validator:
                    metrics_vnode_status[vnode_real_part_type] = -7;
                    break;

                case common::xnode_type_t::relay:
                    metrics_vnode_status[vnode_real_part_type] = -8;
                    break;

                default:
                    assert(false);
                    break;
                }
            }

            XMETRICS_PACKET_INFO("vnode_status",
                                 "rec", metrics_vnode_status[common::xnode_type_t::rec],
                                 "zec", metrics_vnode_status[common::xnode_type_t::zec],
                                 "auditor", metrics_vnode_status[common::xnode_type_t::consensus_auditor],
                                 "validator", metrics_vnode_status[common::xnode_type_t::consensus_validator],
                                 "archive", metrics_vnode_status[common::xnode_type_t::storage_archive],
                                 "edge", metrics_vnode_status[common::xnode_type_t::edge],
                                 "fullnode", metrics_vnode_status[common::xnode_type_t::fullnode],
                                 "evm_auditor", metrics_vnode_status[common::xnode_type_t::evm_auditor],
                                 "evm_validator", metrics_vnode_status[common::xnode_type_t::evm_validator],
                                 "relay", metrics_vnode_status[common::xnode_type_t::relay]);
        }
#endif
    }
}

void xtop_vnode_manager::start_vnodes_with_lock_hold_outside(common::xlogic_time_t const time) {
    start_vnode_with_lock_hold_outside<common::xnode_type_t::frozen>(time);
    start_vnode_with_lock_hold_outside<common::xnode_type_t::rec>(time);
    start_vnode_with_lock_hold_outside<common::xnode_type_t::storage_archive>(time);
    start_vnode_with_lock_hold_outside<common::xnode_type_t::storage_exchange>(time);
    start_vnode_with_lock_hold_outside<common::xnode_type_t::fullnode>(time);
    start_vnode_with_lock_hold_outside<common::xnode_type_t::edge>(time);
    start_vnode_with_lock_hold_outside<common::xnode_type_t::zec>(time);
    start_vnode_with_lock_hold_outside<common::xnode_type_t::consensus_auditor>(time);
    start_vnode_with_lock_hold_outside<common::xnode_type_t::consensus_validator>(time);
    start_vnode_with_lock_hold_outside<common::xnode_type_t::evm_auditor>(time);
    start_vnode_with_lock_hold_outside<common::xnode_type_t::evm_validator>(time);
    start_vnode_with_lock_hold_outside<common::xnode_type_t::relay>(time);
}

void xtop_vnode_manager::fade_vnodes_with_lock_hold_outside(common::xlogic_time_t const time) {
    for (auto it = std::begin(m_all_nodes); it != std::end(m_all_nodes); ++it) {
        auto & vnode = top::get<std::shared_ptr<xvnode_face_t>>(*it);
        assert(vnode != nullptr);

        switch (vnode->rotation_status(time)) {
        case common::xrotation_status_t::outdated: {
            assert(false);
            break;
        }

        case common::xrotation_status_t::faded: {
            if (!vnode->faded() && vnode->running()) {
                vnode->fade();
                m_vnode_proxy->fade(vnode->address());
                // m_sniff_proxy->unreg(vnode->address());

                xwarn("[vnode mgr] vnode (%p) at address %s fades at logic time %" PRIu64 " current logic time %" PRIu64,
                      vnode.get(),
                      vnode->address().to_string().c_str(),
                      vnode->fade_time(),
                      time);
            }

            break;
        }

        default: {
            break;
        }
        }
    }
}

void xtop_vnode_manager::stop_vnodes_with_lock_hold_outside(common::xlogic_time_t const time) {
    for (auto it = std::begin(m_all_nodes); it != std::end(m_all_nodes);) {
        auto & vnode = top::get<std::shared_ptr<xvnode_face_t>>(*it);
        assert(vnode != nullptr);

        switch (vnode->rotation_status(time)) {
        case common::xrotation_status_t::outdated: {
            if (vnode->faded() && vnode->running()) {
                vnode->stop();
                m_vnode_proxy->unreg(vnode->address());
                // m_sniff_proxy->unreg(vnode->address());

                xwarn("[vnode mgr] vnode (%p) at address %s outdates at logic time %" PRIu64 " current logic time %" PRIu64,
                      vnode.get(),
                      vnode->address().to_string().c_str(),
                      vnode->outdate_time(),
                      time);
            }

            it = m_all_nodes.erase(it);
            break;
        }

        default: {
            ++it;
            break;
        }
        }
    }
}

NS_END2
