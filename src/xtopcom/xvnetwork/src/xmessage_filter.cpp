// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvnetwork/xmessage_filter.h"

#include "xelection/xcache/xgroup_element.h"
#include "xelection/xdata_accessor_error.h"
#include "xmetrics/xmetrics.h"
#include "xvm/manager/xmessage_ids.h"
#include "xvnetwork/xmessage_filter_manager.h"
#include "xvnetwork/xvhost_face.h"
#include "xvnetwork/xvnetwork_error2.h"

#include <cinttypes>

NS_BEG2(top, vnetwork)

xmsg_filter_message_empty::xmsg_filter_message_empty(xmessage_filter_manager_t * filter_mgr_ptr) : m_filter_mgr_ptr{filter_mgr_ptr} {}
xmsg_filter_wrong_dst::xmsg_filter_wrong_dst(xmessage_filter_manager_t * filter_mgr_ptr) : m_filter_mgr_ptr{filter_mgr_ptr} {}
xmsg_filter_local_time::xmsg_filter_local_time(xmessage_filter_manager_t * filter_mgr_ptr) : m_filter_mgr_ptr{filter_mgr_ptr} {}
xmsg_filter_validator_neighbors_version_mismatch::xmsg_filter_validator_neighbors_version_mismatch(xmessage_filter_manager_t * filter_mgr_ptr) : m_filter_mgr_ptr{filter_mgr_ptr} {}
xmsg_filter_validator_from_auditor::xmsg_filter_validator_from_auditor(xmessage_filter_manager_t * filter_mgr_ptr) : m_filter_mgr_ptr{filter_mgr_ptr} {}
xmsg_filter_validator_from_archive::xmsg_filter_validator_from_archive(xmessage_filter_manager_t * filter_mgr_ptr) : m_filter_mgr_ptr{filter_mgr_ptr} {}
xmsg_filter_auditor_from_validator::xmsg_filter_auditor_from_validator(xmessage_filter_manager_t * filter_mgr_ptr) : m_filter_mgr_ptr{filter_mgr_ptr} {}
xmsg_filter_version_still_empty::xmsg_filter_version_still_empty(xmessage_filter_manager_t * filter_mgr_ptr) : m_filter_mgr_ptr{filter_mgr_ptr} {}

void xmsg_filter_message_empty::filt(xvnetwork_message_t & vnetwork_message) {
    xvnetwork_message_t empty_message{};
    if (vnetwork_message.empty()) {
        #if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_received_invalid", 1);
        #endif
        xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", vnetwork message empty", vnetwork_message.hash());
        vnetwork_message = empty_message;
        return;
    }

    auto & message = vnetwork_message.message();
    auto & receiver = vnetwork_message.receiver();
    auto & sender = vnetwork_message.sender();
    #if VHOST_METRICS
    XMETRICS_COUNTER_INCREMENT(
        "vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_in_vhost" + std::to_string(static_cast<std::uint32_t>(message.id())),
        1);
    #endif

    xdbg("[vnetwork] recv message :%" PRIx32 " (hash %" PRIx64 " logic time %" PRIu64 ") from:%s to:%s",
         static_cast<std::uint32_t>(message.id()),
         message.hash(),
         vnetwork_message.logic_time(),
         sender.to_string().c_str(),
         receiver.to_string().c_str());

    xdbg("[vnetwork] %s receives message %" PRIx64 " from %s msg id %" PRIx32 " logic time %" PRIu64,
         m_filter_mgr_ptr->get_vhost_ptr()->host_node_id().to_string().c_str(),
         vnetwork_message.hash(),
         sender.to_string().c_str(),
         static_cast<std::uint32_t>(vnetwork_message.message().id()),
         vnetwork_message.logic_time());

    if (vnetwork_message.empty()) {
        #if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_received_invalid", 1);
        #endif
        // assert(false);
        xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", msg id %" PRIx32 " receiving an empty message", vnetwork_message.hash(), static_cast<std::uint32_t>(message.id()));
        vnetwork_message = empty_message;
    }
}

void xmsg_filter_wrong_dst::filt(xvnetwork_message_t & vnetwork_message) {
    xvnetwork_message_t empty_message{};

    auto & message = vnetwork_message.message();
    auto & receiver = vnetwork_message.receiver();

    if (!common::broadcast(receiver.network_id()) && receiver.network_id() != m_filter_mgr_ptr->get_vhost_ptr()->network_id()) {
        #if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_received_invalid", 1);
        #endif
        xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", vnetwork message network id not matched: this network id %" PRIu32 "; sent to %" PRIu32 " ",
              vnetwork_message.hash(),
              static_cast<std::uint32_t>(m_filter_mgr_ptr->get_vhost_ptr()->network_id().value()),
              static_cast<std::uint32_t>(receiver.network_id().value()));
        vnetwork_message = empty_message;
    }
}

