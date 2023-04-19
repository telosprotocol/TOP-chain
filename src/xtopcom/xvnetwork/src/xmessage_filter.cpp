// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvnetwork/xmessage_filter.h"

#include "xcommon/xmessage_category.h"
#include "xelection/xcache/xgroup_element.h"
#include "xelection/xdata_accessor_error.h"
#include "xelection/xdata_accessor_error.h"
#include "xmetrics/xmetrics.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xcommon/xmessage_id.h"
#include "xvnetwork/xmessage_filter_manager.h"
#include "xvnetwork/xvhost_face.h"
#include "xvnetwork/xvnetwork_error2.h"

#include <cinttypes>

NS_BEG2(top, vnetwork)

xtop_message_filter_sender::xtop_message_filter_sender(observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                                                       observer_ptr< election::cache::xdata_accessor_face_t> const & election_data_accessor) noexcept
    : m_vhost{ vhost }, m_election_data_accessor{ election_data_accessor } {
}

xfilter_result_t xtop_message_filter_sender::filter(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);

    // 1. verify if message is empty. for empty message, just ignore.
    if (vnetwork_message.empty()) {
        ec = xvnetwork_errc2_t::empty_message;

        xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", vnetwork message empty", vnetwork_message.hash());

        return xfilter_result_t::stop_filtering;
    }

    auto const & message = vnetwork_message.message();
    auto const & recver = vnetwork_message.receiver();
    auto const & sender = vnetwork_message.sender();

#if defined(DEBUG)
    xdbg("[vnetwork] recv message :%" PRIx32 " (hash %" PRIx64 " logic time %" PRIu64 ") from:%s to:%s",
         static_cast<std::uint32_t>(message.id()),
         message.hash(),
         vnetwork_message.logic_time(),
         sender.to_string().c_str(),
         recver.to_string().c_str());

    xdbg("[vnetwork] %s receives message %" PRIx64 " from %s msg id %" PRIx32 " logic time %" PRIu64,
         m_vhost->account_address().to_string().c_str(),
         vnetwork_message.hash(),
         sender.to_string().c_str(),
         static_cast<std::uint32_t>(message.id()),
         vnetwork_message.logic_time());
#endif

    auto const & sender_network_id = sender.group_address().network_id();
    auto const & sender_zone_id = sender.group_address().zone_id();
    auto const & sender_cluster_id = sender.group_address().cluster_id();
    auto const & sender_group_id = sender.group_address().group_id();
    if (broadcast(sender_network_id) || broadcast(sender_zone_id)  ||
        broadcast(sender_cluster_id) || broadcast(sender_group_id)) {
        ec = xvnetwork_errc2_t::invalid_src_address;

        xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", %s receives a message %" PRIx32 " from %s to %s, but sender's address is a broadcast address",
              vnetwork_message.hash(),
              m_vhost->account_address().to_string().c_str(),
              static_cast<std::uint32_t>(message.id()),
              sender.to_string().c_str(),
              recver.to_string().c_str());

        return xfilter_result_t::stop_filtering;
    }

    bool const sender_is_from_consensus_group = common::has<common::xnode_type_t::consensus>(sender.type()) ||
                                                common::has<common::xnode_type_t::rec>(sender.type()) ||
                                                common::has<common::xnode_type_t::zec>(sender.type());
    if (sender_is_from_consensus_group) {
        auto const & sender_slot_id = sender.account_election_address().slot_id();
        if (broadcast(sender_slot_id)) {
            ec = xvnetwork_errc2_t::invalid_src_address;

            xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", %s receives a message %" PRIx32 " from %s to %s, but sender's address is a broadcast address",
                  vnetwork_message.hash(),
                  m_vhost->account_address().to_string().c_str(),
                  static_cast<std::uint32_t>(message.id()),
                  sender.to_string().c_str(),
                  recver.to_string().c_str());

            return xfilter_result_t::stop_filtering;
        }
    }

    // 2. verify sender info.
    if (sender.logic_epoch().empty()) {
        ec = xvnetwork_errc2_t::invalid_epoch;

        xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", %s receives a message %" PRIx32 " from %s to %s, but sender doesn't provide logic epoch",
              vnetwork_message.hash(),
              m_vhost->account_address().to_string().c_str(),
              static_cast<std::uint32_t>(message.id()),
              sender.to_string().c_str(),
              recver.to_string().c_str());

        return xfilter_result_t::stop_filtering;
    }

    if (sender.account_election_address().empty()) {
        ec = xvnetwork_errc2_t::invalid_account_address;

        xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", %s receives a message %" PRIx32 " from %s to %s, but sender's account address is empty",
              vnetwork_message.hash(),
              m_vhost->account_address().to_string().c_str(),
              static_cast<std::uint32_t>(message.id()),
              sender.to_string().c_str(),
              recver.to_string().c_str());

        return xfilter_result_t::stop_filtering;
    }

    return xfilter_result_t::continue_filtering;
}

xtop_message_filter_recver::xtop_message_filter_recver(observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                                                       observer_ptr< election::cache::xdata_accessor_face_t> const & election_data_accessor) noexcept
    : m_vhost{ vhost }, m_election_data_accessor{ election_data_accessor } {
}

xfilter_result_t xtop_message_filter_recver::filter(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);

    auto const & message = vnetwork_message.message();
    auto const & recver = vnetwork_message.receiver();

    auto const msg_time = vnetwork_message.logic_time();
    auto const local_time = m_vhost->last_logic_time();
    xdbg("[vnetwork] message logic time %" PRIu64, msg_time);

    if (recver.account_address().has_value() && recver.account_address() != m_vhost->account_address()) {
        ec = vnetwork::xvnetwork_errc2_t::invalid_account_address;
        xinfo("message_filter: %s recv message %" PRIx32 " hash %" PRIx64 " from %s to %s; dropped",
              m_vhost->account_address().to_string().c_str(),
             static_cast<uint32_t>(vnetwork_message.message_id()),
             static_cast<uint64_t>(vnetwork_message.hash()),
             vnetwork_message.sender().to_string().c_str(),
             vnetwork_message.receiver().to_string().c_str());

        return xfilter_result_t::stop_filtering;
    }

    if (common::broadcast(recver.network_id())) {
        return xfilter_result_t::stop_filtering;
    }

    // verify if network id embed in the message matches current chain.
    if (recver.network_id() != m_vhost->network_id()) {
        ec = xvnetwork_errc2_t::invalid_dst_address;

        xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", vnetwork message network id not matched: this network id %" PRIu32 "; sent to %" PRIu32 " ",
              vnetwork_message.hash(),
              static_cast<std::uint32_t>(m_vhost->network_id().value()),
              static_cast<std::uint32_t>(recver.network_id().value()));

        return xfilter_result_t::stop_filtering;
    }

    // verify if message is expired or not.
    // for a message if logic epoch not specified, we need to check the message's logic time, to see if this message's time is invalid.
    if (recver.logic_epoch().empty()) {
        constexpr std::uint64_t future_threshold{ 2 };
        constexpr std::uint64_t past_threshold{ 6 };

        auto const message_category = get_message_category(message.id());

        if ((local_time != 0) && (local_time + future_threshold < msg_time) && (message.id() != xmessage_block_broadcast_id && message_category != xmessage_category_timer)) {
            ec = xvnetwork_errc2_t::future_message;

            // receive a message from future, ignore
            xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", receive a message whose logic time is %" PRIu64 " which is much more newer than current node %" PRIu64,
                  vnetwork_message.hash(),
                  msg_time,
                  local_time);
            return xfilter_result_t::stop_filtering;
        }

        if ((msg_time != 0) && (msg_time + past_threshold < local_time) && (message.id() != xmessage_block_broadcast_id && message_category != xmessage_category_timer)) {
            ec = xvnetwork_errc2_t::expired_message;

            // receive a message from past, ignore
            xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", receive a message whose logic time is %" PRIu64 " which is much more older than current node %" PRIu64,
                  vnetwork_message.hash(),
                  msg_time,
                  local_time);

            return xfilter_result_t::stop_filtering;
        }
    }

    if (broadcast(recver.zone_id()) || broadcast(recver.cluster_id()) || broadcast(recver.group_id())) {
        return xfilter_result_t::stop_filtering;
    }

    return xfilter_result_t::continue_filtering;
}

