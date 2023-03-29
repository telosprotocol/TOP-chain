#include <string>
#include "xdata/xethtransaction.h"
#include "xdata/xdatautil.h"
#include "xdata/xerror/xerror.h"
#include "xcommon/xerror/xerror.h"
#include "xcrypto/xckey.h"
#include "xbasic/xhex.h"
#include "xutility/xhash.h"
#include "xbase/xutl.h"
#include "xconfig/xpredefined_configurations.h"
#include "xconfig/xconfig_register.h"
#include "xmetrics/xmetrics.h"

namespace top {
namespace data {

void xeth_accesstuple_t::streamRLP(evm_common::RLPStream& _s) const {
    _s.appendList(2);
    _s << m_addr.to_bytes();
    _s << m_storage_keys;
}

void xeth_accesstuple_t::decodeRLP(evm_common::RLP const& _r, std::error_code & ec) {
    if ( _r.itemCount() != 2 ) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xwarn("xeth_accesstuple_t::decodeRLP fail invalid count=%zu", _r.itemCount());
        return;
    }
    xbytes_t _bytes = _r[0].toBytes();
    if (!_bytes.empty()) {
        m_addr = common::xeth_address_t::build_from(_bytes, ec);
        if (ec) {
            xwarn("xxeth_accesstuple_t::decodeRLP fail to addr.%s", top::to_hex(_bytes).c_str());
            return;
        }
    }
    if (!_r[1].isList()) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xxeth_accesstuple_t::decodeRLP fail keyslist");
        return;
    }
    m_storage_keys = _r[1].toVector<evm_common::xh256_t>();
}

void xeth_accesslist_t::streamRLP(evm_common::RLPStream& _s) const {
    _s.appendList(m_accesstuple.size());
    for (auto & v : m_accesstuple) {
        v.streamRLP(_s);
    }
}

void xeth_accesslist_t::decodeRLP(evm_common::RLP const& _r, std::error_code & ec) {
    if (!_r.isList()) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xeth_accesslist_t::decodeRLP fail list");
        return;
    }
    for (size_t i = 0; i < _r.itemCount(); i++) {
        evm_common::RLP _r_tupple = _r[i];
        xeth_accesstuple_t _v;
        _v.decodeRLP(_r_tupple, ec);
        if (ec) {
            xerror("xeth_accesslist_t::decodeRLP fail tupple decode");
            return;
        }
        m_accesstuple.emplace_back(_v);
    }
}

data::xeth_transaction_t  xeth_transaction_t::build_from(std::string const& rawtx_bin, eth_error & ec) {
    data::xeth_transaction_t _tx;
    xbytes_t _rawtx_bs = top::from_hex(rawtx_bin, ec.error_code);
    if (ec.error_code) {
        return _tx;
    }

    _tx.decodeBytes(_rawtx_bs,  ec);
    if (ec.error_code) {
        return _tx;
    }
    _tx.check_scope(ec);
    return _tx;
}

data::xeth_transaction_t  xeth_transaction_t::build_from(xbytes_t const& rawtx_bs, eth_error & ec) {
    data::xeth_transaction_t _tx;
    _tx.decodeBytes(rawtx_bs,  ec);
    if (ec.error_code) {
        return _tx;
    }
    _tx.check_scope(ec);
    return _tx;
}

xeth_transaction_t::xeth_transaction_t(common::xeth_address_t const& _from, common::xeth_address_t const& _to, xbytes_t const& _data, evm_common::u256 const& _value, evm_common::u256 const& _gas, evm_common::u256 const& _maxGasPrice) {
    m_from = _from;
    m_to = _to;
    m_data = _data;
    m_value = _value;
    m_gas = _gas;
    m_max_fee_per_gas = _maxGasPrice;
}

xbytes_t xeth_transaction_t::encodeBytes(bool includesig) const {
    xbytes_t _bytes;
    _bytes.push_back((uint8_t)m_version);
    evm_common::RLPStream _s;
    switch (m_version)
    {
    case EIP_1559:
        streamRLP_eip1599(includesig, _s);
        break;
    default:
        xassert(false);
        break;
    }
    _bytes.insert(_bytes.begin() + 1, _s.out().begin(), _s.out().end());
    return _bytes;
}

xbytes_t xeth_transaction_t::encodeUnsignHashBytes() const {
    return encodeBytes(false);
}

void xeth_transaction_t::decodeBytes(bool includesig, xbytes_t const& _d, eth_error & ec) {
    m_version = (enum_ethtx_version)_d.front();
    xbytes_t _d2(_d.begin() + 1, _d.end());
    evm_common::RLP _r(_d2);
    switch (m_version)
    {
    case EIP_1559:
        decodeRLP_eip1599(includesig, _r, ec);
        break;
    default:
        ec = eth_error(error::xenum_errc::eth_server_error, "transaction type not supported");
        return;
    }
}

void xeth_transaction_t::decodeBytes(xbytes_t const& _d, std::error_code & ec) {
    eth_error eth_ec;
    decodeBytes(_d, eth_ec);
    ec = eth_ec.error_code;
}