void xmsg_filter_local_time::filt(xvnetwork_message_t & vnetwork_message) {
    xvnetwork_message_t empty_message{};

    auto const msg_time = vnetwork_message.logic_time();
    xdbg("[vnetwork] message logic time %" PRIu64, msg_time);
    auto const local_time = m_filter_mgr_ptr->get_vhost_ptr()->last_logic_time();

    // the logic time is used to calc the version.  thus, if version not specified,
    // logic time will be used to find the version, which means that the time
    // should be verified!
    if (vnetwork_message.receiver().version().empty()) {
        constexpr std::uint64_t future_threshold{2};
        constexpr std::uint64_t past_threshold{6};

        if ((local_time != 0) && (local_time + future_threshold < msg_time) && vnetwork_message.message().id() != top::contract::xmessage_block_broadcast_id) {
            // receive a message from future, ignore
            xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", receive a message whose logic time is %" PRIu64 " which is much more newer than current node %" PRIu64 " ",
                  vnetwork_message.hash(),
                  msg_time,
                  local_time);
        #if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_discard_validation_failure", 1);
        #endif  
            // assert(false);
            vnetwork_message = empty_message;
            return;
        }

        if ((msg_time != 0) && (msg_time + past_threshold < local_time) && vnetwork_message.message().id() != top::contract::xmessage_block_broadcast_id) {
            // receive a message from past, ignore
            xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", receive a message whose logic time is %" PRIu64 " which is much more older than current node %" PRIu64 " ",
                  vnetwork_message.hash(),
                  msg_time,
                  local_time);
        #if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_discard_validation_failure", 1);
        #endif
            // assert(false);
            vnetwork_message = empty_message;
        }
    }
}

void xmsg_filter_validator_neighbors_version_mismatch::filt(xvnetwork_message_t & vnetwork_message) {
    if (!common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.receiver().type()) ||
        vnetwork_message.sender().cluster_address() != vnetwork_message.receiver().cluster_address()) {
        return;
    }

    auto & message = vnetwork_message.message();
    auto & receiver = vnetwork_message.receiver();
    auto & sender = vnetwork_message.sender();
    xvnetwork_message_t empty_message{};

    if (sender.version().has_value() && receiver.version().has_value() && sender.version() != receiver.version()) {
        #if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_discard_validation_failure", 1);
        #endif
        xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", %s receives a message %" PRIx32 " hash %" PRIx64 " from %s to %s but version not match",
              vnetwork_message.hash(),
              m_filter_mgr_ptr->get_vhost_ptr()->host_node_id().to_string().c_str(),
              static_cast<std::uint32_t>(message.id()),
              message.hash(),
              sender.to_string().c_str(),
              receiver.to_string().c_str());
        vnetwork_message = empty_message;
        return;
    }
    assert(sender.version().has_value() || receiver.version().has_value());

    if (sender.version().empty()) {
        #if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_discard_validation_failure", 1);
        #endif

        xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", %s receives a message %" PRIx32 " hash %" PRIx64 " from %s to %s, but sender doesn't provide round version",
              vnetwork_message.hash(),
              m_filter_mgr_ptr->get_vhost_ptr()->host_node_id().to_string().c_str(),
              static_cast<std::uint32_t>(message.id()),
              message.hash(),
              sender.to_string().c_str(),
              receiver.to_string().c_str());
        vnetwork_message = empty_message;
        return;
    }

    if (receiver.version().empty()) {
        if (receiver.account_election_address().empty()) {
            vnetwork_message.receiver(common::xnode_address_t{receiver.sharding_address(), sender.version(), receiver.sharding_size(), receiver.associated_blk_height()});
        } else {
            vnetwork_message.receiver(common::xnode_address_t{
                receiver.sharding_address(), receiver.account_election_address(), sender.version(), receiver.sharding_size(), receiver.associated_blk_height()});
        }
    }
    assert(sender.version().has_value() && receiver.version().has_value());
}