xtop_message_filter_message_id::xtop_message_filter_message_id(observer_ptr<vnetwork::xvhost_face_t> const& vhost,
    observer_ptr<election::cache::xdata_accessor_face_t> const& data_accessor) noexcept
    : m_vhost{ vhost }, m_election_data_accessor{ data_accessor } {
}

xfilter_result_t xtop_message_filter_message_id::filter(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);

    auto const& message_id = vnetwork_message.message_id();
    auto const message_category = get_message_category(message_id);

    xdbg("message_filter: %s recv message %" PRIx32 " category %" PRIx16 " hash %" PRIx64 " from %s to %s ",
         m_vhost->account_address().to_string().c_str(),
        static_cast<uint32_t>(message_id),
         static_cast<uint16_t>(message_category),
        static_cast<uint64_t>(vnetwork_message.hash()),
        vnetwork_message.sender().to_string().c_str(),
        vnetwork_message.receiver().to_string().c_str());

    if (message_category == xmessage_category_timer) {
        return xfilter_result_t::stop_filtering;
    }

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wswitch"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wswitch"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif
    switch (message_id) {
    case xmessage_block_broadcast_id:
        XFALLTHROUGH;
    case xtxpool_msg_send_receipt:
        XFALLTHROUGH;
    case xtxpool_msg_recv_receipt:
        XFALLTHROUGH;
    case xtxpool_msg_pull_recv_receipt:
        XFALLTHROUGH;
    case xtxpool_msg_pull_confirm_receipt:
        XFALLTHROUGH;
    case xtxpool_msg_push_receipt:
        XFALLTHROUGH;
    case xtxpool_msg_pull_confirm_receipt_v2:
        XFALLTHROUGH;
    case xtxpool_msg_receipt_id_state:
    {
        assert(!broadcast(vnetwork_message.receiver().network_id()));
        assert(!broadcast(vnetwork_message.receiver().zone_id()));
        assert(!broadcast(vnetwork_message.receiver().cluster_id()));
        assert(!broadcast(vnetwork_message.receiver().group_id()));

        if (vnetwork_message.receiver().logic_epoch().empty()) {
            normalize_message_recver(vnetwork_message, m_vhost, m_election_data_accessor, ec);
            if (ec) {
                return xfilter_result_t::stop_filtering;
            }
        }

        return xfilter_result_t::stop_filtering;
    }

    default:
        return xfilter_result_t::continue_filtering;
    }
#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif
}

xtop_message_filter_recver_is_validator::xtop_message_filter_recver_is_validator(observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                                                                                 observer_ptr< election::cache::xdata_accessor_face_t> const & election_data_accessor) noexcept
    : m_vhost{ vhost }, m_election_data_accessor{ election_data_accessor } {
}

xfilter_result_t xtop_message_filter_recver_is_validator::filter(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    if (!common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.receiver().type()) &&
        !common::has<common::xnode_type_t::evm_validator>(vnetwork_message.receiver().type())) {
        return xfilter_result_t::continue_filtering;
    }

    if (filter_sender_from_nonconsensus_group(vnetwork_message, ec) && filter_sender_from_consensus_group(vnetwork_message, ec)) {
        return xfilter_result_t::continue_filtering;
    }

    return xfilter_result_t::stop_filtering;
}

bool xtop_message_filter_recver_is_validator::filter_sender_from_nonconsensus_group(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_validator>(vnetwork_message.receiver().type()));
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    auto const & sender = vnetwork_message.sender();
    if (!common::has<common::xnode_type_t::edge>(sender.type()) && !common::has<common::xnode_type_t::storage>(sender.type()) &&
        !common::has<common::xnode_type_t::fullnode>(sender.type())) {
        return true;
    }

    return filter_sender_from_edge(vnetwork_message, ec) && filter_sender_from_storage(vnetwork_message, ec) && filter_sender_from_fullnode(vnetwork_message, ec);
}

bool xtop_message_filter_recver_is_validator::filter_sender_from_consensus_group(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_validator>(vnetwork_message.receiver().type()));

    return filter_sender_from_rec(vnetwork_message, ec)       &&
           filter_sender_from_zec(vnetwork_message, ec)       &&
           filter_sender_from_validator(vnetwork_message, ec) &&
           filter_sender_from_auditor(vnetwork_message, ec);
}

bool xtop_message_filter_recver_is_validator::filter_sender_from_edge(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_validator>(vnetwork_message.receiver().type()));
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    if (!common::has<common::xnode_type_t::edge>(vnetwork_message.sender().type())) {
        return true;
    }

    // we don't allow edge to send message to validator directly.
    ec = xvnetwork_errc2_t::invalid_src_address;
    xinfo("[vnetwork][message_filter] hash: %" PRIx64 " node %s receives msg sent to %s from %s. ignored. error: %s",
            vnetwork_message.hash(),
          m_vhost->account_address().to_string().c_str(),
            vnetwork_message.receiver().to_string().c_str(),
            vnetwork_message.sender().to_string().c_str(),
            ec.message().c_str());

    return false;
}

bool xtop_message_filter_recver_is_validator::filter_sender_from_fullnode(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_validator>(vnetwork_message.receiver().type()));
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    if (!common::has<common::xnode_type_t::fullnode>(vnetwork_message.sender().type())) {
        return true;
    }

    auto const & recver = vnetwork_message.receiver();

    // fix receiver if necessary.
    if (recver.logic_epoch().empty()) {
        normalize_message_recver(vnetwork_message, m_vhost, m_election_data_accessor, ec);
    }

    return false;
}

