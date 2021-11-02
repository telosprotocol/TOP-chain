// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject_ptr.h"
#include "xunit_service/xtimer_picker.h"

NS_BEG2(top, xunit_service)

class xtimer_dispatcher_t : public xcons_dispatcher {
public:
    xtimer_dispatcher_t(std::shared_ptr<xcons_service_para_face> const &para,
                        std::shared_ptr<xblock_maker_face> const &      block_maker);

    virtual ~xtimer_dispatcher_t();

    // dispatch events
    bool dispatch(base::xworkerpool_t *pool, base::xcspdu_t *pdu, const xvip2_t &xip_from, const xvip2_t &xip_to) override;

public:
    virtual bool start(const xvip2_t &xip, const common::xlogic_time_t& start_time);

    virtual bool fade(const xvip2_t &xip);

    virtual bool unreg(const xvip2_t &xip);

    virtual bool destroy(const xvip2_t &xip);

protected:
    xtimer_picker_t *m_picker{};
    std::shared_ptr<xcons_service_para_face> m_para;
    std::shared_ptr<xblock_maker_face>       m_block_maker;
};

NS_END2
