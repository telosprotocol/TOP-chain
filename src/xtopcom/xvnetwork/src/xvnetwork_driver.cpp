// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvnetwork/xvnetwork_driver.h"

#include "xbase/xlog.h"
#include "xbasic/xerror/xerror.h"
#include "xbasic/xthreading/xutility.h"
#include "xbasic/xutility.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xbook_id_mapping.h"
#include "xdata/xtable_id_mapping.h"
#include "xmetrics/xmetrics.h"
#include "xvnetwork/xvnetwork_error.h"
#include "xvnetwork/xvnetwork_error2.h"

#include <cassert>
#include <cinttypes>
#include <ctime>
#include <random>

NS_BEG2(top, vnetwork)

static std::vector<std::uint8_t> book_ids{};
static std::once_flag book_ids_once_flag{};
static constexpr std::size_t book_id_count{enum_vbucket_has_books_count};

xtop_vnetwork_driver::xtop_vnetwork_driver(observer_ptr<xvhost_face_t> const & vhost,
                                           observer_ptr<election::cache::xdata_accessor_face_t> const & election_data_accessor,
                                           common::xnode_address_t const & address,
                                           common::xelection_round_t const & joined_election_round)
  : m_vhost{vhost}, m_election_cache_data_accessor{election_data_accessor}, m_address{address}, m_joined_election_round(joined_election_round) {
    if (m_vhost == nullptr) {
        top::error::throw_error({xvnetwork_errc_t::vhost_empty}, "constructing xvnetwork_driver_t at address " + m_address.to_string());
    }
}

