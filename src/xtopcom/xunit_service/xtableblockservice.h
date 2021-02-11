// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xunit_service/xcons_serivce.h"

NS_BEG2(top, xunit_service)

// default block service entry
class xtableblockservice : public xcons_service_t {
public:
    explicit xtableblockservice(const  std::shared_ptr<xcons_service_para_face> &p_para,
                                const std::shared_ptr<xcons_dispatcher> &      dispatcher);
    virtual ~xtableblockservice();
};

NS_END2