bool xtop_message_filter_recver_is_validator::filter_sender_from_storage(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_validator>(vnetwork_message.receiver().type()));
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    if (!common::has<common::xnode_type_t::storage>(vnetwork_message.sender().type())) {
        return true;
    }

    auto const & recver = vnetwork_message.receiver();

    // fix receiver if necessary.
    if (recver.logic_epoch().empty()) {
        normalize_message_recver(vnetwork_message, m_vhost, m_election_data_accessor, ec);
        if (ec) {
            return false;
        }
    }

    return false;
}

bool xtop_message_filter_recver_is_validator::filter_sender_from_rec(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_validator>(vnetwork_message.receiver().type()));
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    if (!common::has<common::xnode_type_t::rec>(vnetwork_message.sender().type())) {
        return true;
    }

    auto const & recver = vnetwork_message.receiver();

    // fix receiver if necessary.
    if (recver.logic_epoch().empty()) {
        normalize_message_recver(vnetwork_message, m_vhost, m_election_data_accessor, ec);
        if (ec) {
            return false;
        }
    }

    return false;
}

bool xtop_message_filter_recver_is_validator::filter_sender_from_zec(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_validator>(vnetwork_message.receiver().type()));
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    if (!common::has<common::xnode_type_t::zec>(vnetwork_message.sender().type())) {
        return true;
    }

    auto const & recver = vnetwork_message.receiver();

    // fix receiver if necessary.
    if (recver.logic_epoch().empty()) {
        normalize_message_recver(vnetwork_message, m_vhost, m_election_data_accessor, ec);
        if (ec) {
            return false;
        }
    }

    return false;
}

bool xtop_message_filter_recver_is_validator::filter_sender_from_validator(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_validator>(vnetwork_message.receiver().type()));
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    if (!common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.sender().type()) &&
        !common::has<common::xnode_type_t::evm_validator>(vnetwork_message.sender().type())) {
        return true;
    }

    return filter_sender_from_same_validator_group(vnetwork_message, ec) && filter_sender_from_different_validator_group(vnetwork_message, ec);
}

bool xtop_message_filter_recver_is_validator::filter_sender_from_auditor(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_validator>(vnetwork_message.receiver().type()));
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    if (!common::has<common::xnode_type_t::consensus_auditor>(vnetwork_message.sender().type()) &&
        !common::has<common::xnode_type_t::evm_auditor>(vnetwork_message.sender().type())) {
        return true;
    }

    auto const & sender = vnetwork_message.sender();
    auto const & recver = vnetwork_message.receiver();

    auto const sender_auditor_group = m_election_data_accessor->group_element(sender.group_address(), sender.logic_epoch(), ec);
    if (ec) {
        xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", network %" PRIu32 " node %s receives msg sent to %s from %s. ignored. error: %s",
              vnetwork_message.hash(),
              static_cast<std::uint32_t>(m_vhost->network_id().value()),
              m_vhost->account_address().to_string().c_str(),
              recver.to_string().c_str(),
              sender.to_string().c_str(),
              ec.message().c_str());

        return false;
    }

    auto const recver_associated_auditor = m_election_data_accessor->parent_group_element(recver.group_address(), recver.logic_epoch(), ec);
    if (ec) {
        xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", network %" PRIu32 " node %s receives msg sent to %s from %s. ignored. error: %s",
              vnetwork_message.hash(),
              static_cast<std::uint32_t>(m_vhost->network_id().value()),
              m_vhost->account_address().to_string().c_str(),
              recver.to_string().c_str(),
              sender.to_string().c_str(),
              ec.message().c_str());

        return false;
    }

    return filter_sender_from_associated_auditor(vnetwork_message, sender_auditor_group, recver_associated_auditor, ec) &&
           filter_sender_from_non_associated_auditor(vnetwork_message, sender_auditor_group, recver_associated_auditor, ec);
}

bool xtop_message_filter_recver_is_validator::filter_sender_from_same_validator_group(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_validator>(vnetwork_message.receiver().type()));
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.sender().type()) ||
           common::has<common::xnode_type_t::evm_validator>(vnetwork_message.sender().type()));
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    auto const & recver = vnetwork_message.receiver();
    auto const & sender = vnetwork_message.sender();

    if (sender.group_address() != recver.group_address()) {
        return true;
    }

    assert(sender.group_address() == recver.group_address());

    if (recver.logic_epoch().has_value() && sender.logic_epoch() != recver.logic_epoch()) {
        auto const & message = vnetwork_message.message();

        ec = xvnetwork_errc2_t::epoch_mismatch;

        xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", %s receives a message %" PRIx32 " hash %" PRIx64 " from %s to %s but version not match",
              vnetwork_message.hash(),
              m_vhost->account_address().to_string().c_str(),
              static_cast<std::uint32_t>(message.id()),
              message.hash(),
              sender.to_string().c_str(),
              recver.to_string().c_str());

        return false;
    }

    assert(sender.logic_epoch().has_value() || recver.logic_epoch().has_value());

    if (recver.logic_epoch().empty()) {
        normalize_message_recver_by_message_sender(vnetwork_message, m_vhost, m_election_data_accessor, ec);
        if (ec) {
            return false;
        }
    }
    assert(sender.logic_epoch().has_value() && recver.logic_epoch().has_value());

    return false;
}

bool xtop_message_filter_recver_is_validator::filter_sender_from_different_validator_group(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_validator>(vnetwork_message.receiver().type()));
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.sender().type()) ||
           common::has<common::xnode_type_t::evm_validator>(vnetwork_message.sender().type()));
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    auto const & recver = vnetwork_message.receiver();
    auto const & sender = vnetwork_message.sender();

    if (sender.group_address() == recver.group_address()) {
        return true;
    }

    assert(m_election_data_accessor != nullptr);
    auto const sender_associated_auditor = m_election_data_accessor->parent_group_element(sender.group_address(), sender.logic_epoch(), ec);
    if (ec) {
        xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", network %" PRIu32 " node %s receives msg sent to %s from %s. ignored. error: %s",
              vnetwork_message.hash(),
              static_cast<std::uint32_t>(m_vhost->network_id().value()),
              m_vhost->account_address().to_string().c_str(),
              recver.to_string().c_str(),
              sender.to_string().c_str(),
              ec.message().c_str());

        return false;
    }

    auto const recver_associated_auditor = m_election_data_accessor->parent_group_element(recver.group_address(), recver.logic_epoch(), ec);
    if (ec) {
        xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", network %" PRIu32 " node %s receives msg sent to %s from %s. ignored. error: %s",
              vnetwork_message.hash(),
              static_cast<std::uint32_t>(m_vhost->network_id().value()),
              m_vhost->account_address().to_string().c_str(),
              recver.to_string().c_str(),
              sender.to_string().c_str(),
              ec.message().c_str());

        return false;
    }

    return filter_sender_from_different_validator_with_same_associated_auditor_group(vnetwork_message, sender_associated_auditor, recver_associated_auditor, ec) &&
           filter_sender_from_different_validator_without_same_associated_auditor_group(vnetwork_message, sender_associated_auditor, recver_associated_auditor, ec);
}

