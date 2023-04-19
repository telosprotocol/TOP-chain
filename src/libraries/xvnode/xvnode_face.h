// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xrunnable.h"
#include "xcommon/xenable_synchronization.h"
#include "xcommon/xfadable.h"
#include "xcommon/xrotation_aware.h"
#include "xtxpool_service_v2/xtxpool_service_face.h"
#include "xvnetwork/xmessage.h"
#include "xvnetwork/xvnetwork_driver_face.h"
#include "xvnode/xcomponents/xblock_sniffing/xsniffer_config.h"

NS_BEG2(top, vnode)

class xtop_vnode_face : public xbasic_runnable_t<xtop_vnode_face>
                      , public common::xbasic_fadable_t<xtop_vnode_face>
                      , public common::xbasic_rotation_aware_t<xtop_vnode_face>
                      , public common::xenable_synchronization_t<xtop_vnode_face> {
public:
    xtop_vnode_face(xtop_vnode_face const &) = delete;
    xtop_vnode_face & operator=(xtop_vnode_face const &) = delete;
    xtop_vnode_face(xtop_vnode_face &&) = default;
    xtop_vnode_face & operator=(xtop_vnode_face &&) = default;
    ~xtop_vnode_face() override = default;

    virtual common::xnode_type_t type() const noexcept = 0;
    virtual common::xnode_address_t const & address() const noexcept = 0;

    virtual common::xminer_type_t miner_type() const noexcept = 0;
    virtual bool genesis() const noexcept = 0;
    virtual common::xelection_round_t const & joined_election_round() const noexcept = 0;

    virtual void broadcast(common::xip2_t const & broadcast_dst, vnetwork::xmessage_t const & msg, std::error_code & ec) = 0;
    virtual void send_to(common::xip2_t const & unicast_dst, vnetwork::xmessage_t const & msg, std::error_code & ec) = 0;

    virtual common::xrotation_status_t status() const noexcept = 0;

    virtual std::vector<common::xip2_t> neighbors_xip2(std::error_code & ec) const = 0;
    virtual std::vector<common::xip2_t> associated_parent_nodes_xip2(std::error_code & ec) const = 0;
    virtual std::vector<common::xip2_t> associated_child_nodes_xip2(common::xip2_t const & child_group_xip2, std::error_code & ec) const = 0;

    virtual components::sniffing::xsniffer_config_t sniff_config() const = 0;
    // virtual xtxpool_service_v2::xtxpool_proxy_face_ptr const & txpool_proxy() const =0;
    virtual std::shared_ptr<vnetwork::xvnetwork_driver_face_t> const & vnetwork_driver() const = 0;

    virtual uint64_t raw_credit_score() const noexcept = 0;

protected:
    xtop_vnode_face() = default;
};
using xvnode_face_t = xtop_vnode_face;

NS_END2