void xmsg_filter_validator_from_auditor::filt(xvnetwork_message_t & vnetwork_message) {
    if (!common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.receiver().type()) ||
        vnetwork_message.sender().cluster_address() == vnetwork_message.receiver().cluster_address()) {
        return;
    }

    if (common::has<common::xnode_type_t::storage>(vnetwork_message.sender().type())) {
        return;
    }

    auto & message = vnetwork_message.message();
    auto & receiver = vnetwork_message.receiver();
    auto & sender = vnetwork_message.sender();
    auto const & src_type = sender.type();
    auto const msg_time = vnetwork_message.logic_time();
    xvnetwork_message_t empty_message{};

    if (!common::has<common::xnode_type_t::consensus_auditor>(src_type) && !common::has<common::xnode_type_t::storage>(src_type)) {
        #if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_discard_validation_failure", 1);
        #endif
        xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", %s received a message id %" PRIx32 " hash %" PRIx64 " from %s to %s which is not an auditor or archive node",
              vnetwork_message.hash(),
              m_filter_mgr_ptr->get_vhost_ptr()->host_node_id().to_string().c_str(),
              static_cast<std::uint32_t>(message.id()),
              message.hash(),
              sender.to_string().c_str(),
              receiver.to_string().c_str());
        vnetwork_message = empty_message;
        return;
    }

    assert(common::has<common::xnode_type_t::consensus_auditor>(src_type));

    // if receiver has version, the associated auditor must be matched with the sender.
    std::shared_ptr<election::cache::xgroup_element_t> associated_parent{nullptr};

    if (receiver.version().has_value()) {
        assert(m_filter_mgr_ptr->get_election_data_accessor_ptr() != nullptr);
        std::error_code ec{election::xdata_accessor_errc_t::success};
        associated_parent = m_filter_mgr_ptr->get_election_data_accessor_ptr()->parent_group_element(receiver.sharding_address(), receiver.version(), ec);
        if (ec) {
            xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", network %" PRIu32 " node %s receives msg sent to %s. ignored. error: %s",
                  vnetwork_message.hash(),
                  static_cast<std::uint32_t>(m_filter_mgr_ptr->get_vhost_ptr()->network_id().value()),
                  m_filter_mgr_ptr->get_vhost_ptr()->host_node_id().value().c_str(),
                  receiver.to_string().c_str(),
                  ec.message().c_str());
            vnetwork_message = empty_message;
            return;
        }

        if (!(sender.cluster_address() == associated_parent->address().cluster_address() && sender.version() == associated_parent->version())) {
            #if VHOST_METRICS
            XMETRICS_COUNTER_INCREMENT("vhost_discard_validation_failure", 1);
            #endif
            xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", %s received a message id %" PRIx32 " sent to %s from %s which is not its associated parent (%s)",
                  message.hash(),
                  m_filter_mgr_ptr->get_vhost_ptr()->host_node_id().to_string().c_str(),
                  static_cast<std::uint32_t>(message.id()),
                  receiver.to_string().c_str(),
                  sender.to_string().c_str(),
                  associated_parent->address().to_string().c_str());
            vnetwork_message = empty_message;
            return;
        }
    } else {
        assert(receiver.version().empty());
        std::error_code ec{election::xdata_accessor_errc_t::success};
        associated_parent = m_filter_mgr_ptr->get_election_data_accessor_ptr()->group_element(sender.sharding_address(), sender.version(), ec);
        if (ec) {
            xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", network %" PRIu32 " node %s receives msg sent to %s. ignored. error: %s",
                  message.hash(),
                  static_cast<std::uint32_t>(m_filter_mgr_ptr->get_vhost_ptr()->network_id().value()),
                  m_filter_mgr_ptr->get_vhost_ptr()->host_node_id().value().c_str(),
                  receiver.to_string().c_str(),
                  ec.message().c_str());
            vnetwork_message = empty_message;
            return;
        }
    }
    assert(associated_parent != nullptr);
    if (receiver.version().empty()) {
        std::error_code ec{election::xdata_accessor_errc_t::success};
        auto const validator_children = associated_parent->associated_child_groups(msg_time, ec);
        if (ec) {
            xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", %s network %" PRIu32 " node %s receives msg sent to %s. associated_child_groups failed with error %s",
                  vnetwork_message.hash(),
                  vnetwork::vnetwork_category2().name(),
                  static_cast<std::uint32_t>(m_filter_mgr_ptr->get_vhost_ptr()->network_id().value()),
                  m_filter_mgr_ptr->get_vhost_ptr()->host_node_id().value().c_str(),
                  receiver.to_string().c_str(),
                  ec.message().c_str());
            vnetwork_message = empty_message;
            return;
        }

        common::xversion_t max_validator_version;
        for (auto const & validator : validator_children) {
            if ((validator->address().cluster_address() == receiver.cluster_address()) && (max_validator_version < validator->version())) {
                max_validator_version = validator->version();
            }
        }

        if (!max_validator_version.empty()) {
            if (receiver.account_address().empty()) {
                vnetwork_message.receiver(common::xnode_address_t{receiver.cluster_address(), max_validator_version, receiver.sharding_size(), receiver.associated_blk_height()});
            } else {
                vnetwork_message.receiver(common::xnode_address_t{
                    receiver.cluster_address(), receiver.account_election_address(), max_validator_version, receiver.sharding_size(), receiver.associated_blk_height()});
            }
        } else {
            xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", no validator (%s) found associated with auditor %s for msg %" PRIx32 " hash %" PRIx64 " (msg logic time %" PRIu64
                  ") at logic time %" PRIu64,
                  vnetwork_message.hash(),
                  receiver.to_string().c_str(),
                  sender.to_string().c_str(),
                  static_cast<std::uint32_t>(message.id()),
                  message.hash(),
                  msg_time,
                  m_filter_mgr_ptr->get_vhost_ptr()->last_logic_time());
            vnetwork_message = empty_message;
        }
    }
}