bool xtop_message_filter_recver_is_validator::filter_sender_from_different_validator_with_same_associated_auditor_group(xvnetwork_message_t & vnetwork_message,
                                                                                                                        std::shared_ptr<election::cache::xgroup_element_t> const & sender_associated_auditor,
                                                                                                                        std::shared_ptr<election::cache::xgroup_element_t> const & recver_associated_auditor,
                                                                                                                        std::error_code & ec) const {
    assert(!ec);
    assert(common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_validator>(vnetwork_message.receiver().type()));
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.sender().type()) ||
           common::has<common::xnode_type_t::evm_validator>(vnetwork_message.sender().type()));
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    auto const & recver = vnetwork_message.receiver();
    auto const & sender = vnetwork_message.sender();

    assert(sender.group_address() != recver.group_address());
    assert(common::has<common::xnode_type_t::consensus_validator>(sender.type()) || common::has<common::xnode_type_t::evm_validator>(sender.type()));

    // for sender from different validator group, we should check to see if sender and recver have same associated auditor group.
    // if they have same associated auditor group, their logic epoch should be the same (follow the logic defined in the election contract).
    // if they don't have same associated auditor group, do nothing.
    if (sender_associated_auditor->address().group_address() != recver_associated_auditor->address().group_address()) {
        return true;
    }

    if (recver.logic_epoch().has_value()) {
        if (sender_associated_auditor->logic_epoch() != recver_associated_auditor->logic_epoch()) {
            ec = xvnetwork_errc2_t::epoch_mismatch;

            xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", network %" PRIu32 " node %s receives msg sent to %s from %s. ignored. error: %s",
                    vnetwork_message.hash(),
                    static_cast<std::uint32_t>(m_vhost->network_id().value()),
                  m_vhost->account_address().to_string().c_str(),
                    recver.to_string().c_str(),
                    sender.to_string().c_str(),
                    ec.message().c_str());

            return false;
        }
        // recver's logic epoch is not empty and passed the verification, no need to be fixed up.

        if (recver.account_address().has_value()) {
            auto const recver_validator_group = m_election_data_accessor->group_element(recver.group_address(), recver.logic_epoch(), ec);
            if (ec) {
                xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", network %" PRIu32 " node %s receives msg sent to %s from %s. ignored. error: %s",
                      vnetwork_message.hash(),
                      static_cast<std::uint32_t>(m_vhost->network_id().value()),
                      m_vhost->account_address().to_string().c_str(),
                      recver.to_string().c_str(),
                      sender.to_string().c_str(),
                      ec.message().c_str());

                return false;
            }

            if (!recver_validator_group->contains(recver.account_address())) {
                ec = election::xdata_accessor_errc_t::account_not_found;

                xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", network %" PRIu32 " node %s receives msg sent to %s from %s. ignored. error: %s",
                      vnetwork_message.hash(),
                      static_cast<std::uint32_t>(m_vhost->network_id().value()),
                      m_vhost->account_address().to_string().c_str(),
                      recver.to_string().c_str(),
                      sender.to_string().c_str(),
                      ec.message().c_str());

                return false;
            }
        }
    } else {
        // if (sender_associated_auditor->logic_epoch() != recver_associated_auditor->logic_epoch()) {
        //     // if sender and receiver have the same associated auditor group but with different logic epoch,
        //     // it means that the associated auditor group deduced by empty recver.logic_epoch() is more recent than the associated auditor group deduced by the validator from sender.
        //     recver_associated_auditor = sender_associated_auditor;
        // }

        auto const recver_validator_group = m_election_data_accessor->group_element_by_height(recver.group_address(), sender_associated_auditor->associated_blk_height(), ec);
        if (ec) {
            xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", network %" PRIu32 " node %s receives msg sent to %s from %s. ignored. error: %s",
                  vnetwork_message.hash(),
                  static_cast<std::uint32_t>(m_vhost->network_id().value()),
                  m_vhost->account_address().to_string().c_str(),
                  recver.to_string().c_str(),
                  sender.to_string().c_str(),
                  ec.message().c_str());

            return false;
        }

        // fix up vnetwork message's recver
        if (recver.account_election_address().empty()) {
            vnetwork_message.receiver(common::xnode_address_t{ recver.group_address(), recver_validator_group->logic_epoch() });
        } else {
            auto const & node_element = recver_validator_group->node_element(recver.account_address(), ec);
            if (ec) {
                xinfo("message_filter: %s recv message %" PRIx32 " hash %" PRIx64 " from %s to %s; dropped due to %s",
                      m_vhost->account_address().to_string().c_str(),
                      static_cast<uint32_t>(vnetwork_message.message_id()),
                      static_cast<uint64_t>(vnetwork_message.hash()),
                      vnetwork_message.sender().to_string().c_str(),
                      vnetwork_message.receiver().to_string().c_str(),
                      ec.message().c_str());

                return false;
            }

            assert(node_element != nullptr);

            if (broadcast(recver.account_election_address().slot_id())) {
                vnetwork_message.receiver(common::xnode_address_t{ recver.group_address(), common::xaccount_election_address_t{ recver.account_address(), node_element->slot_id() }, recver_validator_group->logic_epoch() });
            } else {
                if (recver.slot_id() != node_element->slot_id()) {
                    ec = top::vnetwork::xvnetwork_errc2_t::slot_id_mismatch;

                    xinfo("message_filter: %s recv message %" PRIx32 " hash %" PRIx64 " from %s to %s; dropped due to %s; local slot id %" PRIu16 " msg slot id %" PRIu16,
                          m_vhost->account_address().to_string().c_str(),
                          static_cast<uint32_t>(vnetwork_message.message_id()),
                          static_cast<uint64_t>(vnetwork_message.hash()),
                          vnetwork_message.sender().to_string().c_str(),
                          vnetwork_message.receiver().to_string().c_str(),
                          ec.message().c_str(),
                          static_cast<uint16_t>(node_element->slot_id()),
                          static_cast<uint16_t>(recver.slot_id()));

                    return false;
                }
                vnetwork_message.receiver(common::xnode_address_t{ recver.group_address(), recver.account_election_address(), recver_validator_group->logic_epoch() });
            }
        }
    }

    return false;
}

