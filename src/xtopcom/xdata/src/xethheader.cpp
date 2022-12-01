// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xhex.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xethheader.h"
#include "xcommon/xerror/xerror.h"

NS_BEG2(top, data)

void xeth_header_t::set_gaslimit(uint64_t gaslimit) {
    m_gaslimit = gaslimit;
}

void xeth_header_t::set_gasused(uint64_t gasused) {
    m_gasused = gasused;
}
void xeth_header_t::set_baseprice(const evm_common::u256 & price) {
    m_baseprice = price;
}
void xeth_header_t::set_logBloom(const evm_common::xbloom9_t & bloom) {
    m_logBloom = bloom;
}
void xeth_header_t::set_transactions_root(const evm_common::xh256_t & root) {
    m_transactions_root = root;
}
void xeth_header_t::set_receipts_root(const evm_common::xh256_t & root) {
    m_receipts_root = root;
}
void xeth_header_t::set_state_root(evm_common::xh256_t const & root) {
    m_state_root = root;
}
void xeth_header_t::set_extra_data(xbytes_t const& _data) {
    m_extra_data = _data;
}
void xeth_header_t::set_coinbase(const common::xeth_address_t & miner) {
    m_coinbase = miner;
}

xbytes_t xeth_header_t::encodeBytes() const {
    xbytes_t _bytes;
    _bytes.push_back(m_version);
    evm_common::RLPStream _s;
    streamRLP(_s);
    _bytes.insert(_bytes.begin() + 1, _s.out().begin(), _s.out().end());
    return _bytes;
}
void xeth_header_t::decodeBytes(xbytes_t const& _d, std::error_code & ec) {
    assert(!ec);

    if (_d.size() < 2) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xeth_header_t::decodeBytes fail bytes,%zu", _d.size());
        return;
    }

    m_version = (uint8_t)_d.front();
    if (m_version != 0) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xeth_header_t::decodeBytes fail invalid version,%d", m_version);
        return;
    }
    xbytes_t _d2(_d.begin() + 1, _d.end());
    evm_common::RLP _r(_d2);
    decodeRLP(_r, ec);
}

void xeth_header_t::streamRLP(evm_common::RLPStream& _s) const {
    // todo: use different way to serialize for nil block and full block to reduce storage cost.
    _s.appendList(9);
    _s << m_gaslimit;
    _s << m_baseprice;
    _s << m_gasused;
    if (!m_coinbase.is_zero()) {
        _s << m_coinbase.to_bytes();
    } else {
        _s << "";
    }
    if (m_logBloom) {
        _s << m_logBloom.to_bytes();
    } else {
        _s << "";
    }
    if (m_transactions_root) {  // check empty
        _s << m_transactions_root.asBytes();
    } else {
        _s << "";
    }
    if (m_receipts_root) {  // check empty
        _s << m_receipts_root.asBytes();
    } else {
        _s << "";
    }
    if (m_state_root) {  // check empty
        _s << m_state_root.asBytes();
    } else {
        _s << "";
    }
    _s << m_extra_data;
}

void xeth_header_t::decodeRLP(evm_common::RLP const& _r, std::error_code & ec) {
    if ((!_r.isList()) || (_r.itemCount() != 9) ) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xeth_header_t::decodeRLP fail item count,%d", _r.itemCount());
        return;
    }

    int field = 0;
    try
    {
        m_gaslimit = _r[field = 0].toInt<uint64_t>();
        m_baseprice = _r[field = 1].toInt<evm_common::u256>();
        m_gasused = _r[field = 2].toInt<uint64_t>();
        const auto & coinbase_r = _r[field = 3];
        if (!coinbase_r.isEmpty()) {
            xbytes_t _bytes = coinbase_r.toBytes();
            m_coinbase = common::xeth_address_t::build_from(_bytes, ec);
            if (ec) {
                xerror("xeth_header_t::decodeRLP fail miner,%s", top::to_hex(_r[field].toString()).c_str());
                return;
            }
        }
        const auto & logbloom_r = _r[field = 4];
        if (!logbloom_r.isEmpty()) {
            xbytes_t _bytes = logbloom_r.toBytes();
            m_logBloom = evm_common::xbloom9_t::build_from(_bytes, ec);
            if (ec) {
                xerror("xeth_header_t::decodeRLP fail bloom,%s", top::to_hex(_r[field].toString()).c_str());
                return;
            }
        }

        const auto & txroot_r = _r[field = 5];
        if (!txroot_r.isEmpty()) {
            xbytes_t _bytes = txroot_r.toBytes();
            if (_bytes.size() != 32) {
                ec = common::error::xerrc_t::invalid_eth_header;
                xerror("xeth_header_t::decodeRLP fail tx root,%s", top::to_hex(_r[field].toString()).c_str());
                return;
            }
            m_transactions_root = evm_common::xh256_t(_bytes);
        }

        const auto & receiptsroot_r = _r[field = 6];
        if (!receiptsroot_r.isEmpty()) {
            xbytes_t _bytes = receiptsroot_r.toBytes();
            if (_bytes.size() != 32) {
                ec = common::error::xerrc_t::invalid_eth_header;
                xerror("xeth_header_t::decodeRLP fail receipts root,%s", top::to_hex(_r[field].toString()).c_str());
                return;
            }
            m_receipts_root = evm_common::xh256_t(_bytes);
        }

        const auto & stateroot_r = _r[field = 7];
        if (!stateroot_r.isEmpty()) {
            xbytes_t _bytes = stateroot_r.toBytes();
            if (_bytes.size() != 32) {
                ec = common::error::xerrc_t::invalid_eth_header;
                xerror("xeth_header_t::decodeRLP fail state root,%s", top::to_hex(_r[field].toString()).c_str());
                return;
            }
            m_state_root = evm_common::xh256_t(_bytes);
        }

        m_extra_data = _r[field = 8].toBytes();
    }
    catch (...)
    {
        xwarn("xeth_header_t::decodeRLP invalid,field=%d,%s", field, top::to_hex(_r[field].toString()).c_str());
        ec = common::error::xerrc_t::invalid_rlp_stream;
    }
}

std::string xeth_header_t::serialize_to_string() const {
    xbytes_t _bs = encodeBytes();
    return top::to_string(_bs);
}
void xeth_header_t::serialize_from_string(const std::string & bin_data, std::error_code & ec) {
    assert(!ec);

    xbytes_t _bs = top::to_bytes(bin_data);
    decodeBytes(_bs, ec);
}

//============= xeth_block_t ===============
// xeth_block_t::xeth_block_t(const xeth_receipts_t & receipts)
// : m_receipts(receipts) {
// }

// bool xeth_block_t::build_block() {
//     if (m_receipts.empty()) {
//         xassert(false);
//         return false;
//     }

//     evm_common::h256 _receipts_root = xeth_build_t::build_receipts_root(m_receipts);
//     m_header.set_receipts_root(_receipts_root);

//     evm_common::LogBloom _block_logbloom = xeth_build_t::build_block_logsbloom(m_receipts);
//     m_header.set_logBloom(_block_logbloom);

//     uint64_t block_gaslimit = XGET_ONCHAIN_GOVERNANCE_PARAMETER(block_gas_limit);
//     m_header.set_gaslimit(block_gaslimit);

    

//     return true;
// }

NS_END2
