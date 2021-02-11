// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xtransaction.h"

#include <string>

NS_BEG2(top, xtxpool_service)

class xrequest_tx_receiver_face {
public:
    virtual int32_t request_transaction_consensus(const data::xtransaction_ptr_t & trans, bool local) = 0;
    virtual xcons_transaction_ptr_t query_tx(const std::string & account, const uint256_t & hash) const = 0;
};

NS_END2
