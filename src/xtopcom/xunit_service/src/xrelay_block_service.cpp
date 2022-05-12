// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xunit_service/xrelay_block_service.h"
#include "xbase/xutl.h"
#include "xunit_service/xcons_utl.h"
#include "xunit_service/xworkpool_dispatcher.h"

NS_BEG2(top, xunit_service)

xrelay_block_service::xrelay_block_service(const std::shared_ptr<xcons_service_para_face> & p_para,
                                       const  std::shared_ptr<xcons_dispatcher> &      dispatcher)
    : xcons_service_t(p_para, dispatcher, xmessage_category_relay) {
    xassert(p_para != nullptr);
    xassert(dispatcher != nullptr);
    // vnode build
    // xunit_info("[tablelock] %p create address %s", this, p_para->get_resources()->get_network()->address().to_string().c_str());
    xunit_dbg("xrelay_block_service::xrelay_block_service,create,this=%p", this);
}

xrelay_block_service::~xrelay_block_service() {
    xunit_dbg("xrelay_block_service::~xrelay_block_service,destroy,this=%p", this);
}

NS_END2