bool xtop_message_filter_recver_is_validator::filter_sender_from_different_validator_without_same_associated_auditor_group(xvnetwork_message_t & vnetwork_message,
                                                                                                                           std::shared_ptr<election::cache::xgroup_element_t> const & sender_associated_auditor,
                                                                                                                           std::shared_ptr<election::cache::xgroup_element_t> const & recver_associated_auditor,
                                                                                                                           std::error_code & ec) const {
    assert(!ec);
    assert(common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_validator>(vnetwork_message.receiver().type()));
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.sender().type()) ||
           common::has<common::xnode_type_t::evm_validator>(vnetwork_message.sender().type()));
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    auto const & recver = vnetwork_message.receiver();
#if !defined(NDEBUG)
    auto const & sender = vnetwork_message.sender();

    assert(sender.group_address() != recver.group_address());
    assert(common::has<common::xnode_type_t::consensus_validator>(sender.type()) || common::has<common::xnode_type_t::evm_validator>(sender.type()));
#endif

    // for sender from different validator group, we should check to see if sender and recver have same associated auditor group.
    // if they have same associated auditor group, their logic epoch should be the same (follow the logic defined in the election contract).
    // if they don't have same associated auditor group, do nothing.
    if (sender_associated_auditor->address().group_address() == recver_associated_auditor->address().group_address()) {
        return true;
    }

    if (recver.logic_epoch().empty()) {
        normalize_message_recver(vnetwork_message, m_vhost, m_election_data_accessor, ec);
        if (ec) {
            return false;
        }
    }

    return false;
}

bool xtop_message_filter_recver_is_validator::filter_sender_from_associated_auditor(xvnetwork_message_t & vnetwork_message,
                                                                                    std::shared_ptr<election::cache::xgroup_element_t> const & sender_auditor,
                                                                                    std::shared_ptr<election::cache::xgroup_element_t> const & recver_associated_auditor,
                                                                                    std::error_code & ec) const {
    assert(!ec);
    assert(common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_validator>(vnetwork_message.receiver().type()));
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(common::has<common::xnode_type_t::consensus_auditor>(vnetwork_message.sender().type()) ||
           common::has<common::xnode_type_t::evm_auditor>(vnetwork_message.sender().type()));
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    auto const & sender = vnetwork_message.sender();
    auto const & recver = vnetwork_message.receiver();

    assert(common::has<common::xnode_type_t::consensus_auditor>(sender.type()) || common::has<common::xnode_type_t::evm_auditor>(sender.type()));
    if (sender_auditor->address().group_address() != recver_associated_auditor->address().group_address()) {
        // not from associated auditor group.
        return true;
    }

    if (recver.logic_epoch().has_value()) {
        assert(sender_auditor->address().group_address() == recver_associated_auditor->address().group_address());

        if (sender_auditor->logic_epoch() != recver_associated_auditor->logic_epoch()) {
            ec = xvnetwork_errc2_t::epoch_mismatch;
            xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", network %" PRIu32 " node %s receives msg sent to %s from %s. ignored. error: %s",
                  vnetwork_message.hash(),
                  static_cast<std::uint32_t>(m_vhost->network_id().value()),
                  m_vhost->account_address().to_string().c_str(),
                  recver.to_string().c_str(),
                  sender.to_string().c_str(),
                  ec.message().c_str());

            return false;
        }

        if (recver.account_address().has_value()) {
            auto const & validator_group = m_election_data_accessor->group_element_by_logic_time(recver.group_address(), vnetwork_message.logic_time(), ec);
            if (ec) {
                xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", network %" PRIu32 " node %s receives msg sent to %s from %s. ignored. error: %s",
                        vnetwork_message.hash(),
                        static_cast<std::uint32_t>(m_vhost->network_id().value()),
                      m_vhost->account_address().to_string().c_str(),
                        recver.to_string().c_str(),
                        sender.to_string().c_str(),
                        ec.message().c_str());

                return false;
            }

            if (!validator_group->contains(recver.account_address())) {
                ec = election::xdata_accessor_errc_t::account_not_found;
                xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", network %" PRIu32 " node %s receives msg sent to %s from %s. ignored. error: %s",
                      vnetwork_message.hash(),
                      static_cast<std::uint32_t>(m_vhost->network_id().value()),
                      m_vhost->account_address().to_string().c_str(),
                      recver.to_string().c_str(),
                      sender.to_string().c_str(),
                      ec.message().c_str());

                return false;
            }
        }
    } else {
        auto const validator_group = m_election_data_accessor->group_element_by_height(recver.group_address(), sender_auditor->associated_blk_height(), ec);
        if (ec) {
            xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", network %" PRIu32 " node %s receives msg sent to %s from %s. ignored. error: %s",
                  vnetwork_message.hash(),
                  static_cast<std::uint32_t>(m_vhost->network_id().value()),
                  m_vhost->account_address().to_string().c_str(),
                  recver.to_string().c_str(),
                  sender.to_string().c_str(),
                  ec.message().c_str());

            return false;
        }

        if (recver.account_election_address().empty()) {
            vnetwork_message.receiver(common::xnode_address_t{ recver.group_address(), validator_group->logic_epoch() });
        } else {
            auto const & node_element = validator_group->node_element(recver.account_address(), ec);
            if (ec) {
                xinfo("message_filter: %s recv message %" PRIx32 " hash %" PRIx64 " from %s to %s; dropped due to %s",
                      m_vhost->account_address().to_string().c_str(),
                      static_cast<uint32_t>(vnetwork_message.message_id()),
                      static_cast<uint64_t>(vnetwork_message.hash()),
                      vnetwork_message.sender().to_string().c_str(),
                      vnetwork_message.receiver().to_string().c_str(),
                      ec.message().c_str());

                return false;
            }

            assert(node_element != nullptr);

            if (broadcast(recver.account_election_address().slot_id())) {
                vnetwork_message.receiver(common::xnode_address_t{ recver.group_address(), common::xaccount_election_address_t{ recver.account_address(), node_element->slot_id() }, validator_group->logic_epoch() });
            } else {
                if (recver.slot_id() != node_element->slot_id()) {
                    ec = top::vnetwork::xvnetwork_errc2_t::slot_id_mismatch;

                    xinfo("message_filter: %s recv message %" PRIx32 " hash %" PRIx64 " from %s to %s; dropped due to %s; local slot id %" PRIu16 " msg slot id %" PRIu16,
                          m_vhost->account_address().to_string().c_str(),
                          static_cast<uint32_t>(vnetwork_message.message_id()),
                          static_cast<uint64_t>(vnetwork_message.hash()),
                          vnetwork_message.sender().to_string().c_str(),
                          vnetwork_message.receiver().to_string().c_str(),
                          ec.message().c_str(),
                          static_cast<uint16_t>(node_element->slot_id()),
                          static_cast<uint16_t>(recver.slot_id()));

                    return false;
                }
                vnetwork_message.receiver(common::xnode_address_t{ recver.group_address(), recver.account_election_address(), validator_group->logic_epoch() });
            }
        }
    }

    return false;
}

