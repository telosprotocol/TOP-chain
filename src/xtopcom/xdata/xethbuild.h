// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xdata/xethheader.h"
#include "xdata/xethreceipt.h"

NS_BEG2(top, data)

class xeth_build_t {
 public:
    static evm_common::h256         build_receipts_root(const xeth_receipts_t & receipts);
    // static evm_common::LogBloom     build_block_logsbloom(const xeth_receipts_t & receipts);
    static uint64_t                 calc_receipts_gas_used(const xeth_receipts_t & receipts);
};

NS_END2
