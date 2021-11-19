// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xunit_service/xcons_face.h"

NS_BEG2(top, xunit_service)

// default block service entry
class xcons_service_t : public xcons_service_face, public xpdu_reactor_face, public std::enable_shared_from_this<xcons_service_t> {
public:
    explicit xcons_service_t(std::shared_ptr<xcons_service_para_face> const &p_para,
                             std::shared_ptr<xcons_dispatcher> const &      dispatcher,
                             common::xmessage_category_t                     category);
    virtual ~xcons_service_t();

public:
    common::xmessage_category_t get_msg_category() override;
    bool start(const xvip2_t & xip, const common::xlogic_time_t& start_time) override;
    bool fade(const xvip2_t & xip) override;
    bool unreg(const xvip2_t & xip) override;
    bool destroy(const xvip2_t & xip) override;
public:
    bool is_running() override;
    void on_pdu(const xvip2_t &xip_from,
            const xvip2_t &xip_to, const base::xcspdu_t &packet) override;
protected:
    std::shared_ptr<xcons_service_para_face> m_para{};
    std::shared_ptr<xcons_dispatcher>        m_dispatcher{};
    common::xmessage_category_t              m_category{};
    volatile bool                            m_running{false};
};

NS_END2