void xmsg_filter_validator_from_archive::filt(xvnetwork_message_t & vnetwork_message) {
    if (!common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.receiver().type()) ||
        vnetwork_message.sender().cluster_address() == vnetwork_message.receiver().cluster_address()) {
        return;
    }
    if (common::has<common::xnode_type_t::consensus_auditor>(vnetwork_message.sender().type())) {
        return;
    }
    auto & message = vnetwork_message.message();
    auto & receiver = vnetwork_message.receiver();
    auto & sender = vnetwork_message.sender();
    auto const msg_time = vnetwork_message.logic_time();
    xvnetwork_message_t empty_message{};

    assert(common::has<common::xnode_type_t::storage>(sender.type()));

    if (receiver.version().empty()) {
        std::error_code ec{election::xdata_accessor_errc_t::success};
        auto const group = m_filter_mgr_ptr->get_election_data_accessor_ptr()->group_element_by_logic_time(receiver.sharding_address(), msg_time, ec);
        if (ec) {
            xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", network %" PRIu32 " node %s receives msg sent to %s. ignored. error: %s",
                  vnetwork_message.hash(),
                  static_cast<std::uint32_t>(m_filter_mgr_ptr->get_vhost_ptr()->network_id().value()),
                  m_filter_mgr_ptr->get_vhost_ptr()->host_node_id().value().c_str(),
                  receiver.to_string().c_str(),
                  ec.message().c_str());
            vnetwork_message = empty_message;
            return;
        }
        auto version = group->version();

        if (receiver.account_address().empty()) {
            vnetwork_message.receiver(common::xnode_address_t{receiver.cluster_address(), version, receiver.sharding_size(), receiver.associated_blk_height()});

        } else {
            vnetwork_message.receiver(
                common::xnode_address_t{receiver.cluster_address(), receiver.account_election_address(), version, receiver.sharding_size(), receiver.associated_blk_height()});
        }
    }
}