std::string xeth_transaction_t::serialize_to_string() const {
    xbytes_t _bs = encodeBytes();

    return top::to_string(_bs);
}

void xeth_transaction_t::serialize_from_string(const std::string & bin_data, eth_error & ec) {
    m_transaction_hash = utl::xkeccak256_t::digest(bin_data);
    xbytes_t _bs = top::to_bytes(bin_data);
    decodeBytes(_bs, ec);
}

void xeth_transaction_t::streamRLP_eip1599(bool includesig, evm_common::RLPStream& _s) const {
    _s.appendList(includesig ? 12 : 9);
    _s << m_chainid;
    _s << m_nonce;
    _s << m_max_priority_fee_per_gas;
    _s << m_max_fee_per_gas;
    _s << m_gas;
    if (!m_to.is_zero()) {
        _s << m_to.to_bytes();
    } else {
        _s << "";
    }
    _s << m_value;
    _s << m_data;
    m_accesslist.streamRLP(_s);
    if (includesig) {
        _s << m_signV;
        _s << m_signR;
        _s << m_signS;
    }
}

void xeth_transaction_t::decodeRLP_eip1599(bool includesig, evm_common::RLP const& _r, eth_error & ec) {
    size_t itemcount = includesig ? 12 : 9;

    if (_r.itemCount() > itemcount) {
        ec = eth_error(error::xenum_errc::eth_server_error, "rlp: input list has too many elements for types.DynamicFeeTx");
        return;
    } else if (_r.itemCount() < itemcount) {
        ec = eth_error(error::xenum_errc::eth_server_error, "rlp: too few elements for types.DynamicFeeTx");
        return;
    }

    int field = 0;
    try
    {
        if (_r[field = 0].size() > 32) {
            ec = eth_error(error::xenum_errc::eth_server_error, "rlp: input string too long for common.ChainId, decoding into (types.DynamicFeeTx).ChainId");
            return;
        }
        m_chainid = _r[field = 0].toInt<evm_common::u256>();
        if (_r[field = 1].size() > 32) {
            ec = eth_error(error::xenum_errc::eth_server_error, "rlp: input string too long for uint64, decoding into (types.DynamicFeeTx).Nonce");
            return;
        }
        m_nonce = _r[field = 1].toInt<evm_common::u256>();
        if (_r[field = 2].size() > 32) {
            ec = eth_error(error::xenum_errc::eth_server_error, "rlp: input string too long for common.MaxPriorityFeePerGas, decoding into (types.DynamicFeeTx).MaxPriorityFeePerGas");
            return;
        }
        m_max_priority_fee_per_gas = _r[field = 2].toInt<evm_common::u256>();
        if (_r[field = 3].size() > 32) {
            ec = eth_error(error::xenum_errc::eth_server_error, "rlp: input string too long for common.MaxFeePerGas, decoding into (types.DynamicFeeTx).MaxFeePerGas");
            return;
        }
        m_max_fee_per_gas = _r[field = 3].toInt<evm_common::u256>();
        if (_r[4].size() > 32) {
            ec = eth_error(error::xenum_errc::eth_server_error, "rlp: input string too long for common.Gas, decoding into (types.DynamicFeeTx).Gas");
            return;
        }
        m_gas = _r[field = 4].toInt<evm_common::u256>();

        if (_r[5].size() != 20 && _r[5].size() != 0) {
            if (_r[5].size() > 20) {
                ec = eth_error(error::xenum_errc::eth_server_error, "rlp: input string too long for common.Address, decoding into (types.DynamicFeeTx).To");
            } else {
                ec = eth_error(error::xenum_errc::eth_server_error, "rlp: input string too short for common.Address, decoding into (types.DynamicFeeTx).To");
            }
            return;
        }

        if (!_r[5].isEmpty()) {
            xbytes_t _bytes = _r[field = 5].toBytes();
            m_to = common::xeth_address_t::build_from(_bytes, ec.error_code);
            if (ec.error_code) {
                ec = eth_error(error::xenum_errc::eth_server_error, "rlp: input string invalid for common.Address, decoding into (types.DynamicFeeTx).To");
                return;
            }
        } else {
            m_to = common::xeth_address_t::zero();
        }
        if (_r[6].size() > 32) {
            ec = eth_error(error::xenum_errc::eth_server_error, "rlp: input string too long for common.value, decoding into (types.DynamicFeeTx).value");
            return;
        }
        m_value = _r[field = 6].toInt<evm_common::u256>();
        m_data = _r[field = 7].toBytes();
        evm_common::RLP _r_al = _r[field = 8];
        m_accesslist.decodeRLP(_r_al, ec.error_code);
        if (ec.error_code) {
            ec = eth_error(error::xenum_errc::eth_server_error, "rlp: input string invalid for accesslist, decoding into (types.DynamicFeeTx).Accesslist");
            return;
        }
        if (includesig) {
            if (_r[9].size() > 32) {
                ec = eth_error(error::xenum_errc::eth_server_error, "rlp: input string too long for common.signV, decoding into (types.DynamicFeeTx).signV");
                return;
            }
            m_signV = _r[field = 9].toInt<evm_common::u256>();
            if (_r[10].size() > 32) {
                ec = eth_error(error::xenum_errc::eth_server_error, "rlp: input string too long for common.signR, decoding into (types.DynamicFeeTx).signR");
                return;
            }
            m_signR = _r[field = 10].toInt<evm_common::u256>();
            if (_r[11].size() > 32) {
                ec = eth_error(error::xenum_errc::eth_server_error, "rlp: input string too long for common.signS, decoding into (types.DynamicFeeTx).signS");
                return;
            }
            m_signS = _r[field = 11].toInt<evm_common::u256>();
        }

        if (m_data.empty() && m_to.is_zero()) {
            ec = eth_error(error::xenum_errc::eth_server_error, "rlp: input data and to all empty, not valid tx");
            return;
        }
    }
    catch (...)
    {
        xwarn("xeth_transaction_t::decodeRLP_eip1599 invalid,field=%d,%s", field, top::to_hex(_r[field].toString()).c_str());
        ec = eth_error(error::xenum_errc::eth_server_error, "rlp: input string invalid");
    }
}

