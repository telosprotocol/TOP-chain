// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvnetwork/xmessage_filter_base.h"

#include "xvnetwork/xvnetwork_error2.h"

#include <cinttypes>

NS_BEG2(top, vnetwork)

void xtop_message_filter_base::normalize_message_recver(xvnetwork_message_t & vnetwork_message,
                                                        observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                                                        observer_ptr<election::cache::xdata_accessor_face_t> const & data_accessor,
                                                        std::error_code & ec) const {
    assert(!ec);

    auto const & recver = vnetwork_message.receiver();
    assert(recver.logic_epoch().empty());

    auto const group_element = data_accessor->group_element_by_logic_time(recver.group_address(), vnetwork_message.logic_time(), ec);
    if (!ec) {
        if (recver.account_address().empty()) {
            vnetwork_message.receiver(common::xnode_address_t{ recver.group_address(), group_element->logic_epoch() });
        } else {
            auto const& node_element = group_element->node_element(recver.account_address(), ec);
            if (ec) {
                xinfo("message_filter: normalizing vnetwork_message failed. %s recv message %" PRIx32 " hash %" PRIx64 " from %s to %s; dropped due to %s",
                      vhost->account_address().to_string().c_str(),
                      static_cast<uint32_t>(vnetwork_message.message_id()),
                      static_cast<uint64_t>(vnetwork_message.hash()),
                      vnetwork_message.sender().to_string().c_str(),
                      vnetwork_message.receiver().to_string().c_str(),
                      ec.message().c_str());

                return;
            }

            assert(node_element != nullptr);

            if (broadcast(recver.account_election_address().slot_id())) {
                vnetwork_message.receiver(common::xnode_address_t{ recver.group_address(), common::xaccount_election_address_t{ recver.account_address(), node_element->slot_id() }, group_element->logic_epoch() });
            }
            else {
                if (recver.slot_id() != node_element->slot_id()) {
                    ec = top::vnetwork::xvnetwork_errc2_t::slot_id_mismatch;

                    xinfo("message_filter: normalizing vnetwork_message failed. %s recv message %" PRIx32 " hash %" PRIx64 " from %s to %s; dropped due to %s; local slot id %" PRIu16 " msg slot id %" PRIu16,
                          vhost->account_address().to_string().c_str(),
                          static_cast<uint32_t>(vnetwork_message.message_id()),
                          static_cast<uint64_t>(vnetwork_message.hash()),
                          vnetwork_message.sender().to_string().c_str(),
                          vnetwork_message.receiver().to_string().c_str(),
                          ec.message().c_str(),
                          static_cast<uint16_t>(node_element->slot_id()),
                          static_cast<uint16_t>(recver.slot_id()));

                    return;
                }
                vnetwork_message.receiver(common::xnode_address_t{ recver.group_address(), recver.account_election_address(), group_element->logic_epoch() });
            }
        }
    }
}

void xtop_message_filter_base::normalize_message_recver(xvnetwork_message_t & vnetwork_message,
                                                        observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                                                        std::shared_ptr<election::cache::xgroup_element_t> const & recver_group,
                                                        std::error_code & ec) const {
    assert(!ec);

    auto const & recver = vnetwork_message.receiver();
    assert(recver.logic_epoch().empty());

    if (!ec) {
        if (recver.account_address().empty()) {
            vnetwork_message.receiver(common::xnode_address_t{recver.group_address(), recver_group->logic_epoch()});
        } else {
            auto const & node_element = recver_group->node_element(recver.account_address(), ec);
            if (ec) {
                xinfo("message_filter: normalizing vnetwork_message failed. %s recv message %" PRIx32 " hash %" PRIx64 " from %s to %s; dropped due to %s",
                      vhost->account_address().to_string().c_str(),
                      static_cast<uint32_t>(vnetwork_message.message_id()),
                      static_cast<uint64_t>(vnetwork_message.hash()),
                      vnetwork_message.sender().to_string().c_str(),
                      vnetwork_message.receiver().to_string().c_str(),
                      ec.message().c_str());

                return;
            }

            assert(node_element != nullptr);

            if (broadcast(recver.account_election_address().slot_id())) {
                vnetwork_message.receiver(common::xnode_address_t{
                    recver.group_address(), common::xaccount_election_address_t{recver.account_address(), node_element->slot_id()}, recver_group->logic_epoch()});
            } else {
                if (recver.slot_id() != node_element->slot_id()) {
                    ec = top::vnetwork::xvnetwork_errc2_t::slot_id_mismatch;

                    xinfo("message_filter: normalizing vnetwork_message failed. %s recv message %" PRIx32 " hash %" PRIx64
                          " from %s to %s; dropped due to %s; local slot id %" PRIu16 " msg slot id %" PRIu16,
                          vhost->account_address().to_string().c_str(),
                          static_cast<uint32_t>(vnetwork_message.message_id()),
                          static_cast<uint64_t>(vnetwork_message.hash()),
                          vnetwork_message.sender().to_string().c_str(),
                          vnetwork_message.receiver().to_string().c_str(),
                          ec.message().c_str(),
                          static_cast<uint16_t>(node_element->slot_id()),
                          static_cast<uint16_t>(recver.slot_id()));

                    return;
                }
                vnetwork_message.receiver(common::xnode_address_t{recver.group_address(), recver.account_election_address(), recver_group->logic_epoch()});
            }
        }
    }
}

