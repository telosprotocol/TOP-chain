// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xversion.h"
#include "xbase/xmem.h"
#include "xdata/xtransaction.h"

NS_BEG2(top, xunit_service)

using data::xtransaction_ptr_t;
using data::xreceipt_unit_ptr_t;

class xproposal_tx_t {
 public:
    void serialize(base::xstream_t & stream);
    void deserialize(base::xstream_t & stream);

    uint8_t             m_tx_type;
    xtransaction_ptr_t  m_tx;
    xreceipt_unit_ptr_t m_receipt;
};

class xproposal_account_t {
 public:
    void serialize(base::xstream_t & stream);
    void deserialize(base::xstream_t & stream);

    std::string                     m_account;
    uint64_t                        m_last_unit_height;
    std::string                     m_last_unit_hash;
    std::vector<xproposal_tx_t>     m_txs;
};

class xproposal_batch_account_t {
 public:
    void serialize(base::xstream_t & stream);
    void deserialize(base::xstream_t & stream);

    std::vector<xproposal_account_t>    m_accounts;
};


NS_END2