void xtop_vnetwork_driver::start() {
    assert(m_vhost);
    m_vhost->register_message_ready_notify(
        address(), std::bind(&xtop_vnetwork_driver::on_vhost_message_data_ready, shared_from_this(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    assert(!running());
    running(true);
}

void xtop_vnetwork_driver::stop() {
    assert(running());
    running(false);
    assert(m_vhost);
    m_vhost->unregister_message_ready_notify(address());
}

common::xnetwork_id_t xtop_vnetwork_driver::network_id() const noexcept {
    return m_vhost->network_id();
}

common::xnode_address_t xtop_vnetwork_driver::address() const {
    XLOCK(m_address_mutex);
    return m_address;
}

common::xnode_address_t xtop_vnetwork_driver::parent_group_address() const {
    assert(m_vhost);
    return m_vhost->parent_group_address(m_address);
}

void xtop_vnetwork_driver::send_to(common::xnode_address_t const & to, xmessage_t const & message, std::error_code & ec) {
    assert(!ec);
    assert(m_vhost);
    #if VHOST_METRICS
    XMETRICS_COUNTER_INCREMENT("vnetwork_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vnetwork_driver_send_to" +
                                   std::to_string(static_cast<std::uint32_t>(message.id())),
                               1);
    #endif
    m_vhost->send_to(m_address, to, message, ec);
}

void xtop_vnetwork_driver::send_to(common::xip2_t const & dst, xmessage_t const & message, std::error_code & ec) {
    assert(!ec);
    assert(m_vhost);
    if (!running()) {
        ec = xvnetwork_errc2_t::vnetwork_driver_not_run;
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return;
    }

    if (common::broadcast(dst.network_id()) || common::broadcast(dst.zone_id()) || common::broadcast(dst.cluster_id()) ||
        common::broadcast(dst.group_id()) || common::broadcast(dst.slot_id()) || common::broadcast(dst.size()) || common::broadcast(dst.height())) {
        ec = xvnetwork_errc2_t::invalid_dst_address;
        xwarn("%s %s. dst address is a broadcast address %s", vnetwork_category2().name(), ec.message().c_str(), dst.to_string().c_str());
        return;
    }

    auto dst_node_id = m_election_cache_data_accessor->account_address_from(dst, ec);
    if (ec) {
        xwarn("%s ec category: %s ec msg: %s", vnetwork_category2().name(), ec.category().name(), ec.message().c_str());
        return;
    }

    auto dst_election_epoch = m_election_cache_data_accessor->election_epoch_from(dst, ec);
    if (ec) {
        xwarn("%s ec category: %s ec msg: %s", vnetwork_category2().name(), ec.category().name(), ec.message().c_str());
        return;
    }

    common::xnode_address_t to{common::xsharding_address_t{dst.network_id(), dst.zone_id(), dst.cluster_id(), dst.group_id()},
                                common::xaccount_election_address_t{dst_node_id, dst.slot_id()},
                                dst_election_epoch,
                                dst.size(),
                                dst.height()};
    m_vhost->send_to(m_address, to, message, ec);
}

void xtop_vnetwork_driver::broadcast(common::xip2_t const & dst, xmessage_t const & message, std::error_code & ec) {
    assert(m_vhost);
    if (!running()) {
        ec = xvnetwork_errc2_t::vnetwork_driver_not_run;
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return;
    }

    if (!common::broadcast(dst.network_id()) && !common::broadcast(dst.zone_id()) && !common::broadcast(dst.cluster_id()) && !common::broadcast(dst.group_id()) &&
        !common::broadcast(dst.slot_id()) && !common::broadcast(dst.size()) && !common::broadcast(dst.height())) {
        ec = xvnetwork_errc2_t::invalid_dst_address;
        xwarn("%s %s. dst address is not a broadcast address %s", ec.category().name(), ec.message().c_str(), dst.to_string().c_str());
        return;
    }

    common::xnode_address_t to{
        common::xsharding_address_t{dst.network_id(), dst.zone_id(), dst.cluster_id(), dst.group_id()}, common::xelection_round_t{}, dst.size(), dst.height()};
    m_vhost->broadcast(m_address, to, message, ec);
}

common::xnode_id_t const & xtop_vnetwork_driver::account_address() const noexcept {
    assert(m_vhost);
    return m_vhost->account_address();
}

std::map<common::xslot_id_t, data::xnode_info_t> xtop_vnetwork_driver::neighbors_info2() const {
    assert(m_vhost);
    return m_vhost->members_info_of_group2(address().cluster_address(), address().election_round());
}

std::map<common::xslot_id_t, data::xnode_info_t> xtop_vnetwork_driver::parents_info2() const {
    assert(m_vhost);
    auto const & parent_cluster_addr = m_vhost->parent_group_address(address());
    return m_vhost->members_info_of_group2(parent_cluster_addr.cluster_address(), parent_cluster_addr.election_round());
}

std::map<common::xslot_id_t, data::xnode_info_t> xtop_vnetwork_driver::children_info2(common::xgroup_id_t const & gid, common::xelection_round_t const & election_round) const {
    try {
        assert(m_vhost);
        assert(gid >= common::xvalidator_group_id_begin && gid < common::xvalidator_group_id_end);

        xcluster_address_t child_cluster_address{address().network_id(), address().zone_id(), address().cluster_id(), gid};

        return m_vhost->members_info_of_group2(child_cluster_address, election_round);
    } catch (top::error::xtop_error_t const & eh) {
        xwarn("[vnetwork] xtop_error_t exception caught: cateogry:%s; msg:%s; error code:%d; error msg:%s", eh.code().category().name(), eh.what(), eh.code().value(), eh.code().message().c_str());
    } catch (std::exception const & eh) {
        xwarn("[vnetwork] std::exception exception caught: %s", eh.what());
    } catch (...) {
        xerror("[vnetwork] unknown exception caught");
    }

    return {};
}

observer_ptr<xvhost_face_t> xtop_vnetwork_driver::virtual_host() const noexcept {
    return m_vhost;
}

common::xnode_type_t xtop_vnetwork_driver::type() const noexcept {
    return real_part_type(m_address.cluster_address().type());
}

std::vector<common::xnode_address_t> xtop_vnetwork_driver::archive_addresses(common::xnode_type_t node_type) const {
    assert(m_vhost != nullptr);
    std::vector<common::xnode_address_t> result;
    switch (node_type) {
    case common::xnode_type_t::storage_archive:
    {
        auto const & tmp = m_vhost->members_info_of_group2(common::build_archive_sharding_address(common::xarchive_group_id, network_id()), common::xelection_round_t::max());
        result.reserve(tmp.size());

        for (auto const & n : tmp) {
            result.push_back(top::get<data::xnode_info_t>(n).address);
        }

        break;
    }

    case common::xnode_type_t::storage_exchange:
    {
        auto const & tmp = m_vhost->members_info_of_group2(common::build_exchange_sharding_address(network_id()), common::xelection_round_t::max());
        result.reserve(tmp.size());

        for (auto const & n : tmp) {
            result.push_back(top::get<data::xnode_info_t>(n).address);
        }

        break;
    }

    default:
        assert(false);
        break;
    }

    return result;
}

std::vector<common::xnode_address_t> xtop_vnetwork_driver::fullnode_addresses(std::error_code & ec) const {
    assert(m_vhost != nullptr);
    std::vector<common::xnode_address_t> result;

    auto tmp = m_vhost->members_info_of_group2(common::build_fullnode_group_address(network_id()), common::xelection_round_t::max());
    std::transform(std::begin(tmp), std::end(tmp), std::back_inserter(result), [](std::pair<common::xslot_id_t const, data::xnode_info_t> & datum) -> common::xnode_address_t {
        return top::get<data::xnode_info_t>(std::move(datum)).address;
    });

    return result;
}
std::vector<common::xnode_address_t> xtop_vnetwork_driver::relay_addresses(std::error_code & ec) const {
    assert(m_vhost != nullptr);
    std::vector<common::xnode_address_t> result;

    common::xelection_round_t election_round;
    auto tmp = m_vhost->members_info_of_group2(common::build_relay_group_address(network_id()), election_round);
    std::transform(std::begin(tmp), std::end(tmp), std::back_inserter(result), [](std::pair<common::xslot_id_t const, data::xnode_info_t> & datum) -> common::xnode_address_t {
        return top::get<data::xnode_info_t>(std::move(datum)).address;
    });

    return result;
}
std::vector<std::uint16_t> xtop_vnetwork_driver::table_ids() const {
    xdbg("[vnetwork driver] getting table ids for %s", address().to_string().c_str());
    std::error_code ec;
    auto associated_parent_address = m_election_cache_data_accessor->parent_address(m_address.group_address(), m_address.logic_epoch(), ec);
    return data::get_table_ids(m_address.group_address(),
                               m_vhost->parent_group_address(m_address).group_id());

    //std::vector<std::uint16_t> book_ids;
    //book_ids.reserve(enum_vbucket_has_books_count);

    //switch (type()) {
    //case common::xnode_type_t::fullnode:
    //    XATTRIBUTE_FALLTHROUGH;
    //case common::xnode_type_t::storage_archive:
    //    XATTRIBUTE_FALLTHROUGH;
    //case common::xnode_type_t::storage_exchange:
    //    XATTRIBUTE_FALLTHROUGH;
    //case common::xnode_type_t::committee: {
    //    auto const zone_range = data::book_ids_belonging_to_zone(common::xcommittee_zone_id, 1, {0, static_cast<std::uint16_t>(enum_vbucket_has_books_count)});
    //    for (auto i = zone_range.first; i < zone_range.second; ++i) {
    //        book_ids.push_back(i);
    //    }

    //    break;
    //}

    //case common::xnode_type_t::zec: {
    //    uint32_t zone_count = XGET_CONFIG(zone_count);
    //    auto const zone_range = data::book_ids_belonging_to_zone(m_address.zone_id(), zone_count, {0, static_cast<std::uint16_t>(enum_vbucket_has_books_count)});
    //    for (auto i = zone_range.first; i < zone_range.second; ++i) {
    //        book_ids.push_back(i);
    //    }

    //    break;
    //}

    //case common::xnode_type_t::consensus_auditor: {
    //    uint32_t zone_count = XGET_CONFIG(zone_count);
    //    uint32_t cluster_count = XGET_CONFIG(cluster_count);

    //    auto const auditor_group_count = XGET_CONFIG(auditor_group_count);

    //    auto const zone_range = data::book_ids_belonging_to_zone(m_address.zone_id(), zone_count, {0, static_cast<std::uint16_t>(enum_vbucket_has_books_count)});
    //    auto const cluster_range = data::book_ids_belonging_to_cluster(m_address.cluster_id(), cluster_count, zone_range);
    //    auto const advance_range = data::book_ids_belonging_to_auditor_group(m_address.group_id(), auditor_group_count, cluster_range);
    //    for (auto i = advance_range.first; i < advance_range.second; ++i) {
    //        book_ids.push_back(i);
    //    }

    //    break;
    //}

    //case common::xnode_type_t::consensus_validator: {
    //    try {
    //        std::uint16_t zone_count = XGET_CONFIG(zone_count);
    //        std::uint16_t cluster_count = XGET_CONFIG(cluster_count);

    //        auto const auditor_group_count = XGET_CONFIG(auditor_group_count);

    //        auto const validator_group_count = XGET_CONFIG(validator_group_count);

    //        auto const zone_range = data::book_ids_belonging_to_zone(m_address.zone_id(), zone_count, {0, static_cast<std::uint16_t>(enum_vbucket_has_books_count)});
    //        auto const cluster_range = data::book_ids_belonging_to_cluster(m_address.cluster_id(), cluster_count, zone_range);
    //        auto const consensus_range = data::book_ids_belonging_to_validator_group(
    //            m_vhost->parent_group_address(m_address).group_id(), m_address.group_id(), auditor_group_count, validator_group_count, cluster_range);
    //        for (auto i = consensus_range.first; i < consensus_range.second; ++i) {
    //            book_ids.push_back(i);
    //        }
    //    } catch (top::error::xtop_error_t const & eh) {
    //        xwarn("[vnetwork] xtop_error_t exception caught: cateogry:%s; msg:%s; error code:%d; error msg:%s", eh.code().category().name(), eh.what(), eh.code().value(), eh.code().message().c_str());
    //    } catch (std::exception const & eh) {
    //        xwarn("[vnetwork driver] caught std::exception %s", eh.what());
    //    } catch (...) {
    //        xerror("[vnetwork driver] caught unknown exception");
    //    }
    //    break;
    //}

    //case common::xnode_type_t::frozen: {
    //    // TODO george
    //    // get all beacon table
    //    book_ids.push_back(0);
    //    break;
    //}

    //case common::xnode_type_t::edge: {
    //    // TODO george
    //    // get all zec table
    //    book_ids.push_back(0);
    //    break;
    //}

    //case common::xnode_type_t::evm_auditor:
    //    XATTRIBUTE_FALLTHROUGH;
    //case common::xnode_type_t::evm_validator:
    //    XATTRIBUTE_FALLTHROUGH;
    //case common::xnode_type_t::relay: {
    //    break;
    //}


    //default: {
    //    assert(false);
    //    break;
    //}
    //}

    //std::vector<std::uint16_t> table_ids;
    //table_ids.reserve(book_ids.size() * enum_vbook_has_tables_count);

    //for (auto const book_id : book_ids) {
    //    for (auto i = 0; i < enum_vbook_has_tables_count; ++i) {
    //        table_ids.push_back(book_id * enum_vbook_has_tables_count + i);
    //    }
    //}

    //// rec and zec only use some tables
    //if (common::has<common::xnode_type_t::committee>(type()) || common::has<common::xnode_type_t::frozen>(type())) {
    //    table_ids.resize(MAIN_CHAIN_REC_TABLE_USED_NUM);
    //} else if (common::has<common::xnode_type_t::zec>(type()) || common::has<common::xnode_type_t::edge>(type())) {
    //    table_ids.resize(MAIN_CHAIN_ZEC_TABLE_USED_NUM);
    //} else if (common::has<common::xnode_type_t::evm>(type())) {
    //    table_ids.resize(MAIN_CHAIN_EVM_TABLE_USED_NUM);
    //} else if (common::has<common::xnode_type_t::relay>(type())) {
    //    table_ids.resize(MAIN_CHAIN_RELAY_TABLE_USED_NUM);
    //}

    //return table_ids;
}

void xtop_vnetwork_driver::on_vhost_message_data_ready(common::xnode_address_t const & src, xmessage_t const & msg, std::uint64_t const msg_time) {
    if (!running()) {
        xwarn("[vnetwork driver] received msg %" PRIx64 " but vnetwork dirver %s isn't started yet", msg.hash(), address().to_string().c_str());
        return;
    }

    if (src == address()) {
        xwarn("[vnetwork driver] received msg %" PRIx64 " but from itself.", msg.hash());
        return;
    }

    XMETRICS_GAUGE(metrics::vnode_recv_msg, 1);

    XLOCK_GUARD(m_message_cache_mutex) {
        int ret = 0;
        #if VHOST_METRICS
        XMETRICS_TIME_RECORD("vnetwork_lru_cache");
        #endif
        if (m_message_cache.contains(msg.hash())) {
            xkinfo("[vnetwork driver] received a dup msg %" PRIx64 " message id %" PRIx32, msg.hash(), static_cast<std::uint32_t>(msg.id()));
            ret = -1;
        }

        if (ret == -1) {
            return;
        }
    }

    #if VHOST_METRICS
    XMETRICS_COUNTER_INCREMENT("vnetwork_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(msg.id()))) + "_in_vnetwork_driver_filtered" +
                                   std::to_string(static_cast<std::uint32_t>(msg.id())),
                               1);

    XMETRICS_COUNTER_INCREMENT("vnetwork_driver_received", 1);
    #endif

    xvnetwork_message_ready_callback_t callback;
    auto const message_id = msg.id();
    auto const message_cagegory = get_message_category(message_id);
    XLOCK_GUARD(m_callbacks_mutex) {
        auto const it = m_callbacks.find(message_cagegory);
        if (it != std::end(m_callbacks)) {
            xdbg("[vnetwork driver] host %s received msg %" PRIx64 " message id %" PRIx32 " at address %s from %s",
                 account_address().to_string().c_str(),
                 msg.hash(),
                 static_cast<std::uint32_t>(msg.id()),
                 address().to_string().c_str(),
                 src.to_string().c_str());
            callback = top::get<xvnetwork_message_ready_callback_t>(*it);
        }
    }

    if (callback) {
        #if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vnetwork_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(msg.id()))) + "_in_vnetwork_driver_callback" +
                                       std::to_string(static_cast<std::uint32_t>(msg.id())),
                                   1);
        #endif

        xdbg("[vnetwork driver] host %s push msg %" PRIx64 " message id %" PRIx32 " at address %s to callback from %s",
             account_address().to_string().c_str(),
             msg.hash(),
             static_cast<std::uint32_t>(msg.id()),
             address().to_string().c_str(),
             src.to_string().c_str());
        {
            auto const message_category = common::get_message_category(msg.id());
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
                XMETRICS_GAUGE(metrics::message_category_consensus, 1);
                break;
            }
            case xmessage_category_timer:
            {
                XMETRICS_GAUGE(metrics::message_category_timer, 1);
                break;
            }
            case xmessage_category_txpool:
            {
                XMETRICS_GAUGE(metrics::message_category_txpool, 1);
                break;
            }

            case xmessage_category_rpc:
            {
                XMETRICS_GAUGE(metrics::message_category_rpc, 1);
                break;
            }

            case xmessage_category_sync:
            {
                XMETRICS_GAUGE(metrics::message_category_sync, 1);
                break;
            }

            case xmessage_category_state_sync:
            {
                XMETRICS_GAUGE(metrics::message_category_state_sync, 1);
                break;
            }

            case xmessage_block_broadcast:
            {
                XMETRICS_GAUGE(metrics::message_block_broadcast, 1);
                break;
            }
            case xmessage_category_relay:
            {
                XMETRICS_GAUGE(metrics::message_category_relay, 1);
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
                XMETRICS_GAUGE(metrics::message_category_unknown, 1);
                break;
            }
            }
        }
        XMETRICS_GAUGE(metrics::vnode_recv_callback, 1);
        callback(src, msg, msg_time);
        XLOCK_GUARD(m_message_cache_mutex) { m_message_cache.insert({msg.hash(), std::time(nullptr)}); }
    } else {
        xwarn("[vnetwork driver] no callback found for message %" PRIx64 " with message id %" PRIx32 " timer height %" PRIu64,
              msg.hash(),
              static_cast<std::uint32_t>(msg.id()),
              msg_time);
    }
}

common::xelection_round_t const & xtop_vnetwork_driver::joined_election_round() const {
    return m_joined_election_round;
}

NS_END2
