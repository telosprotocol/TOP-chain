// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xrunnable.h"
#include "xcommon/xaddress.h"
#include "xelection/xcache/xgroup_update_result.h"
#include "xvnode/xvnode_manager_face_fwd.h"

#include <unordered_map>
#include <vector>

NS_BEG2(top, vnode)

class xtop_vnode_manager_face : public xbasic_runnable_t<xtop_vnode_manager_face> {
public:
    xtop_vnode_manager_face() = default;
    xtop_vnode_manager_face(xtop_vnode_manager_face const &) = delete;
    xtop_vnode_manager_face & operator=(xtop_vnode_manager_face const &) = delete;
    xtop_vnode_manager_face(xtop_vnode_manager_face &&) = default;
    xtop_vnode_manager_face & operator=(xtop_vnode_manager_face &&) = default;
    ~xtop_vnode_manager_face() override = default;

    virtual std::pair<std::vector<common::xip2_t>, std::vector<common::xip2_t>> handle_election_data(std::unordered_map<common::xsharding_address_t, election::cache::xgroup_update_result_t> const & election_data) = 0;
    // virtual std::shared_ptr<xvnode_face_t> create_vnode_at(std::shared_ptr<election::cache::xgroup_element_t> const & group) const = 0;
};
using xvnode_manager_face_t = xtop_vnode_manager_face;

NS_END2
