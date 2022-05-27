// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbasic/xbyte_buffer.h"
#include "xdata/xethreceipt.h"
#include "xdata/xlightunit_info.h"
#include "xevm_common/common.h"
#include "xevm_common/xfixed_hash.h"
#include "xevm_common/rlp.h"
#include "xevm_common/xbloom9.h"

NS_BEG2(top, data)

enum enum_eth_header_format {
    ETH_HEADER_fORMAT_NORMAL = 0,
    ETH_HEADER_fORMAT_SIMPLE = 1,
};

class xeth_header_t {
 public:
    xeth_header_t() = default;
    ~xeth_header_t() = default;

 public:  // serialize
    void    streamRLP(evm_common::RLPStream& _s) const;
    void    decodeRLP(evm_common::RLP const& _r, std::error_code & ec);

 public:  // get APIS
    enum_eth_header_format get_format() const {return m_format;}
    uint64_t    get_gaslimit() const {return m_gaslimit;}
    uint64_t    get_gasused() const {return m_gasused;}
    const evm_common::u256 &            get_baseprice() const {return m_baseprice;}
    const evm_common::xbloom9_t &       get_logBloom() const {return m_logBloom;}
    const evm_common::xh256_t &         get_transactions_root() const {return m_transactions_root;}
    const evm_common::xh256_t &         get_receipts_root() const {return m_receipts_root;}
    const evm_common::xh256_t &         get_state_root() const {return m_state_root;}

 public:  // set APIS
    void        set_format(enum_eth_header_format format);
    void        set_gaslimit(uint64_t gaslimit);
    void        set_gasused(uint64_t gasused);
    void        set_baseprice(const evm_common::u256 & price);
    void        set_logBloom(const evm_common::xbloom9_t & bloom);
    void        set_transactions_root(const evm_common::xh256_t & root);
    void        set_receipts_root(const evm_common::xh256_t & root);
    void        set_state_root(const evm_common::xh256_t & root);

 private:
    enum_eth_header_format  m_format{ETH_HEADER_fORMAT_NORMAL};
    uint64_t                m_gaslimit{0};  // the block gaslimit
    evm_common::u256        m_baseprice;
    uint64_t                m_gasused{0};  // the block gasUsed by all txs
    evm_common::xbloom9_t   m_logBloom;
    evm_common::xh256_t     m_transactions_root;
    evm_common::xh256_t     m_receipts_root;
    evm_common::xh256_t     m_state_root;
    xbytes_t                m_extra_data;
};

class xeth_header_builder {
public:
    static const std::string build(uint64_t clock, enum_eth_header_format format, uint64_t gas_limit, std::vector<data::xlightunit_tx_info_ptr_t> txs_info = {});
    static bool string_to_eth_header(const std::string & eth_header_str, xeth_header_t & eth_header);
};

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