bool xtop_message_filter_recver_is_validator::filter_sender_from_non_associated_auditor(xvnetwork_message_t & vnetwork_message,
                                                                                        std::shared_ptr<election::cache::xgroup_element_t> const & sender_auditor,
                                                                                        std::shared_ptr<election::cache::xgroup_element_t> const & recver_associated_auditor,
                                                                                        std::error_code & ec) const {
    assert(!ec);
    assert(common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_validator>(vnetwork_message.receiver().type()));
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(common::has<common::xnode_type_t::consensus_auditor>(vnetwork_message.sender().type()) ||
           common::has<common::xnode_type_t::evm_auditor>(vnetwork_message.sender().type()));
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    auto const & recver = vnetwork_message.receiver();

#if !defined(NDEBUG)
    auto const & sender = vnetwork_message.sender();
    assert(common::has<common::xnode_type_t::consensus_auditor>(sender.type()) || common::has<common::xnode_type_t::evm_auditor>(sender.type()));
#endif
    if (sender_auditor->address().group_address() == recver_associated_auditor->address().group_address()) {
        return true;
    }

    // fix receiver if necessary.
    if (recver.logic_epoch().empty()) {
        normalize_message_recver(vnetwork_message, m_vhost, m_election_data_accessor, ec);
        if (ec) {
            return false;
        }
    }

    return false;
}

xtop_message_filter_recver_is_auditor::xtop_message_filter_recver_is_auditor(observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                                                                             observer_ptr< election::cache::xdata_accessor_face_t> const & election_data_accessor) noexcept
    : m_vhost{ vhost }, m_election_data_accessor{ election_data_accessor } {
}

xfilter_result_t xtop_message_filter_recver_is_auditor::filter(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    if (!common::has<common::xnode_type_t::consensus_auditor>(vnetwork_message.receiver().type()) &&
        !common::has<common::xnode_type_t::evm_auditor>(vnetwork_message.receiver().type())) {
        return xfilter_result_t::continue_filtering;
    }

    if (filter_sender_from_nonconsensus_group(vnetwork_message, ec) && filter_sender_from_consensus_group(vnetwork_message, ec)) {
        return xfilter_result_t ::continue_filtering;
    }

    return xfilter_result_t::stop_filtering;
}

bool xtop_message_filter_recver_is_auditor::filter_sender_from_nonconsensus_group(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(common::has<common::xnode_type_t::consensus_auditor>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_auditor>(vnetwork_message.receiver().type()));
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    auto const & sender = vnetwork_message.sender();
    if (!common::has<common::xnode_type_t::edge>(sender.type()) && !common::has<common::xnode_type_t::storage>(sender.type()) &&
        !common::has<common::xnode_type_t::fullnode>(sender.type())) {
        return true;
    }

    return filter_sender_from_edge(vnetwork_message, ec) && filter_sender_from_storage(vnetwork_message, ec) && filter_sender_from_fullnode(vnetwork_message, ec);
}

bool xtop_message_filter_recver_is_auditor::filter_sender_from_consensus_group(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(common::has<common::xnode_type_t::consensus_auditor>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_auditor>(vnetwork_message.receiver().type()));
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    auto const & sender = vnetwork_message.sender();
    if (!common::has<common::xnode_type_t::consensus>(sender.type()) &&
        !common::has<common::xnode_type_t::rec>(sender.type())       &&
        !common::has<common::xnode_type_t::zec>(sender.type())) {
        return true;
    }

    return filter_sender_from_rec(vnetwork_message, ec)     &&
           filter_sender_from_zec(vnetwork_message, ec)     &&
           filter_sender_from_auditor(vnetwork_message, ec) &&
           filter_sender_from_validator(vnetwork_message, ec);
}

bool xtop_message_filter_recver_is_auditor::filter_sender_from_edge(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(common::has<common::xnode_type_t::consensus_auditor>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_auditor>(vnetwork_message.receiver().type()));
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    auto const & sender = vnetwork_message.sender();
    if (!common::has<common::xnode_type_t::edge>(sender.type())) {
        return true;
    }

    auto const & recver = vnetwork_message.receiver();

    // fix receiver if necessary.
    if (recver.logic_epoch().empty()) {
        normalize_message_recver(vnetwork_message, m_vhost, m_election_data_accessor, ec);
        if (ec) {
            return false;
        }
    }

    return false;
}

bool xtop_message_filter_recver_is_auditor::filter_sender_from_fullnode(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(common::has<common::xnode_type_t::consensus_auditor>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_auditor>(vnetwork_message.receiver().type()));
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    auto const & sender = vnetwork_message.sender();
    if (!common::has<common::xnode_type_t::fullnode>(sender.type())) {
        return true;
    }

    auto const & recver = vnetwork_message.receiver();
    // fix receiver if necessary.
    if (recver.logic_epoch().empty()) {
        normalize_message_recver(vnetwork_message, m_vhost, m_election_data_accessor, ec);
    }

    return false;
}

bool xtop_message_filter_recver_is_auditor::filter_sender_from_storage(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(common::has<common::xnode_type_t::consensus_auditor>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_auditor>(vnetwork_message.receiver().type()));
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    if (!common::has<common::xnode_type_t::storage>(vnetwork_message.sender().type())) {
        return true;
    }

    auto const & recver = vnetwork_message.receiver();

    // fix receiver if necessary.
    if (recver.logic_epoch().empty()) {
        normalize_message_recver(vnetwork_message, m_vhost, m_election_data_accessor, ec);
        if (ec) {
            return false;
        }
    }

    return false;
}

bool xtop_message_filter_recver_is_auditor::filter_sender_from_rec(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(common::has<common::xnode_type_t::consensus_auditor>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_auditor>(vnetwork_message.receiver().type()));
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    if (!common::has<common::xnode_type_t::rec>(vnetwork_message.sender().type())) {
        return true;
    }

    auto const & recver = vnetwork_message.receiver();

    // fix receiver if necessary.
    if (recver.logic_epoch().empty()) {
        normalize_message_recver(vnetwork_message, m_vhost, m_election_data_accessor, ec);
        if (ec) {
            return false;
        }
    }

    return false;
}

bool xtop_message_filter_recver_is_auditor::filter_sender_from_zec(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(common::has<common::xnode_type_t::consensus_auditor>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_auditor>(vnetwork_message.receiver().type()));
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    if (!common::has<common::xnode_type_t::rec>(vnetwork_message.sender().type())) {
        return true;
    }

    auto const & recver = vnetwork_message.receiver();

    // fix receiver if necessary.
    if (recver.logic_epoch().empty()) {
        normalize_message_recver(vnetwork_message, m_vhost, m_election_data_accessor, ec);
        if (ec) {
            return false;
        }
    }

    return false;
}

bool xtop_message_filter_recver_is_auditor::filter_sender_from_auditor(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(common::has<common::xnode_type_t::consensus_auditor>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_auditor>(vnetwork_message.receiver().type()));
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    if (!common::has<common::xnode_type_t::consensus_auditor>(vnetwork_message.sender().type()) &&
        !common::has<common::xnode_type_t::evm_auditor>(vnetwork_message.sender().type())) {
        return true;
    }

    return filter_sender_from_same_auditor_group(vnetwork_message, ec) && filter_sender_from_different_auditor_group(vnetwork_message, ec);
}

