// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xdata/xethheader.h"
#include "xdata/xethreceipt.h"
#include "xdata/xethtransaction.h"

NS_BEG2(top, data)

struct xethheader_para_t {
    uint64_t                m_gaslimit{0};  // the block gaslimit
    evm_common::u256        m_baseprice;
    common::xeth_address_t  m_coinbase;
};

class xeth_build_t {
 public:
    static evm_common::h256         build_transactions_root(const xeth_transactions_t & ethtxs);
    static evm_common::h256         build_receipts_root(const xeth_receipts_t & receipts);
    static void                     build_ethheader(xethheader_para_t const& para, const xeth_transactions_t & ethtxs, const xeth_store_receipts_t & receipts, evm_common::xh256_t const & state_root, xeth_header_t & ethheader);
    static void                     build_ethheader(xethheader_para_t const& para, const xeth_transactions_t & ethtxs, const xeth_receipts_t & receipts, evm_common::xh256_t const & state_root, xeth_header_t & ethheader);
};

NS_END2
