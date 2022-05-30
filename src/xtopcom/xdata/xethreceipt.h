// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xevm_common/common.h"
#include "xevm_common/xfixed_hash.h"
#include "xevm_common/xevm_transaction_result.h"
#include "xevm_common/rlp.h"
#include "xevm_common/address.h"
#include "xevm_common/xbloom9.h"
#include "xdata/xserial_transfrom.h"

NS_BEG2(top, data)

enum enum_ethreceipt_status {
    ethreceipt_status_failed = 0,
    ethreceipt_status_successful = 1
};

// xeth_receipt_t is fully compatiable with eth
class xeth_receipt_t {
 public:
    xeth_receipt_t() = default;
    xeth_receipt_t(enum_ethtx_version _version, enum_ethreceipt_status _status, uint64_t _gasused, evm_common::xevm_logs_t const & _logs);

    xbytes_t    encodeBytes() const;
    void        decodeBytes(xbytes_t const& _d, std::error_code & ec);
 public:  // set APIS
    void        create_bloom();
 public:  // get APIS
    enum_ethtx_version          get_tx_version_type() const {return m_tx_version_type;}
    enum_ethreceipt_status      get_tx_status() const {return m_tx_status;}
    uint64_t                    get_cumulative_gas_used() const {return m_cumulative_gas_used;}
    const evm_common::xbloom9_t &   get_logsBloom() const {return m_logsBloom;}
    const evm_common::xevm_logs_t & get_logs() const {return m_logs;}

 protected:
    void    streamRLP(evm_common::RLPStream& _s) const;
    void    decodeRLP(evm_common::RLP const& _r, std::error_code & ec);

 private:
    enum_ethtx_version          m_tx_version_type{EIP_1559};
    enum_ethreceipt_status      m_tx_status{ethreceipt_status_failed};
    uint64_t                    m_cumulative_gas_used{0};
    evm_common::xbloom9_t       m_logsBloom;
    evm_common::xevm_logs_t     m_logs;
};

// xeth_local_receipt_t represents the full results of a transaction.
class xeth_local_receipt_t : public xeth_receipt_t {
 public:
    xeth_local_receipt_t() = default;
    xeth_local_receipt_t(enum_ethtx_version _version, enum_ethreceipt_status _status, uint64_t _gasused, evm_common::xevm_logs_t const & _logs);
    ~xeth_local_receipt_t() = default;

 public:  // get APIS
    evm_common::xh256_t const&      get_tx_hash() const {return m_tx_hash;}
    uint64_t                        get_gas_used() const {return m_used_gas;}
    common::xeth_address_t const&   get_contract_address() const {return m_contract_address;}
    evm_common::xh256_t const&      get_block_hash() const {return m_block_hash;}
    uint64_t                        get_block_number() const {return m_block_number;}
    uint32_t                        get_transaction_index() const {return m_transaction_index;}

 private:
    // Implementation fields: These fields are added by geth when processing a transaction.
    // They are stored in the chain database.
    evm_common::xh256_t         m_tx_hash;
    common::xeth_address_t      m_contract_address;
    uint64_t                    m_used_gas{0};
    // Inclusion information: These fields provide information about the inclusion of the
    // transaction corresponding to this receipt.
    evm_common::xh256_t         m_block_hash;
    uint64_t                    m_block_number{0};
    uint32_t                    m_transaction_index{0};
};

using xeth_receipts_t = std::vector<xeth_receipt_t>;
using xeth_local_receipts_t = std::vector<xeth_local_receipt_t>;

NS_END2
