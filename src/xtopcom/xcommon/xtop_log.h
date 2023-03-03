// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xmem.h"
#include "xbasic/xbyte_buffer.h"
#include "xcommon/xaccount_address.h"
#include "xcommon/rlp.h"
#include "xcommon/xbloom9.h"
#include "xcommon/xfixed_hash.h"

#include <string>
#include <vector>

NS_BEG2(top, common)

class xtop_log_t {
public:
    xtop_log_t() = default;
    xtop_log_t(xtop_log_t const &) = default;
    xtop_log_t & operator=(xtop_log_t const &) = default;
    xtop_log_t(xtop_log_t &&) = default;
    xtop_log_t & operator=(xtop_log_t &&) = default;
    ~xtop_log_t() = default;

    xtop_log_t(xaccount_address_t const & _address, evm_common::xh256s_t const & topics, xbytes_t const & data);
    
    void streamRLP(evm_common::RLPStream & _s) const;
    void decodeRLP(evm_common::RLP const & _r, std::error_code & ec);
    evm_common::xbloom9_t bloom() const;

    xaccount_address_t address;
    evm_common::xh256s_t topics;
    xbytes_t data;
};

using xtop_logs_t = std::vector<xtop_log_t>;

NS_END2
