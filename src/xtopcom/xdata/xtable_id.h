// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>

#include "xbase/xns_macro.h"
// TODO(jimmy) #include "xbase/xvledger.h"

NS_BEG2(top, data)

class xtable_id_t {
 public:
    xtable_id_t(base::enum_xchain_zone_index zone_index, uint16_t subaddr) {
        m_zone_index = zone_index;
        m_subaddr = subaddr;
    }
    base::enum_xchain_zone_index    get_zone_index() const {return m_zone_index;}
    uint16_t                        get_subaddr() const {return m_subaddr;}

 private:
    base::enum_xchain_zone_index    m_zone_index;
    uint16_t                        m_subaddr;
};

NS_END2
