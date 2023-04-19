// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvnetwork/xvhost.h"

#include "xbase/xlog.h"
#include "xbasic/xerror/xerror.h"
#include "xbasic/xmemory.hpp"
#include "xbasic/xscope_executer.h"
#include "xbasic/xthreading/xbackend_thread.hpp"
#include "xbasic/xthreading/xutility.h"
#include "xbasic/xutility.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xcommon/xaddress.h"
#include "xconfig/xconfig_register.h"
#include "xelection/xcache/xgroup_element.h"
#include "xelection/xdata_accessor_error.h"
#include "xmetrics/xmetrics.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xcommon/xmessage_id.h"
#include "xutility/xhash.h"
#include "xvnetwork/xcodec/xmsgpack/xmessage_codec.hpp"
#include "xvnetwork/xcodec/xmsgpack/xvnetwork_message_codec.hpp"
#include "xvnetwork/xmessage.h"
#include "xvnetwork/xmessage_filter_manager.h"
#include "xvnetwork/xvnetwork_driver.h"
#include "xvnetwork/xvnetwork_error.h"
#include "xvnetwork/xvnetwork_error2.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cinttypes>
#include <functional>
#include <random>
#include <sstream>
#include <unordered_set>

NS_BEG2(top, vnetwork)

//static common::xip2_t convert_to_p2p_xip2(common::xnode_address_t const & address) {
//    auto const & sharding_xip = address.sharding_address().xip();
//    if (address.election_round().empty()) {
//        return {sharding_xip.network_id(), sharding_xip.zone_id(), sharding_xip.cluster_id(), sharding_xip.group_id()};
//    }
//    return {sharding_xip.network_id(),
//            sharding_xip.zone_id(),
//            sharding_xip.cluster_id(),
//            sharding_xip.group_id(),
//            address.slot_id(),
//            address.group_size(),
//            // address.election_round().value() // version 0 && 1
//            address.associated_blk_height() // version 2.
//            }; // p2p layer use election_round as block height.
//}

static void msg_metrics(xvnetwork_message_t const & message, metrics::E_SIMPLE_METRICS_TAG tag_start) {
#if defined(ENABLE_METRICS)
    auto const message_category = common::get_message_category(message.message_id());
    auto delta = (uint16_t)message_category - (uint16_t)xmessage_category_consensus;
    XMETRICS_GAUGE((metrics::E_SIMPLE_METRICS_TAG)(tag_start + delta), 1);
#endif
}

xtop_vhost::xtop_vhost(observer_ptr<elect::xnetwork_driver_face_t> const & network_driver,
                       observer_ptr<time::xchain_time_face_t> const & chain_timer,
                       common::xnetwork_id_t const & nid,
                       observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor,
                       std::unique_ptr<xmessage_filter_manager_face_t> message_filter_manager_ptr)
  : base_t{nid, chain_timer, election_cache_data_accessor}, m_network_driver{network_driver} {
    assert(m_network_driver != nullptr);

    if (message_filter_manager_ptr == nullptr) {
        m_filter_manager = top::make_unique<xmessage_filter_manager_t>(make_observer<xvhost_face_t>(this), election_cache_data_accessor);
    } else {
        // mock the m_filter_manager if you do not use the default para.
        m_filter_manager = std::move(message_filter_manager_ptr);
    }
}

common::xnode_id_t const & xtop_vhost::account_address() const noexcept {
    assert(m_network_driver);
    return m_network_driver->account_address();
}

