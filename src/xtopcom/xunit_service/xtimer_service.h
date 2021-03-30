// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xunit_service/xcons_serivce.h"

NS_BEG2(top, xunit_service)

class xtimer_service_t : public xcons_service_t {
public:
    xtimer_service_t(std::shared_ptr<xcons_service_para_face> const &p_para,
                     std::shared_ptr<xcons_dispatcher> const &      dispatcher);
    virtual ~xtimer_service_t();
};

NS_END2