void xmsg_filter_auditor_from_validator::filt(xvnetwork_message_t & vnetwork_message) {
    if (!common::has<common::xnode_type_t::consensus_auditor>(vnetwork_message.receiver().type()) ||
        !(common::has<common::xnode_type_t::consensus_validator>(vnetwork_message.sender().type()))) {
        return;
    }

    auto & message = vnetwork_message.message();
    auto & receiver = vnetwork_message.receiver();
    auto & sender = vnetwork_message.sender();
    xvnetwork_message_t empty_message{};

    std::error_code ec{};

    std::shared_ptr<election::cache::xgroup_element_t> auditor{nullptr};
    // for a auditor node, if the incomming message is from validator,
    // then this validator must be from its associated validator.
    if (receiver.version().empty()) {
        if (sender.version().empty()) {
            // if auditor version and validator version are both empty. it's not acceptable.
            xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", invalid message sent from validator to auditor. message id %" PRIx32 " hash %" PRIx64 " ",
                  vnetwork_message.hash(),
                  message.id(),
                  message.hash());
            vnetwork_message = empty_message;
            return;
        }

        xdbg("[vnetwork] auditor received message %" PRIx64 " from validator but not specify the auditor round version.  calculating...");

        auditor = m_filter_mgr_ptr->get_election_data_accessor_ptr()->parent_group_element(sender.sharding_address(), sender.version(), ec);
        if (ec) {
            xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", network %" PRIu32 " node %s receives msg sent to %s. ignored. error: %s",
                  vnetwork_message.hash(),
                  static_cast<std::uint32_t>(m_filter_mgr_ptr->get_vhost_ptr()->network_id().value()),
                  m_filter_mgr_ptr->get_vhost_ptr()->host_node_id().value().c_str(),
                  receiver.to_string().c_str(),
                  ec.message().c_str());
            vnetwork_message = empty_message;
            return;
        }
    } else {
        auditor = m_filter_mgr_ptr->get_election_data_accessor_ptr()->group_element(receiver.sharding_address(), receiver.version(), ec);
        if (ec) {
            xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", network %" PRIu32 " node %s receives msg sent to %s. ignored. error: %s",
                  vnetwork_message.hash(),
                  static_cast<std::uint32_t>(m_filter_mgr_ptr->get_vhost_ptr()->network_id().value()),
                  m_filter_mgr_ptr->get_vhost_ptr()->host_node_id().value().c_str(),
                  receiver.to_string().c_str(),
                  ec.message().c_str());
            vnetwork_message = empty_message;
            return;
        }
    }

    ec.clear();
    auto const validator_children = auditor->associated_child_groups(common::xjudgement_day, ec);
    if (ec) {
        xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", %s auditor %s queries associated child groups failed with msg %s",
              vnetwork_message.hash(),
              vnetwork::vnetwork_category2().name(),
              auditor->address().to_string().c_str(),
              ec.message().c_str());
        vnetwork_message = empty_message;
        return;
    }

    bool valid_child{false};
    for (auto const & validator_child : validator_children) {
        if (validator_child->version() != sender.version()) {
            continue;
        }
        if (validator_child->group_id() != sender.group_id()) {
            continue;
        }
        if (validator_child->cluster_id() != sender.cluster_id()) {
            xdbg("[vnetwork][message_filter] hash: %" PRIx64 ", recving msg from different area: src area %s dst area %s",
                 vnetwork_message.hash(),
                 validator_child->cluster_id().to_string().c_str(),
                 sender.cluster_id().to_string().c_str());

            assert(false);
            continue;
        }
        if (validator_child->zone_id() != sender.zone_id()) {
            xdbg("[vnetwork][message_filter] hash: %" PRIx64 ", recving msg from different zone: src zone %s dst zone %s",
                 vnetwork_message.hash(),
                 validator_child->zone_id().to_string().c_str(),
                 sender.zone_id().to_string().c_str());

            assert(false);
            continue;
        }
        if (validator_child->network_id() != sender.network_id()) {
            xdbg("[vnetwork][message_filter] hash: %" PRIx64 ", recving msg from different network: src network %s dst network %s",
                 vnetwork_message.hash(),
                 validator_child->network_id().to_string().c_str(),
                 sender.network_id().to_string().c_str());

            assert(false);
            continue;
        }
        valid_child = true;
        break;
    }

    if (!valid_child) {
        #if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_discard_validation_failure", 1);
        #endif
        xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", %s received a message id %" PRIx32 " hash %" PRIx64 " sent to %s from %s which is not its associated child",
              vnetwork_message.hash(),
              m_filter_mgr_ptr->get_vhost_ptr()->host_node_id().to_string().c_str(),
              static_cast<std::uint32_t>(message.id()),
              message.hash(),
              receiver.to_string().c_str(),
              sender.to_string().c_str());
        // assert(false);   // TODO: revert after VNode destory enabled
        vnetwork_message = empty_message;
    }
}

void xmsg_filter_version_still_empty::filt(xvnetwork_message_t & vnetwork_message) {
    #if VHOST_METRICS
    XMETRICS_COUNTER_INCREMENT("vhost_received_valid", 1);
    #endif

    if (vnetwork_message.receiver().version().empty()) {
        auto & message = vnetwork_message.message();
        auto & receiver = vnetwork_message.receiver();
        auto & sender = vnetwork_message.sender();
        auto const msg_time = vnetwork_message.logic_time();
        xvnetwork_message_t empty_message{};

        if (common::has<common::xnode_type_t::consensus_validator>(receiver.type())) {
            xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", receive empty version to validator!!!!", vnetwork_message.hash());
            vnetwork_message = empty_message;
            return;
        }

        std::error_code ec{election::xdata_accessor_errc_t::success};
        auto const group_element = m_filter_mgr_ptr->get_election_data_accessor_ptr()->group_element_by_logic_time(receiver.sharding_address(), msg_time, ec);
        if (!ec) {
            if (receiver.account_election_address().empty()) {
                vnetwork_message.receiver(
                    common::xnode_address_t{receiver.sharding_address(), group_element->version(), group_element->sharding_size(), group_element->associated_blk_height()});

            } else {
                vnetwork_message.receiver(common::xnode_address_t{receiver.sharding_address(),
                                                                  receiver.account_election_address(),
                                                                  group_element->version(),
                                                                  group_element->sharding_size(),
                                                                  group_element->associated_blk_height()});
            }
        }
        // todo: ec?
    }
}

NS_END2