void xtop_message_filter_base::normalize_message_recver_by_message_sender(xvnetwork_message_t & vnetwork_message,
                                                                          observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                                                                          observer_ptr<election::cache::xdata_accessor_face_t> const & data_accessor,
                                                                          std::error_code & ec) const {
    auto const & recver = vnetwork_message.receiver();
    auto const & sender = vnetwork_message.sender();

    assert(recver.logic_epoch().empty());
    assert(sender.group_address() == recver.group_address());

    if (recver.account_address().empty()) {
        vnetwork_message.receiver(common::xnode_address_t{ recver.group_address(), sender.logic_epoch()});
    } else {
        auto const group = data_accessor->group_element_by_height(recver.group_address(), sender.associated_blk_height(), ec);
        if (ec) {
            xinfo("[vnetwork][message_filter] hash: %" PRIx64 ", network %" PRIu32 " node %s receives msg sent to %s from %s. ignored. error: %s",
                    vnetwork_message.hash(),
                    static_cast<std::uint32_t>(vhost->network_id().value()),
                  vhost->account_address().to_string().c_str(),
                    recver.to_string().c_str(),
                    vnetwork_message.sender().to_string().c_str(),
                    ec.message().c_str());

            return;
        }

        auto const & node_element = group->node_element(recver.account_address(), ec);
        if (ec) {
            xinfo("message_filter: %s recv message %" PRIx32 " hash %" PRIx64 " from %s to %s; dropped due to %s",
                  vhost->account_address().to_string().c_str(),
                  static_cast<uint32_t>(vnetwork_message.message_id()),
                  static_cast<uint64_t>(vnetwork_message.hash()),
                  vnetwork_message.sender().to_string().c_str(),
                  vnetwork_message.receiver().to_string().c_str(),
                  ec.message().c_str());

            return;
        }

        assert(node_element != nullptr);

        if (broadcast(recver.account_election_address().slot_id())) {
            vnetwork_message.receiver(
                common::xnode_address_t{recver.group_address(), common::xaccount_election_address_t{recver.account_address(), node_element->slot_id()}, group->logic_epoch()});
        } else {
            if (recver.slot_id() != node_element->slot_id()) {
                ec = top::vnetwork::xvnetwork_errc2_t::slot_id_mismatch;

                xinfo("message_filter: %s recv message %" PRIx32 " hash %" PRIx64 " from %s to %s; dropped due to %s; local slot id %" PRIu16 " msg slot id %" PRIu16,
                      vhost->account_address().to_string().c_str(),
                      static_cast<uint32_t>(vnetwork_message.message_id()),
                      static_cast<uint64_t>(vnetwork_message.hash()),
                      vnetwork_message.sender().to_string().c_str(),
                      vnetwork_message.receiver().to_string().c_str(),
                      ec.message().c_str(),
                      static_cast<uint16_t>(node_element->slot_id()),
                      static_cast<uint16_t>(recver.slot_id()));

                return;
            }
            vnetwork_message.receiver(common::xnode_address_t{recver.group_address(), recver.account_election_address(), group->logic_epoch()});
        }
    }
}

NS_END2
