// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xunit_service/xtimer_service.h"

NS_BEG2(top, xunit_service)

xtimer_service_t::xtimer_service_t(std::shared_ptr<xcons_service_para_face> const &p_para,
                                   std::shared_ptr<xcons_dispatcher> const &       dispatcher)
    : xcons_service_t(p_para, dispatcher, xmessage_category_timer) {
    xassert(p_para != nullptr);
    xassert(dispatcher != nullptr);
    // vnode build
    xunit_dbg("xtimer_service_t::xtimer_service_t,create,this=%p", this);
}

xtimer_service_t::~xtimer_service_t() {
    xunit_dbg("xtimer_service_t::~xtimer_service_t,detroy,this=%p", this);
}

NS_END2
