// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject_ptr.h"
#include "xunit_service/xrelay_packer.h"

NS_BEG2(top, xunit_service)

class xrelay_dispatcher_t
  : public xcons_dispatcher
  , public std::enable_shared_from_this<xrelay_dispatcher_t> {
public:
    xrelay_dispatcher_t(std::shared_ptr<xcons_service_para_face> const & para, std::shared_ptr<xblock_maker_face> const & block_maker);

    virtual ~xrelay_dispatcher_t();

    // dispatch events
    bool dispatch(base::xworkerpool_t * pool, base::xcspdu_t * pdu, const xvip2_t & xip_from, const xvip2_t & xip_to) override;

public:
    virtual bool start(const xvip2_t & xip, const common::xlogic_time_t & start_time);

    virtual bool fade(const xvip2_t & xip);

    virtual bool unreg(const xvip2_t & xip);

    virtual bool destroy(const xvip2_t & xip);

protected:
    void chain_timer(common::xlogic_time_t time);
    void on_clock(base::xvblock_t * clock_block);

protected:
    xrelay_packer * m_packer{};
    std::shared_ptr<xcons_service_para_face> m_para;
    std::shared_ptr<xblock_maker_face> m_block_maker;
    base::xworker_t * m_worker;
    std::mutex m_mutex;
    std::string m_watcher_name;
};

NS_END2