void xtop_vhost::start() {
    assert(m_network_driver);
    m_network_driver->register_message_ready_notify(std::bind(&xtop_vhost::on_network_data_ready, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    assert(!running());
    m_filter_manager->start();
    running(true);
    assert(running());

    auto self = shared_from_this();
    threading::xbackend_thread::spawn([this, self] {
#if defined DEBUG
        m_vhost_thread_id = std::this_thread::get_id();
#endif
        do_handle_network_data();
    });
}

void xtop_vhost::stop() {
    m_message_queue.push(xbyte_buffer_t{});

    assert(running());
    running(false);
    assert(!running());

    assert(m_network_driver);

    m_network_driver->unregister_message_ready_notify();
}

void xtop_vhost::send_to_through_frozen(common::xnode_address_t const & src, common::xnode_address_t const & dst, xmessage_t const & message, std::error_code & ec) {
    assert(src.zone_id() == common::xfrozen_zone_id);
    assert(src.cluster_id() == common::xdefault_cluster_id);
    assert(src.group_id() == common::xdefault_group_id);

    if (!running()) {
        ec = xvnetwork_errc2_t::vhost_not_run;
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return;
    }
    if (common::broadcast(dst.network_id()) || common::broadcast(dst.zone_id()) || common::broadcast(dst.cluster_id()) || common::broadcast(dst.group_id()) ||
        common::broadcast(dst.slot_id())) {
        ec = xvnetwork_errc2_t::invalid_dst_address;
        xwarn("%s %s. dst address is a broadcast address %s", vnetwork_category2().name(), ec.message().c_str(), dst.to_string().c_str());
        return;
    }

    xvnetwork_message_t const vmsg{src, dst, message, m_chain_timer->logic_time()};
    assert(common::get_message_category(vmsg.message_id()) == xmessage_category_sync);
    auto bytes = codec::msgpack_encode(vmsg);

    m_network_driver->send_to_through_root(src.xip2(), dst.node_id(), bytes, ec);

    msg_metrics(vmsg, metrics::message_send_category_begin);
}

void xtop_vhost::send_to(common::xnode_address_t const & src, common::xnode_address_t const & dst, xmessage_t const & message, std::error_code & ec) {
    assert(src.zone_id() != common::xfrozen_zone_id);
    assert(dst.zone_id() != common::xfrozen_zone_id);
    if (!running()) {
        ec = xvnetwork_errc2_t::vhost_not_run;
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return;
    }

    if (common::broadcast(dst.network_id()) || common::broadcast(dst.zone_id()) || common::broadcast(dst.cluster_id()) || common::broadcast(dst.group_id()) ||
        common::broadcast(dst.slot_id())) {
        ec = xvnetwork_errc2_t::invalid_dst_address;
        xwarn("%s %s. dst address is a broadcast address %s", vnetwork_category2().name(), ec.message().c_str(), dst.to_string().c_str());
        return;
    }

    xvnetwork_message_t const vmsg{src, dst, message, m_chain_timer->logic_time()};
    auto const bytes = codec::msgpack_encode(vmsg);

    if (account_address() == dst.account_address()) {
        on_network_data_ready(account_address(), bytes);
    } else {
        assert(!dst.account_address().empty());
    }

    m_network_driver->send_to(src.xip2(), dst.xip2(), bytes, ec);

    msg_metrics(vmsg, metrics::message_send_category_begin);
}

void xtop_vhost::broadcast(common::xnode_address_t const & src, common::xnode_address_t const & dst, xmessage_t const & message, std::error_code & ec) {
    if (!running()) {
        ec = xvnetwork_errc2_t::vhost_not_run;
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return;
    }

    if (!common::broadcast(dst.network_id()) && !common::broadcast(dst.zone_id()) && !common::broadcast(dst.cluster_id()) && !common::broadcast(dst.group_id()) &&
        !common::broadcast(dst.slot_id())) {
        ec = xvnetwork_errc2_t::invalid_dst_address;
        xwarn("%s %s. dst address is not a broadcast address %s", ec.category().name(), ec.message().c_str(), dst.to_string().c_str());
        return;
    }

    if (src.zone_id() == common::xfrozen_zone_id && dst.zone_id() == common::xfrozen_zone_id) {
        xvnetwork_message_t const vmsg{src, dst, message, m_chain_timer->logic_time()};
        assert(common::get_message_category(vmsg.message_id()) == xmessage_category_sync);
        auto bytes = codec::msgpack_encode(vmsg);
        auto new_hash_val = base::xhash32_t::digest(std::string((char *)bytes.data(), bytes.size()));
        xinfo("[vnetwork]xtop_vhost::broadcast frozen [vnet hash: %" PRIx64 "] [msg hash: %u] [xxh32 to_hash:%" PRIu32 "]", vmsg.hash(), message.hash(), new_hash_val);
        assert(m_network_driver);
        m_network_driver->broadcast(src.xip2(), bytes, ec);
        return;
    }
    assert(src.zone_id() != common::xfrozen_zone_id);
    assert(dst.zone_id() != common::xfrozen_zone_id);

    if (src.network_id() == dst.network_id() && src.zone_id() == dst.zone_id() && src.cluster_id() == dst.cluster_id() && src.group_id() == dst.group_id()) {
        // broadcast message in the same group

        if (src.logic_epoch() != dst.logic_epoch() && !dst.logic_epoch().empty()) {
            ec = xvnetwork_errc2_t::epoch_mismatch;
            xwarn("%s %s. src %s dst %s", ec.category().name(), ec.message().c_str(), src.to_string().c_str(), dst.to_string().c_str());
            return;
        }

        xvnetwork_message_t const vmsg{src, dst, message, m_chain_timer->logic_time()};
        auto bytes = codec::msgpack_encode(vmsg);

        auto new_hash_val = base::xhash32_t::digest(std::string((char *)bytes.data(), bytes.size()));
        xinfo("[vnetwork]xtop_vhost::broadcast [vnet hash: %" PRIx64 "] [msg hash: %u] [xxh32 to_hash:%" PRIu32 "]", vmsg.hash(), message.hash(), new_hash_val);

#if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_broadcast" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   1);
#endif
#if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_broadcast_size" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   bytes.size());
#endif
        // auto message_type = vmsg.message_id();
        // if (src.network_id() == common::xnetwork_id_t{top::config::to_chainid(XGET_CONFIG(chain_name))} && src.zone_id() == common::xfrozen_zone_id &&
        //     src.cluster_id() == common::xdefault_cluster_id && src.group_id() == common::xdefault_group_id &&
        //     (message_type == xmessage_id_sync_frozen_gossip || message_type == xmessage_id_sync_get_blocks || message_type == xmessage_id_sync_blocks ||
        //         message_type == xmessage_id_sync_frozen_broadcast_chain_state || message_type == xmessage_id_sync_frozen_response_chain_state)) {
        //     m_network_driver->send_to_through_root(convert_to_p2p_xip2(src), dst.node_id(), bytes, ec);
        //     msg_metrics(vmsg, metrics::message_send_category_begin);
        // } else {
        m_network_driver->spread_rumor(src.xip2(), dst.xip2(), bytes, ec);
        msg_metrics(vmsg, metrics::message_rumor_category_begin);
        // }
        // m_network_driver->spread_rumor(bytes);
    } else if (common::broadcast(dst.network_id()) || common::broadcast(dst.zone_id())) {
        // broadcast in the specified network

        xvnetwork_message_t const vnetwork_message{src, dst, message, m_chain_timer->logic_time()};
        auto const bytes_message = top::codec::msgpack_encode(vnetwork_message);

        on_network_data_ready(account_address(), bytes_message);  // broadcast to local vnet  // TODO remove codec part

        xdbg("%s broadcast msg %x from:%s to:%s hash %" PRIx64, vnetwork_category2().name(), message.id(), src.to_string().c_str(), dst.to_string().c_str(), message.hash());

#if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_broadcast_to_all" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   1);
#endif
#if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_broadcast_to_all_size" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   bytes_message.size());
#endif
        assert(m_network_driver);
        // m_network_driver->spread_rumor(bytes_message);
        m_network_driver->broadcast(src.xip2(), bytes_message, ec);
        msg_metrics(vnetwork_message, metrics::message_broad_category_begin);
    } else if (common::broadcast(dst.slot_id())) {
        // broadcast to another group

        common::xnode_address_t n_dst = dst;
        // fill up election_round() if associated
        if (common::has<common::xnode_type_t::consensus_auditor>(src.type()) && common::has<common::xnode_type_t::consensus_validator>(n_dst.type())) {
            // dst is child
            auto child_address = m_election_cache_data_accessor->child_addresses(src.sharding_address(), src.logic_epoch(), ec);
            if (ec) {
                xwarn("%s ec category: %s ec msg: %s", vnetwork_category2().name(), ec.category().name(), ec.message().c_str());
                assert(false);
            }
            for (auto const & _child : child_address) {
                if (_child.sharding_address() == n_dst.sharding_address()) {
                    n_dst = common::xnode_address_t{n_dst.sharding_address(), _child.election_round(), _child.group_size(), _child.associated_blk_height()};
                    break;
                }
            }
        } else if (common::has<common::xnode_type_t::consensus_validator>(src.type()) && common::has<common::xnode_type_t::consensus_auditor>(n_dst.type())) {
            // dst is parent
            auto parent_address = m_election_cache_data_accessor->parent_address(src.sharding_address(), src.logic_epoch(), ec);
            if (ec) {
                xwarn("%s ec category: %s ec msg: %s", vnetwork_category2().name(), ec.category().name(), ec.message().c_str());
                assert(false);
            }
            if (n_dst.sharding_address() == parent_address.sharding_address()) {
                n_dst = common::xnode_address_t{n_dst.sharding_address(), parent_address.election_round(), parent_address.group_size(), parent_address.associated_blk_height()};
            }
        } else if (common::has<common::xnode_type_t::consensus_validator>(src.type()) && common::has<common::xnode_type_t::consensus_validator>(n_dst.type())) {
            // dst is sibling
            auto parent_address = m_election_cache_data_accessor->parent_address(src.sharding_address(), src.logic_epoch(), ec);
            if (ec) {
                xwarn("%s ec category: %s ec msg: %s", vnetwork_category2().name(), ec.category().name(), ec.message().c_str());
                assert(false);
            }
            auto child_address = m_election_cache_data_accessor->child_addresses(parent_address.sharding_address(), parent_address.logic_epoch(), ec);
            if (ec) {
                xwarn("%s ec category: %s ec msg: %s", vnetwork_category2().name(), ec.category().name(), ec.message().c_str());
                assert(false);
            }
            for (auto const & _child : child_address) {
                if (_child.sharding_address() == n_dst.sharding_address()) {
                    n_dst = common::xnode_address_t{n_dst.sharding_address(), _child.election_round(), _child.group_size(), _child.associated_blk_height()};
                    break;
                }
            }
        }

        if (common::has<common::xnode_type_t::evm_auditor>(src.type()) && common::has<common::xnode_type_t::evm_validator>(n_dst.type())) {
            // dst is child
            auto child_address = m_election_cache_data_accessor->child_addresses(src.sharding_address(), src.logic_epoch(), ec);
            if (ec) {
                xwarn("%s ec category: %s ec msg: %s", vnetwork_category2().name(), ec.category().name(), ec.message().c_str());
                assert(false);
            }
            for (auto const & _child : child_address) {
                if (_child.sharding_address() == n_dst.sharding_address()) {
                    n_dst = common::xnode_address_t{n_dst.sharding_address(), _child.election_round(), _child.group_size(), _child.associated_blk_height()};
                    break;
                }
            }
        } else if (common::has<common::xnode_type_t::evm_validator>(src.type()) && common::has<common::xnode_type_t::evm_auditor>(n_dst.type())) {
            // dst is parent
            auto parent_address = m_election_cache_data_accessor->parent_address(src.sharding_address(), src.logic_epoch(), ec);
            if (ec) {
                xwarn("%s ec category: %s ec msg: %s", vnetwork_category2().name(), ec.category().name(), ec.message().c_str());
                assert(false);
            }
            if (n_dst.sharding_address() == parent_address.sharding_address()) {
                n_dst = common::xnode_address_t{n_dst.sharding_address(), parent_address.election_round(), parent_address.group_size(), parent_address.associated_blk_height()};
            }
        }

        xvnetwork_message_t const vmsg{src, n_dst, message, m_chain_timer->logic_time()};
        auto const bytes_message = top::codec::msgpack_encode(vmsg);

        auto const hash_val = base::xhash64_t::digest(std::string((char *)bytes_message.data(), bytes_message.size()));
        xinfo("%s %s forward_broadcast_message [type: %" PRIx32 "] [msg hash: %" PRIx64 "][bytes hash:%" PRIx64 "][msgid:%" PRIx32 "][%s][%s]",
              vnetwork_category2().name(),
              account_address().to_string().c_str(),
              static_cast<std::uint32_t>(message.type()),
              message.hash(),
              hash_val,
              static_cast<std::uint32_t>(message.id()),
              src.to_string().c_str(),
              n_dst.to_string().c_str());

#if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_forward_broadcast_message" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   1);
#endif
#if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_forward_broadcast_message_size" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   bytes_message.size());
#endif
        on_network_data_ready(account_address(), bytes_message);

        // m_network_driver->spread_rumor(bytes_message);
        m_network_driver->spread_rumor(src.xip2(), n_dst.xip2(), bytes_message, ec);
        msg_metrics(vmsg, metrics::message_rumor_category_begin);
    } else {
        ec = xvnetwork_errc2_t::not_supported, xwarn("%s %s", ec.category().name(), ec.message().c_str());
    }
}

void xtop_vhost::on_network_data_ready(common::xnode_id_t const &, xbyte_buffer_t const & bytes) {
#if VHOST_METRICS
    XMETRICS_COUNTER_INCREMENT("vhost_on_data_ready_called", 1);
#endif
    try {
        if (!running()) {
            xwarn("[vnetwork] vhost not run");
            return;
        }

#if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_total_size_of_all_messages", bytes.size());
#endif
        m_message_queue.push(bytes);
    } catch (std::exception const & eh) {
        xwarn("[vnetwork] std::exception exception caught: %s", eh.what());
    } catch (...) {
        xerror("[vnetwork] unknown exception caught");
    }
}

/*
 * this is the callback function ,which is called by the lower module,
 * refactor by @Charles.Liu 2020.05.08
 */
void xtop_vhost::do_handle_network_data() {
    assert(m_vhost_thread_id == std::this_thread::get_id());

#if defined DEBUG
    auto self = shared_from_this();
    xscope_executer_t do_handle_network_data_exit_verifier{[this, self] { assert(!running()); }};
#endif
    while (running()) {
        try {
            auto all_byte_messages = m_message_queue.wait_and_pop_all();

            // XMETRICS_FLOW_COUNT("vhost_handle_data_ready_called", all_byte_messages.size());

#if defined(XENABLE_VHOST_BENCHMARK)
            auto xxbegin = std::chrono::high_resolution_clock::now();
#endif
            for (auto & bytes : all_byte_messages) {
                if (!running()) {
                    xwarn("[vnetwork] vhost is not running!");
                    break;
                }

                try {
                    if (bytes.empty()) {
                        // this may be a stop notification message.
                        // anyway, for an empty message, just ignore it.
                        xwarn("[vnetwork] message byte empty!");
                        continue;
                    }

                    // todo check decode return value.
                    auto vnetwork_message = top::codec::msgpack_decode<xvnetwork_message_t>(bytes);
                    XMETRICS_GAUGE(metrics::vhost_recv_msg, 1);
                    auto const & message = vnetwork_message.message();
                    auto const & receiver = vnetwork_message.receiver();
                    auto const & sender = vnetwork_message.sender();
                    auto const msg_time = vnetwork_message.logic_time();

                    xdbg("[vnetwork] message hash: %" PRIx64 " , before filter:s&r sender is %s , receiver is %s",
                         vnetwork_message.hash(),
                         sender.to_string().c_str(),
                         receiver.to_string().c_str());

                    auto const message_category = common::get_message_category(vnetwork_message.message_id());
                    switch (message_category) {
#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wswitch"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wswitch"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif
                    case xmessage_category_consensus:
                    {
                        XMETRICS_GAUGE(metrics::message_category_consensus_contains_duplicate, 1);
                        break;
                    }
                    case xmessage_category_timer:
                    {
                        XMETRICS_GAUGE(metrics::message_category_timer_contains_duplicate, 1);
                        break;
                    }
                    case xmessage_category_txpool:
                    {
                        XMETRICS_GAUGE(metrics::message_category_txpool_contains_duplicate, 1);
                        break;
                    }

                    case xmessage_category_rpc:
                    {
                        XMETRICS_GAUGE(metrics::message_category_rpc_contains_duplicate, 1);
                        break;
                    }

                    case xmessage_category_sync:
                    {
                        XMETRICS_GAUGE(metrics::message_category_sync_contains_duplicate, 1);
                        break;
                    }

                    case xmessage_category_state_sync:
                    {
                        XMETRICS_GAUGE(metrics::message_category_state_sync_contains_duplicate, 1);
                        break;
                    }

                    case xmessage_block_broadcast:
                    {
                        XMETRICS_GAUGE(metrics::message_block_broadcast_contains_duplicate, 1);
                        break;
                    }

                    case xmessage_category_relay:
                    {
                        XMETRICS_GAUGE(metrics::message_category_relay_contains_duplicate, 1);
                        break;
                    }
#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif
                    default:
                    {
                        assert(false);
                        XMETRICS_GAUGE(metrics::message_category_unknown_contains_duplicate, 1);
                        break;
                    }
                    }
                    #if VHOST_METRICS
                    XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(vnetwork_message.message().id()))) +
                                                   "_in_vhost_size" + std::to_string(static_cast<std::uint32_t>(vnetwork_message.message().id())),
                                               bytes.size());
                    #endif
                    std::error_code ec;
                    m_filter_manager->filter_message(vnetwork_message, ec);
                    if (ec) {
                        xinfo("[vnetwork] message filter: message id %" PRIx32 " hash %" PRIx64 " filted out",
                              static_cast<uint32_t>(message.id()),
                              static_cast<uint64_t>(message.hash()));
                        continue;
                    }

                    xinfo("[vnetwork] message hash: %" PRIx64 " , after  filter:s&r sender is %s z%" PRIu16 "g%" PRIu16 "s%" PRIu16 ", receiver is %s z%" PRIu16 "g%" PRIu16
                          "s%" PRIu16,
                          vnetwork_message.hash(),
                          sender.account_address().to_string().c_str(),
                          static_cast<uint16_t>(sender.zone_id().value()),
                          static_cast<uint16_t>(sender.group_id().value()),
                          static_cast<uint16_t>(sender.slot_id().value()),
                          receiver.account_address().empty() ? "'empty'" : receiver.account_address().to_string().c_str(),
                          static_cast<uint16_t>(receiver.zone_id().value()),
                          static_cast<uint16_t>(receiver.group_id().value()),
                          static_cast<uint16_t>(receiver.slot_id().value()));

                    std::vector<std::pair<common::xnode_address_t const, xmessage_ready_callback_t>> callbacks;
                    XLOCK_GUARD(m_callbacks_mutex) {
                        for (auto const & callback_info : m_callbacks) {
                            auto const & callback_addr = top::get<common::xnode_address_t const>(callback_info);
                            xdbg("[vnetwork] see callback address: %s", callback_addr.to_string().c_str());

                            auto contains = callback_addr.contains(receiver);
                            if (!contains) {
                                contains = receiver.contains(callback_addr);
                            }

                            if (!contains) {
                                #if VHOST_METRICS
                                XMETRICS_COUNTER_INCREMENT("vhost_discard_addr_not_match", 1);
                                #endif
                                xdbg("[vnetwork] callback at address %s not matched", callback_addr.to_string().c_str());
                                continue;
                            }
                            callbacks.push_back(callback_info);
                        }
                    }

                    if (callbacks.empty()) {
                        xinfo("[vnetwork] no callback found for message hash: %" PRIx64 " , sender is %s , receiver is %s msg id %" PRIx32 " msg category %" PRIx16,
                              vnetwork_message.hash(),
                              sender.to_string().c_str(),
                              receiver.to_string().c_str(),
                              static_cast<uint32_t>(message.id()),
                              static_cast<uint16_t>(common::get_message_category(message.id())));
                        continue;
                    }

                    for (auto & callback_info : callbacks) {
                        auto const & callback = top::get<xmessage_ready_callback_t>(callback_info);
                        if (callback) {
                            try {
                                // callback(sender, receiver, message);
                                xdbg("[vnetwork] send msg %" PRIx32 " (hash %" PRIx64 ") to callback %p at address %s",
                                     static_cast<std::uint32_t>(message.id()),
                                     message.hash(),
                                     &callback,
                                     top::get<common::xnode_address_t const>(callback_info).to_string().c_str());
#ifdef VHOST_METRICS
                                char msg_info[30] = {0};
                                snprintf(msg_info, 29, "%x|%" PRIx64, static_cast<uint32_t>(vnetwork_message.message().id()), message.hash());
                                XMETRICS_TIME_RECORD_KEY_WITH_TIMEOUT("vhost_handle_data_callback", msg_info, uint32_t(100000));
#endif
                                XMETRICS_GAUGE(metrics::vhost_recv_callback, 1);
                                callback(sender, message, msg_time);
                            } catch (std::exception const & eh) {
                                xerror("[vnetwork] exception caught from callback: %s", eh.what());
                            }
                        } else {
                            xerror("[vnetwork] callback not registered");
                        }
                    }
                } catch (top::error::xtop_error_t const & eh) {
                    xwarn("[vnetwork] catches top error. category %s; error code %d; eh msg: %s; ec msg: %s", eh.code().category().name(), eh.code().value(), eh.what(), eh.code().message().c_str());
                } catch (std::exception const & eh) {
                    xwarn("[vnetwork] catches std::exception: %s", eh.what());
                } catch (...) {
                    xwarn("[vnetwork] catches an unknown exception");
                }
            }

#if defined(XENABLE_VHOST_BENCHMARK)
            auto xxend = std::chrono::high_resolution_clock::now();
            auto ms = static_cast<std::size_t>(std::chrono::duration_cast<std::chrono::milliseconds>(xxend - xxbegin).count());
            xwarn("vhost processed in %zu milliseconds against %zu packs => tps = %lf", ms, all_byte_messages.size(), static_cast<double>(all_byte_messages.size()) * 1000 / ms);
#endif


        } catch (std::exception const & eh) {
            xwarn("[vnetwork] catches std::exception: %s", eh.what());
        } catch (...) {
            xwarn("[vnetwork] catches an unknown exception");
        }
    }
}

NS_END2
