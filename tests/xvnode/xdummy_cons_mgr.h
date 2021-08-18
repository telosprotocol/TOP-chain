// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
# if 0
#pragma once
#include "xbasic/xmemory.hpp"
#include "xchain_timer/xchain_timer_face.h"
#include "xunit_service/xcons_face.h"
#include "xunit_service/xcons_utl.h"

#include <map>
#include <mutex>
#include <vector>

NS_BEG3(top, tests, vnode)

class xtop_dummy_cons_proxy_face : public top::xunit_service::xcons_proxy_face {
public:
    xtop_dummy_cons_proxy_face(uint16_t init_count) : start_count{init_count}, fade_count{init_count} {}
    bool start(const common::xlogic_time_t& start_time) {
        start_count++;
        return false;
    }
    bool fade() {
        fade_count++;
        return false;
    }
    bool outdated() { return false; }
    xvip2_t get_ip() { return {}; }
    uint16_t start_count;
    uint16_t fade_count;
};
using xdummy_cons_proxy_face = xtop_dummy_cons_proxy_face;

class xtop_dummy_cons_service_mgr
  : public top::xunit_service::xcons_service_mgr_face
  , public std::enable_shared_from_this<xtop_dummy_cons_service_mgr> {
public:
    xtop_dummy_cons_service_mgr() { m_dummy_cons_proxy_face_ptr = std::make_shared<xdummy_cons_proxy_face>(0); }

    top::xunit_service::xcons_proxy_face_ptr create(const std::shared_ptr<top::vnetwork::xvnetwork_driver_face_t> & network) { return m_dummy_cons_proxy_face_ptr; }

    bool destroy(const xvip2_t & xip) { return false; }

    bool start(const xvip2_t & xip, const common::xlogic_time_t& start_time) { return false; }

    bool fade(const xvip2_t & xip) { return false; }

    std::shared_ptr<xdummy_cons_proxy_face> test_get_cons_proxy_face_ptr() { return m_dummy_cons_proxy_face_ptr; }

    std::shared_ptr<xdummy_cons_proxy_face> m_dummy_cons_proxy_face_ptr;
};
using xdummy_cons_service_mgr_t = xtop_dummy_cons_service_mgr;

extern xdummy_cons_service_mgr_t dummy_cons_service_mgr;

NS_END3
#endif