bool xtop_message_filter_recver_is_auditor::filter_sender_from_validator(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(common::has<common::xnode_type_t::consensus_auditor>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_auditor>(vnetwork_message.receiver().type()));
    assert(vnetwork_message.sender().logic_epoch().has_value());

    if (!common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.sender().type()) &&
        !common::has<common::xnode_type_t::evm_validator>(vnetwork_message.sender().type())) {
        return true;
    }

    auto const & recver = vnetwork_message.receiver();
    auto const & sender = vnetwork_message.sender();

    auto const sender_associated_group = m_election_data_accessor->parent_group_element(sender.group_address(), sender.logic_epoch(), ec);
    if (ec) {
        xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", network %" PRIu32 " node %s receives msg sent to %s from %s. ignored. error: %s",
              vnetwork_message.hash(),
              static_cast<std::uint32_t>(m_vhost->network_id().value()),
              m_vhost->account_address().to_string().c_str(),
              recver.to_string().c_str(),
              sender.to_string().c_str(),
              ec.message().c_str());

        return false;
    }

    std::shared_ptr<election::cache::xgroup_element_t> auditor_group;
    if (recver.logic_epoch().empty()) {
        auditor_group = m_election_data_accessor->group_element_by_logic_time(recver.group_address(), vnetwork_message.logic_time(), ec);
    } else {
        auditor_group = m_election_data_accessor->group_element(recver.group_address(), recver.logic_epoch(), ec);
    }

    if (ec) {
        xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", network %" PRIu32 " node %s receives msg sent to %s from %s. ignored. error: %s",
              vnetwork_message.hash(),
              static_cast<std::uint32_t>(m_vhost->network_id()),
              m_vhost->account_address().to_string().c_str(),
              recver.to_string().c_str(),
              sender.to_string().c_str(),
              ec.message().c_str());

        return false;
    }

    return filter_sender_from_associated_validator(vnetwork_message, sender_associated_group, auditor_group, ec) &&
           filter_sender_from_non_associated_validator(vnetwork_message, sender_associated_group, auditor_group, ec);
}

bool xtop_message_filter_recver_is_auditor::filter_sender_from_same_auditor_group(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(common::has<common::xnode_type_t::consensus_auditor>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_auditor>(vnetwork_message.receiver().type()));
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(common::has<common::xnode_type_t::consensus_auditor>(vnetwork_message.sender().type()) ||
           common::has<common::xnode_type_t::evm_auditor>(vnetwork_message.sender().type()));
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    auto const & recver = vnetwork_message.receiver();
    auto const & sender = vnetwork_message.sender();

    if (sender.group_address() != recver.group_address()) {
        return true;
    }

    assert(sender.group_address() == recver.group_address());

    if (recver.logic_epoch().has_value() && sender.logic_epoch() != recver.logic_epoch()) {
        auto const & message = vnetwork_message.message();

        ec = xvnetwork_errc2_t::epoch_mismatch;

        xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", %s receives a message %" PRIx32 " hash %" PRIx64 " from %s to %s but version not match",
                vnetwork_message.hash(),
                m_vhost->account_address().to_string().c_str(),
                static_cast<std::uint32_t>(message.id()),
                message.hash(),
                sender.to_string().c_str(),
                recver.to_string().c_str());

        return false;
    }
    assert(sender.logic_epoch().has_value() || recver.logic_epoch().has_value());

    if (recver.logic_epoch().empty()) {
        normalize_message_recver_by_message_sender(vnetwork_message, m_vhost, m_election_data_accessor, ec);
        if (ec) {
            return false;
        }
    }
    assert(sender.logic_epoch().has_value() && recver.logic_epoch().has_value());

    return false;
}

bool xtop_message_filter_recver_is_auditor::filter_sender_from_different_auditor_group(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(common::has<common::xnode_type_t::consensus_auditor>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_auditor>(vnetwork_message.receiver().type()));
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(common::has<common::xnode_type_t::consensus_auditor>(vnetwork_message.sender().type()) ||
           common::has<common::xnode_type_t::evm_auditor>(vnetwork_message.sender().type()));
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    auto const & recver = vnetwork_message.receiver();
    auto const & sender = vnetwork_message.sender();

    if (sender.group_address() == recver.group_address()) {
        return true;
    }

    assert(sender.group_address() != recver.group_address());

    // fix receiver if necessary.
    if (recver.logic_epoch().empty()) {
        normalize_message_recver(vnetwork_message, m_vhost, m_election_data_accessor, ec);
        if (ec) {
            return false;
        }
    }

    return false;
}

bool xtop_message_filter_recver_is_auditor::filter_sender_from_associated_validator(xvnetwork_message_t & vnetwork_message,
                                                                                    std::shared_ptr<election::cache::xgroup_element_t> const & sender_associated_auditor,
                                                                                    std::shared_ptr<election::cache::xgroup_element_t> const & recver_auditor,
                                                                                    std::error_code & ec) const {
    assert(!ec);
    assert(common::has<common::xnode_type_t::consensus_auditor>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_auditor>(vnetwork_message.receiver().type()));
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.sender().type()) ||
           common::has<common::xnode_type_t::evm_validator>(vnetwork_message.sender().type()));
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    auto const & sender = vnetwork_message.sender();
    auto const & recver = vnetwork_message.receiver();

    assert(common::has<common::xnode_type_t::consensus_validator>(sender.type()) || common::has<common::xnode_type_t::evm_validator>(sender.type()));
//#if defined(DEBUG)
//    auto const & sender_associated_group_address = sender_associated_group->address().group_address();
//    auto const & auditor_group_address = auditor_group->address().group_address();
//    auto const sender_associated_group_id = sender_associated_group_address.group_id();
//    auto const auditor_group_id = auditor_group_address.group_id();
//#endif

    if (sender_associated_auditor->address().group_address() != recver_auditor->address().group_address()) {
        // not from associated auditor group.
        return true;
    }

    assert(sender_associated_auditor->address().group_address() == recver_auditor->address().group_address());
    if (recver.logic_epoch().has_value()) {
        if (sender_associated_auditor->logic_epoch() != recver_auditor->logic_epoch()) {
            ec = xvnetwork_errc2_t::epoch_mismatch;

            xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", network %" PRIu32 " node %s receives msg sent to %s from %s. ignored. error: %s",
                  vnetwork_message.hash(),
                  static_cast<std::uint32_t>(m_vhost->network_id().value()),
                  m_vhost->account_address().to_string().c_str(),
                  recver.to_string().c_str(),
                  sender.to_string().c_str(),
                  ec.message().c_str());

            return false;
        }

        if (!recver.account_address().empty() && !recver_auditor->contains(recver.account_address())) {
            ec = top::election::xdata_accessor_errc_t::node_not_found;

            xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", network %" PRIu32 " node %s receives msg sent to %s from %s. ignored. error: %s",
                  vnetwork_message.hash(),
                  static_cast<std::uint32_t>(m_vhost->network_id().value()),
                  m_vhost->account_address().to_string().c_str(),
                  recver.to_string().c_str(),
                  sender.to_string().c_str(),
                  ec.message().c_str());

            return false;
        }
    } else {
        normalize_message_recver(vnetwork_message, m_vhost, sender_associated_auditor, ec);
        if (ec) {
            return false;
        }
    }

    return false;
}

