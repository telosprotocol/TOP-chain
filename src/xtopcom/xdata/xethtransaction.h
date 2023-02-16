#pragma once

#include <string>
#include "xbase/xrefcount.h"
#include "xevm_common/common.h"
#include "xevm_common/xfixed_hash.h"
#include "xevm_common/rlp.h"
#include "xcommon/xeth_address.h"

namespace top {
namespace data {

enum enum_ethtx_version {
    EIP_LEGACY = 0,  // not support
    EIP_2930 = 1, // not support
    EIP_1559 = 2,
};

static std::string strNull = "";

struct eth_error
{
    eth_error() = default;
    eth_error(std::error_code ec, std::string const& em)
    : error_code(ec), error_message(em) {}
    std::error_code error_code;
    std::string  error_message;
};

class xeth_accesstuple_t {
 public:
    void    streamRLP(evm_common::RLPStream& _s) const;
    void    decodeRLP(evm_common::RLP const& _r, std::error_code & ec);

 private:
    common::xeth_address_t  m_addr;
    evm_common::xh256s_t    m_storage_keys;
};

class xeth_accesslist_t {
 public:
    void    streamRLP(evm_common::RLPStream& _s) const;
    void    decodeRLP(evm_common::RLP const& _r, std::error_code & ec);
    size_t  capacity() const {return m_accesstuple.capacity();}

 private:
    std::vector<xeth_accesstuple_t>  m_accesstuple;
};

// xeth_transaction_t is eth format transaction
class xeth_transaction_t {
 public:
    static data::xeth_transaction_t  build_from(std::string const& rawtx_bin, eth_error & ec);
    static data::xeth_transaction_t  build_from(xbytes_t const& rawtx_bs, eth_error & ec);
 public:
    xeth_transaction_t() = default;
    xeth_transaction_t(common::xeth_address_t const& _from, common::xeth_address_t const& _to, xbytes_t const& _data, evm_common::u256 const& _value, evm_common::u256 const& _gas, evm_common::u256 const& _maxGasPrice);

 public:
    xbytes_t    encodeBytes() const {return encodeBytes(true);}
    void        decodeBytes(xbytes_t const& _d, eth_error & ec) {return decodeBytes(true, _d, ec);}
    void        decodeBytes(xbytes_t const& _d, std::error_code & ec);
    xbytes_t    encodeUnsignHashBytes() const;
    std::string serialize_to_string() const;
    void        serialize_from_string(const std::string & bin_data, eth_error & ec);

 public:
    enum_ethtx_version      get_tx_version() const {return m_version;}
    uint256_t               get_tx_hash() const;
    common::xeth_address_t  get_from() const;
    std::string             dump() const;

    evm_common::u256 const&    get_chainid() const { return m_chainid; }
    evm_common::u256 const&    get_nonce() const { return m_nonce; }
    evm_common::u256 const&    get_max_priority_fee_per_gas() const { return m_max_priority_fee_per_gas; }
    evm_common::u256 const&    get_max_fee_per_gas() const { return m_max_fee_per_gas; }
    evm_common::u256 const&    get_gas() const { return m_gas; }
    common::xeth_address_t const&   get_to() const { return m_to; }
    evm_common::u256 const&    get_value() const { return m_value; }
    xbytes_t const&            get_data() const { return m_data; }
    evm_common::u256 const&    get_signV() const { return m_signV; }
    evm_common::h256 const&    get_signR() const { return m_signR; }
    evm_common::h256 const&    get_signS() const { return m_signS; }
    const xeth_accesslist_t&   get_accesslist() const { return m_accesslist; }

 public:
    void    set_tx_version(enum_ethtx_version version) {m_version = version;}
    void    set_chainid(evm_common::u256 const& value) {m_chainid = value;}
    void    set_nonce(evm_common::u256 const& value) {m_nonce = value;}
    void    set_max_priority_fee_per_gas(evm_common::u256 const& value) {m_max_priority_fee_per_gas = value;}
    void    set_max_fee_per_gas(evm_common::u256 const& value) {m_max_fee_per_gas = value;}
    void    set_gas(evm_common::u256 const& value) {m_gas = value;}
    void    set_to(common::xeth_address_t const& value) {m_to = value;}
    void    set_value(evm_common::u256 const& value) {m_value = value;}
    void    set_data(xbytes_t const& value) {m_data = value;}
    void    set_signV(evm_common::u256 const& value) {m_signV = value;}
    void    set_signR(evm_common::h256 const& value) {m_signR = value;}
    void    set_signS(evm_common::h256 const& value) {m_signS = value;}

 protected:
    void        streamRLP_eip1599(bool includesig, evm_common::RLPStream& _s) const;
    void        decodeRLP_eip1599(bool includesig, evm_common::RLP const& _r, eth_error & ec);
    xbytes_t    encodeBytes(bool includesig) const;
    void        decodeBytes(bool includesig, xbytes_t const& _d, eth_error & ec);
    void        check_scope(eth_error & ec) const;


 private:
    enum_ethtx_version  m_version{EIP_1559};
    evm_common::u256    m_chainid;
    evm_common::u256    m_nonce;
    evm_common::u256    m_max_priority_fee_per_gas;
    evm_common::u256    m_max_fee_per_gas;
    evm_common::u256    m_gas;
    common::xeth_address_t  m_to;
    evm_common::u256    m_value;
    xbytes_t            m_data;
    xeth_accesslist_t   m_accesslist;
    evm_common::u256    m_signV;
    evm_common::xh256_t m_signR;
    evm_common::xh256_t m_signS;

 private:
    // cache members
    mutable common::xeth_address_t  m_from;
    mutable uint256_t               m_transaction_hash;
};

using xeth_transactions_t = std::vector<xeth_transaction_t>;

}  // namespace data
}  // namespace top