void xeth_transaction_t::check_scope(eth_error & ec) const {
    if (m_chainid != XGET_CONFIG(chain_id)) {
        ec = eth_error(error::xenum_errc::eth_server_error, "invalid sender");
        return;
    }
    if (m_gas > XGET_ONCHAIN_GOVERNANCE_PARAMETER(block_gas_limit)) {
        ec = eth_error(error::xenum_errc::eth_server_error, "exceeds block gas limit");
        return;
    }
    if (0 == m_gas) {
        ec = eth_error(error::xenum_errc::eth_server_error, "intrinsic gas too low");
        return;
    }
    if (0 == m_max_fee_per_gas) {
        ec = eth_error(error::xenum_errc::eth_server_error, "transaction underpriced");
        return;
    }
    if (m_max_priority_fee_per_gas > m_max_fee_per_gas) {
        ec = eth_error(error::xenum_errc::eth_server_error, "max priority fee per gas higher than max fee per ga");
        return;
    }
}

uint256_t xeth_transaction_t::get_tx_hash() const {
    if (m_transaction_hash.empty()) {
        xbytes_t _bytes = encodeBytes();
        std::string _str = top::to_string(_bytes);
        // std::cout << "tx_hash_bytes=" << top::to_hex(_str) << std::endl;
        m_transaction_hash = utl::xkeccak256_t::digest(_str);
    }

    return m_transaction_hash;
}

common::xeth_address_t xeth_transaction_t::get_from() const {
    if (m_from.is_zero()) {
        xbytes_t _bytes = encodeUnsignHashBytes();
        std::string _str = top::to_string(_bytes);
        // std::cout << "unsign_bytes=" << top::to_hex(_bytes) << std::endl;
        uint256_t m_unsign_hash = utl::xkeccak256_t::digest(_str);
        XMETRICS_GAUGE(metrics::ethtx_get_from, 1);
        // std::cout << "unsign_hash=" << to_hex_str(m_unsign_hash) << std::endl;

        char szSign[65] = {0};
        memcpy(szSign, (char *)&m_signV, 1);
        memcpy(szSign + 1, (char *)m_signR.data(), m_signR.asBytes().size());
        memcpy(szSign + 33, (char *)m_signS.data(), m_signS.asBytes().size());

        top::utl::xecdsasig_t sig((uint8_t *)szSign);
        uint8_t szOutput[65] = {0};
        top::utl::xsecp256k1_t::get_publickey_from_signature(sig, m_unsign_hash, szOutput);
        top::utl::xecpubkey_t pubkey(szOutput);
        std::string _addr = pubkey.to_raw_eth_address();
        xbytes_t addrbytes = top::to_bytes(_addr);
        std::error_code ec;
        common::xeth_address_t _from = common::xeth_address_t::build_from(addrbytes, ec);
        if (ec) {
            xwarn("xeth_transaction_t::get_from fail addr.%s,size=%zu", top::to_hex(addrbytes).c_str(), addrbytes.size());
        } else {
            m_from = _from;
        }
        // std::cout << "address=" << _from.to_hex_string() << std::endl;
    }
    return m_from;
}

std::string xeth_transaction_t::dump() const
{
    char local_param_buf[256];
    auto txhash = get_tx_hash();
    xbytes_t txhash_bs = top::to_bytes(txhash);
    xprintf(local_param_buf, sizeof(local_param_buf),"{xeth_transaction_t:hash=%s,from=%s,to=%s}",
        top::to_hex_prefixed(txhash_bs).c_str(), get_from().to_hex_string().c_str(),  m_to.to_hex_string().c_str());
    return std::string(local_param_buf);
}

}  // namespace data
}  // namespace top
