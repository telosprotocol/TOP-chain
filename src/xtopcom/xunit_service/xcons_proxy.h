// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <vector>
#include "xunit_service/xcons_face.h"

NS_BEG2(top, xunit_service)

// default block service entry
class xcons_proxy : public xcons_proxy_face {
public:
    explicit xcons_proxy(const xvip2_t & xip,
                         const std::shared_ptr<xcons_service_mgr_face> & cons_mgr);
public:
    bool start(const common::xlogic_time_t& start_time);
    bool fade() override;
    bool outdated() override;
    xvip2_t get_ip() override;
private:
    std::shared_ptr<xcons_service_mgr_face> m_cons_mgr;
    xvip2_t m_xip;
};

NS_END2
