// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xbasic/xbyte_buffer.h"
// #include "xevm_common/rlp/xrlp_iowriter.h"

#include <system_error>

NS_BEG3(top, evm_common, rlp)

template <typename T>
class xtop_rlp_encodable {
public:
    xtop_rlp_encodable() = default;
    xtop_rlp_encodable(xtop_rlp_encodable const &) = default;
    xtop_rlp_encodable & operator=(xtop_rlp_encodable const &) = default;
    xtop_rlp_encodable(xtop_rlp_encodable &&) = default;
    xtop_rlp_encodable & operator=(xtop_rlp_encodable &&) = default;
    virtual ~xtop_rlp_encodable() = default;

    virtual void EncodeRLP(xbytes_t & buf, std::error_code & ec) = 0;
};

template <typename T>
using xrlp_encodable_t = xtop_rlp_encodable<T>;

NS_END3