bool xtop_message_filter_recver_is_auditor::filter_sender_from_non_associated_validator(xvnetwork_message_t & vnetwork_message,
                                                                                        std::shared_ptr<election::cache::xgroup_element_t> const & sender_associated_auditor,
                                                                                        std::shared_ptr<election::cache::xgroup_element_t> const & recver_auditor,
                                                                                        std::error_code & ec) const {
    assert(!ec);
    assert(common::has<common::xnode_type_t::consensus_auditor>(vnetwork_message.receiver().type()) ||
           common::has<common::xnode_type_t::evm_auditor>(vnetwork_message.receiver().type()));
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.sender().type()) ||
           common::has<common::xnode_type_t::evm_validator>(vnetwork_message.sender().type()));
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    auto const & sender = vnetwork_message.sender();
    auto const & recver = vnetwork_message.receiver();

    assert(common::has<common::xnode_type_t::consensus_validator>(sender.type()) || common::has<common::xnode_type_t::evm_validator>(sender.type()));

    if (sender_associated_auditor->address().group_address() == recver_auditor->address().group_address()) {
        return true;
    }

    assert(sender_associated_auditor->address().group_address() != recver_auditor->address().group_address());
    if (recver.account_address().has_value()) {
        if (!recver_auditor->contains(recver.account_address())) {
            ec = top::election::xdata_accessor_errc_t::account_not_found;

            xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", network %" PRIu32 " node %s receives msg sent to %s from %s. ignored. error: %s",
                  vnetwork_message.hash(),
                  static_cast<std::uint32_t>(m_vhost->network_id().value()),
                  m_vhost->account_address().to_string().c_str(),
                  recver.to_string().c_str(),
                  sender.to_string().c_str(),
                  ec.message().c_str());

            return false;
        }
    }

    if (recver.logic_epoch().empty()) {
        normalize_message_recver(vnetwork_message, m_vhost, m_election_data_accessor, ec);
        if (ec) {
            return false;
        }
    }

    return false;
}

xtop_message_filter_recver_is_rec::xtop_message_filter_recver_is_rec(observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                                                                     observer_ptr< election::cache::xdata_accessor_face_t> const & election_data_accessor) noexcept
    : m_vhost{ vhost }, m_election_data_accessor{ election_data_accessor } {
}

xfilter_result_t xtop_message_filter_recver_is_rec::filter(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    if (!common::has<common::xnode_type_t::rec>(vnetwork_message.receiver().type())) {
        return xfilter_result_t::continue_filtering;
    }

    if (filter_sender_from_rec(vnetwork_message, ec) && filter_sender_from_non_rec(vnetwork_message, ec)) {
        return xfilter_result_t::continue_filtering;
    }

    return xfilter_result_t::stop_filtering;
}

bool xtop_message_filter_recver_is_rec::filter_sender_from_rec(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(common::has<common::xnode_type_t::rec>(vnetwork_message.receiver().type()));
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    if (!common::has<common::xnode_type_t::rec>(vnetwork_message.sender().type())) {
        return true;
    }

    auto const & recver = vnetwork_message.receiver();
    // auto const & sender = vnetwork_message.sender();
    if (recver.logic_epoch().empty()) {
        normalize_message_recver_by_message_sender(vnetwork_message, m_vhost, m_election_data_accessor, ec);
        if (ec) {
            return false;
        }
    }

    return false;
}

bool xtop_message_filter_recver_is_rec::filter_sender_from_non_rec(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(common::has<common::xnode_type_t::rec>(vnetwork_message.receiver().type()));
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    if (!common::has<common::xnode_type_t::rec>(vnetwork_message.sender().type())) {
        return true;
    }
    auto const & recver = vnetwork_message.receiver();

    // fix receiver if necessary.
    if (recver.logic_epoch().empty()) {
        normalize_message_recver(vnetwork_message, m_vhost, m_election_data_accessor, ec);
        if (ec) {
            return false;
        }
    }

    return false;
}

xtop_message_filter_recver_is_zec::xtop_message_filter_recver_is_zec(observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                                                                     observer_ptr< election::cache::xdata_accessor_face_t> const & election_data_accessor) noexcept
    : m_vhost{ vhost }, m_election_data_accessor{ election_data_accessor } {
}

xfilter_result_t xtop_message_filter_recver_is_zec::filter(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    if (!common::has<common::xnode_type_t::zec>(vnetwork_message.receiver().type())) {
        return xfilter_result_t::continue_filtering;
    }

    if (filter_sender_from_zec(vnetwork_message, ec) && filter_sender_from_non_zec(vnetwork_message, ec)) {
        return xfilter_result_t ::continue_filtering;
    }

    return xfilter_result_t::stop_filtering;
}

bool xtop_message_filter_recver_is_zec::filter_sender_from_zec(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(common::has<common::xnode_type_t::zec>(vnetwork_message.receiver().type()));
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    if (!common::has<common::xnode_type_t::zec>(vnetwork_message.sender().type())) {
        return true;
    }

    auto const & recver = vnetwork_message.receiver();
    // auto const & sender = vnetwork_message.sender();
    if (recver.logic_epoch().empty()) {
        normalize_message_recver_by_message_sender(vnetwork_message, m_vhost, m_election_data_accessor, ec);
        if (ec) {
            return false;
        }
    }

    return false;
}

bool xtop_message_filter_recver_is_zec::filter_sender_from_non_zec(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const {
    assert(!ec);
    assert(vnetwork_message.sender().logic_epoch().has_value());
    assert(common::has<common::xnode_type_t::zec>(vnetwork_message.receiver().type()));
    assert(!broadcast(vnetwork_message.receiver().network_id()));
    assert(!broadcast(vnetwork_message.receiver().zone_id()));
    assert(!broadcast(vnetwork_message.receiver().cluster_id()));
    assert(!broadcast(vnetwork_message.receiver().group_id()));

    if (!common::has<common::xnode_type_t::zec>(vnetwork_message.sender().type())) {
        return true;
    }
    auto const & recver = vnetwork_message.receiver();

    // fix receiver if necessary.
    if (recver.logic_epoch().empty()) {
        normalize_message_recver(vnetwork_message, m_vhost, m_election_data_accessor, ec);
        if (ec) {
            return false;
        }
    }

    return false;
}

NS_END2
