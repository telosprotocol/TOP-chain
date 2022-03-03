// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaddress.h"
#include "xcommon/xnode_type.h"
#include "xvnode/xvnode_face.h"

NS_BEG3(top, tests, vnode)

template <top::common::xnode_type_t NodeTypeV>
class xdummy_vnode;

#define XDUMMY_VNODE(TYPE)                                                                                                                                                         \
    template <>                                                                                                                                                                    \
    class xdummy_vnode<top::common::xnode_type_t::TYPE> : public top::vnode::xvnode_face_t {                                                                                       \
        top::common::xnode_address_t address_;                                                                                                                                     \
        top::common::xelection_round_t joined_election_round_;                                                                                                                     \
                                                                                                                                                                                   \
    public:                                                                                                                                                                        \
        xdummy_vnode(xdummy_vnode &&) = default;                                                                                                                                   \
        xdummy_vnode & operator=(xdummy_vnode &&) = default;                                                                                                                       \
                                                                                                                                                                                   \
        explicit xdummy_vnode(top::common::xnode_address_t address);                                                                                                               \
                                                                                                                                                                                   \
        top::common::xnode_type_t type() const noexcept override {                                                                                                                 \
            return address_.type();                                                                                                                                                \
        }                                                                                                                                                                          \
                                                                                                                                                                                   \
        top::common::xnode_address_t const & address() const noexcept override {                                                                                                   \
            return address_;                                                                                                                                                       \
        }                                                                                                                                                                          \
                                                                                                                                                                                   \
        top::common::xelection_round_t const & joined_election_round() const noexcept override {                                                                                   \
            return joined_election_round_;                                                                                                                                         \
        }                                                                                                                                                                          \
                                                                                                                                                                                   \
        void start() override {                                                                                                                                                    \
        }                                                                                                                                                                          \
                                                                                                                                                                                   \
        void stop() override {                                                                                                                                                     \
        }                                                                                                                                                                          \
                                                                                                                                                                                   \
        void fade() override {                                                                                                                                                     \
        }                                                                                                                                                                          \
                                                                                                                                                                                   \
        void synchronize() override {                                                                                                                                              \
        }                                                                                                                                                                          \
                                                                                                                                                                                   \
        void broadcast(common::xip2_t const & broadcast_dst, top::vnetwork::xmessage_t const &, std::error_code &) override {                                                      \
        }                                                                                                                                                                          \
                                                                                                                                                                                   \
        void send_to(common::xip2_t const & unicast_dst, top::vnetwork::xmessage_t const &, std::error_code &) override {                                                          \
        }                                                                                                                                                                          \
                                                                                                                                                                                   \
        common::xrotation_status_t status() const noexcept override {                                                                                                              \
            return common::xrotation_status_t::invalid;                                                                                                                            \
        }                                                                                                                                                                          \
                                                                                                                                                                                   \
        std::vector<common::xip2_t> neighbors_xip2(std::error_code &) const override {                                                                                             \
            return {};                                                                                                                                                             \
        }                                                                                                                                                                          \
                                                                                                                                                                                   \
        std::vector<common::xip2_t> associated_nodes_xip2(common::xip_t const &, std::error_code &) const {                                                                        \
            return {};                                                                                                                                                             \
        }                                                                                                                                                                          \
                                                                                                                                                                                   \
        std::vector<common::xip2_t> nonassociated_nodes_xip2(common::xip_t const &, std::error_code &) const {                                                                     \
            return {};                                                                                                                                                             \
        }                                                                                                                                                                          \
                                                                                                                                                                                   \
        std::vector<common::xip2_t> associated_parent_nodes_xip2(std::error_code &) const override {                                                                               \
            return {};                                                                                                                                                             \
        }                                                                                                                                                                          \
                                                                                                                                                                                   \
        std::vector<common::xip2_t> associated_child_nodes_xip2(common::xip2_t const &, std::error_code &) const override {                                                        \
            return {};                                                                                                                                                             \
        }                                                                                                                                                                          \
        top::vnode::components::sniffing::xvnode_sniff_config_t sniff_config() noexcept override {                                                                                 \
            return top::vnode::components::sniffing::xvnode_sniff_config_t{};                                                                                                      \
        }                                                                                                                                                                          \
    }

XDUMMY_VNODE(committee);
XDUMMY_VNODE(zec);
XDUMMY_VNODE(storage_archive);
XDUMMY_VNODE(edge);
XDUMMY_VNODE(consensus_auditor);
XDUMMY_VNODE(consensus_validator);

#undef XDUMMY_VNODE

NS_END3
