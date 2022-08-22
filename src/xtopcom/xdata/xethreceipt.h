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
#include "xdata/xethtransaction.h"

NS_BEG2(top, data)

enum enum_ethreceipt_status {
    ethreceipt_status_failed = 0,
    ethreceipt_status_successful = 1
};

class xeth_receipt_t;
// xeth_store_receipt_t is packed in block
class xeth_store_receipt_t {
 public:
    xeth_store_receipt_t() = default;

    xbytes_t    encodeBytes() const;
    void        decodeBytes(xbytes_t const& _d, std::error_code & ec);
 public:  // get APIS
    uint8_t                     get_version() const {return m_version;}
    enum_ethreceipt_status      get_tx_status() const {return m_tx_status;}
    uint64_t                    get_cumulative_gas_used() const {return m_cumulative_gas_used;}
    const evm_common::xevm_logs_t & get_logs() const {return m_logs;}
    evm_common::u256            get_gas_price() const {return m_gas_price;}
    uint64_t                    get_gas_used() const {return m_gas_used;}
    common::xeth_address_t const&   get_contract_address() const {return m_contract_address;}
    evm_common::xbloom9_t       bloom() const;

 public:
    void    set_tx_status(enum_ethreceipt_status status) {m_tx_status = status;}
    void    set_cumulative_gas_used(uint64_t gas) {m_cumulative_gas_used = gas;}
    void    set_logs(evm_common::xevm_logs_t const& logs) {m_logs = logs;}
    void    set_gas_price(evm_common::u256 const& price) {m_gas_price = price;}
    void    set_gas_used(uint64_t gas) {m_gas_used = gas;}
    void    set_contract_address(common::xeth_address_t const& addr) {m_contract_address = addr;}

 protected:
    void    streamRLP(evm_common::RLPStream& _s) const;
    void    decodeRLP(evm_common::RLP const& _r, std::error_code & ec);

 private:
    uint8_t                     m_version{0};
    enum_ethreceipt_status      m_tx_status{ethreceipt_status_failed};
    uint64_t                    m_cumulative_gas_used{0};
    evm_common::xevm_logs_t     m_logs;
    evm_common::u256            m_gas_price;  // TODO(jimmy) effective gas price
    uint64_t                    m_gas_used{0};
    common::xeth_address_t      m_contract_address;
};

using xeth_store_receipts_t = std::vector<xeth_store_receipt_t>;
using xeth_store_receipt_ptr_t = std::shared_ptr<xeth_store_receipt_t>;

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
 public:
    void        set_tx_version_type(enum_ethtx_version version) {m_tx_version_type = version;}
    void        set_tx_status(enum_ethreceipt_status _status) {m_tx_status = _status;}
    void        set_cumulative_gas_used(uint64_t _gas) {m_cumulative_gas_used = _gas;}
    void        set_logs(evm_common::xevm_logs_t const& logs) {m_logs = logs;}

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

// xeth_local_receipt_t represents the full results of a transaction
class xeth_local_receipt_t : public xeth_receipt_t {
 public:
    xeth_local_receipt_t() = default;
    xeth_local_receipt_t(enum_ethtx_version _version, enum_ethreceipt_status _status, uint64_t _gasused, evm_common::xevm_logs_t const & _logs);
    ~xeth_local_receipt_t() = default;

 public:  // get APIS
    std::string const&              get_tx_hash() const {return m_tx_hash;}
    uint64_t                        get_gas_used() const {return m_used_gas;}
    common::xeth_address_t const&   get_contract_address() const {return m_contract_address;}
    common::xeth_address_t const&   get_from_address() const {return m_from_address;}
    common::xeth_address_t const&   get_to_address() const {return m_to_address;}
    std::string const&              get_block_hash() const {return m_block_hash;}
    uint64_t                        get_block_number() const {return m_block_number;}
    uint32_t                        get_transaction_index() const {return m_transaction_index;}

 public:
    void        set_block_hash(std::string const& blockhash);
    void        set_block_number(uint64_t blocknumer);
    void        set_transaction_index(uint32_t index);
    void        set_tx_hash(std::string const& txhash);
    void        set_from_address(common::xeth_address_t const& address);
    void        set_to_address(common::xeth_address_t const& address);
    void        set_contract_address(common::xeth_address_t const& address);
    void        set_used_gas(uint64_t used_gas);

 private:
    // Implementation fields: These fields are added by geth when processing a transaction.
    // They are stored in the chain database.
    std::string                 m_tx_hash;
    common::xeth_address_t      m_from_address;
    common::xeth_address_t      m_to_address;
    common::xeth_address_t      m_contract_address;
    uint64_t                    m_used_gas{0};
    // Inclusion information: These fields provide information about the inclusion of the
    // transaction corresponding to this receipt.
    std::string                 m_block_hash;
    uint64_t                    m_block_number{0};
    uint32_t                    m_transaction_index{0};
};

using xeth_receipt_ptr_t = std::shared_ptr<xeth_receipt_t>;
using xeth_local_receipt_prt_t = std::shared_ptr<xeth_local_receipt_t>;
using xeth_receipts_t = std::vector<xeth_receipt_t>;
using xeth_local_receipts_t = std::vector<xeth_local_receipt_t>;

NS_END2
