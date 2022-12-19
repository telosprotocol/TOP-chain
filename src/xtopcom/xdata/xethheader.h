// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbasic/xbyte_buffer.h"
#include "xcommon/xeth_address.h"
#include "xevm_common/common.h"
#include "xevm_common/xfixed_hash.h"
#include "xevm_common/rlp.h"
#include "xevm_common/xbloom9.h"

NS_BEG2(top, data)

class xeth_header_t {
 public:
    xeth_header_t() = default;
    ~xeth_header_t() = default;

 public:  // serialize
    std::string serialize_to_string() const;
    void        serialize_from_string(const std::string & bin_data, std::error_code & ec);
    xbytes_t    encodeBytes() const;
    void        decodeBytes(xbytes_t const& _d, std::error_code & ec);
    void        streamRLP(evm_common::RLPStream& _s) const;
    void        decodeRLP(evm_common::RLP const& _r, std::error_code & ec);
 public:  // get APIS
    uint64_t    get_gaslimit() const {return m_gaslimit;}
    uint64_t    get_gasused() const {return m_gasused;}
    const evm_common::u256 &            get_baseprice() const {return m_baseprice;}
    const evm_common::xbloom9_t &       get_logBloom() const {return m_logBloom;}
    const evm_common::xh256_t &         get_transactions_root() const {return m_transactions_root;}
    const evm_common::xh256_t &         get_receipts_root() const {return m_receipts_root;}
    evm_common::xh256_t const & get_state_root() const {
        return m_state_root;
    }
    xbytes_t const&                     get_extra_data() const {return m_extra_data;}
    common::xeth_address_t const&       get_coinbase() const {return m_coinbase;}

 public:  // set APIS
    void        set_gaslimit(uint64_t gaslimit);
    void        set_gasused(uint64_t gasused);
    void        set_baseprice(const evm_common::u256 & price);
    void        set_logBloom(const evm_common::xbloom9_t & bloom);
    void        set_transactions_root(const evm_common::xh256_t & root);
    void        set_receipts_root(const evm_common::xh256_t & root);
    void        set_state_root(evm_common::xh256_t const & root);
    void        set_extra_data(xbytes_t const& _data);
    void        set_coinbase(const common::xeth_address_t & miner);

 private:
    uint8_t                 m_version{0};
    uint64_t                m_gaslimit{0};  // the block gaslimit
    evm_common::u256        m_baseprice;
    uint64_t                m_gasused{0};  // the block gasUsed by all txs
    evm_common::xbloom9_t   m_logBloom;
    evm_common::xh256_t     m_transactions_root;
    evm_common::xh256_t     m_receipts_root;
    evm_common::xh256_t     m_state_root;
    xbytes_t                m_extra_data;
    common::xeth_address_t  m_coinbase;
};

using xeth_header_ptr_t = std::shared_ptr<xeth_header_t>;

// class xeth_block_t {
//  public:
//     xeth_block_t(const xeth_receipts_t & receipts);
//     ~xeth_block_t() = default;

//  public:  // get APIS
//     const xeth_header_t &   get_header() const {return m_header;}
//  public:  // set APIS
//     bool    build_block();

//  public:  // serialize
//     void        streamRLP(evm_common::RLPStream& _s) const;

//  protected:


//  private:
//     xeth_header_t   m_header;
//     xeth_receipts_t m_receipts;
// };


NS_END2
