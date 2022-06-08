// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xhex.h"
#include "xdata/xethreceipt.h"
#include "xcommon/xerror/xerror.h"

NS_BEG2(top, data)

void xeth_store_receipt_t::streamRLP(evm_common::RLPStream& _s) const {
    _s.appendList(6);
    _s << (uint8_t)m_tx_status;
    _s << m_cumulative_gas_used;
    _s.appendList(m_logs.size());
    for (auto & log : m_logs) {
        log.streamRLP(_s);
    }
    _s << m_gas_price;
    _s << m_gas_used;
    _s << m_contract_address.to_bytes();
}

void xeth_store_receipt_t::decodeRLP(evm_common::RLP const& _r, std::error_code & ec) {
    if (!_r.isList() || _r.itemCount() != 6) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xeth_store_receipt_t::decodeRLP fail item count,%d", _r.itemCount());
        return;
    }

    int field = 0;
    try {
        m_tx_status = (enum_ethreceipt_status)(uint8_t)_r[field = 0];
        m_cumulative_gas_used = _r[field = 1].toInt<uint64_t>();

        field = 2;
        if (!_r[field].isList()) {
            ec = common::error::xerrc_t::invalid_rlp_stream;
            xerror("xeth_store_receipt_t::decodeRLP fail loglist");
            return;
        }
        for (auto const& i : _r[field]) {
            evm_common::RLP _rlog(i);
            evm_common::xevm_log_t _log;
            _log.decodeRLP(_rlog, ec);
            if (ec) {
                xerror("xeth_store_receipt_t::decodeRLP fail log decode");
                return;
            }
            m_logs.emplace_back(_log);
        }
        m_gas_price = (evm_common::u256)_r[field = 3];
        m_gas_used = _r[field = 4].toInt<uint64_t>();

        xbytes_t address_bytes = _r[field = 5].toBytes();
        m_contract_address = common::xeth_address_t::build_from(address_bytes, ec);
        if (ec) {
            xerror("xeth_store_receipt_t::decodeRLP fail address decode");
            return;
        }
    }
    catch (...)
    {
        xwarn("xeth_store_receipt_t::decodeRLP invalid,field=%d,%s", field, top::to_hex(_r[field].toString()).c_str());
        ec = common::error::xerrc_t::invalid_rlp_stream;
    }
    return;
}

xbytes_t xeth_store_receipt_t::encodeBytes() const {
    xbytes_t _bytes;
    _bytes.push_back(m_version);
    evm_common::RLPStream _s;
    streamRLP(_s);
    _bytes.insert(_bytes.begin() + 1, _s.out().begin(), _s.out().end());
    return _bytes;
}

void xeth_store_receipt_t::decodeBytes(xbytes_t const& _d, std::error_code & ec) {
    if (_d.size() < 2) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xeth_store_receipt_t::decodeBytes fail bytes,%zu", _d.size());
        return;
    }

    m_version = (uint8_t)_d.front();
    if (m_version != 0) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xeth_store_receipt_t::decodeBytes fail invalid version,%d", m_version);
        return;
    }

    xbytes_t _d2(_d.begin() + 1, _d.end());
    evm_common::RLP _r(_d2);
    decodeRLP(_r, ec);
}

evm_common::xbloom9_t xeth_store_receipt_t::bloom() const {
    evm_common::xbloom9_t  logsbloom;
    for (auto & log : m_logs) {
        evm_common::xbloom9_t logbloom = log.bloom();
        logsbloom |= logbloom;
    }
    return logsbloom;
}


xeth_receipt_t::xeth_receipt_t(enum_ethtx_version _version, enum_ethreceipt_status _status, uint64_t _gasused, evm_common::xevm_logs_t const & _logs)
: m_tx_version_type(_version), m_tx_status(_status), m_cumulative_gas_used(_gasused), m_logs(_logs) {

}

void xeth_receipt_t::streamRLP(evm_common::RLPStream& _s) const {
    _s.appendList(4);
    _s << (uint8_t)m_tx_status;
    _s << m_cumulative_gas_used;
    _s << m_logsBloom.get_data();
    _s.appendList(m_logs.size());
    for (auto & log : m_logs) {
        log.streamRLP(_s);
    }
}

void xeth_receipt_t::decodeRLP(evm_common::RLP const& _r, std::error_code & ec) {
    if (!_r.isList() || _r.itemCount() != 4) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xeth_receipt_t::decodeRLP fail item count,%d", _r.itemCount());
        return;
    }

    int field = 0;
    try {
        m_tx_status = (enum_ethreceipt_status)(uint8_t)_r[field = 0];
        m_cumulative_gas_used = _r[field = 1].toInt<uint64_t>();
        xbytes_t logbloom_bytes = _r[field = 2].toBytes();
        m_logsBloom = evm_common::xbloom9_t(logbloom_bytes);

        field = 3;
        if (!_r[field].isList()) {
            ec = common::error::xerrc_t::invalid_rlp_stream;
            xerror("xeth_receipt_t::decodeRLP fail loglist");
            return;
        }
        for (auto const& i : _r[field]) {
            evm_common::RLP _rlog(i);
            evm_common::xevm_log_t _log;
            _log.decodeRLP(_rlog, ec);
            if (ec) {
                xerror("xeth_receipt_t::decodeRLP fail log decode");
                return;
            }
            m_logs.emplace_back(_log);
        }
    }
    catch (...)
    {
        xwarn("xeth_receipt_t::decodeRLP invalid,field=%d,%s", field, top::to_hex(_r[field].toString()).c_str());
        ec = common::error::xerrc_t::invalid_rlp_stream;
    }
    return;
}

xbytes_t xeth_receipt_t::encodeBytes() const {
    xbytes_t _bytes;
    if (m_tx_version_type != EIP_LEGACY) {
        _bytes.push_back((uint8_t)m_tx_version_type);
    }
    evm_common::RLPStream _s;
    streamRLP(_s);
    if (m_tx_version_type != EIP_LEGACY) {
        _bytes.insert(_bytes.begin() + 1, _s.out().begin(), _s.out().end());
    } else {
        _bytes = _s.out();
    }
    return _bytes;
}

void xeth_receipt_t::decodeBytes(xbytes_t const& _d, std::error_code & ec) {    
    if (_d.size() < 2) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xeth_receipt_t::decodeBytes fail bytes,%zu", _d.size());
        return;
    }

    enum_ethtx_version tx_version = (enum_ethtx_version)(uint8_t)_d.front();
    if (tx_version != EIP_1559) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xeth_receipt_t::decodeBytes fail tx version,%d", tx_version);
        return;
    }

    xbytes_t _d2(_d.begin() + 1, _d.end());

    evm_common::RLP _r(_d2);
    decodeRLP(_r, ec);
}

void xeth_receipt_t::create_bloom() {
    if (m_logs.empty()) {
        return;
    }
    evm_common::xbloom9_t  logsbloom;
    for (auto & log : m_logs) {
        evm_common::xbloom9_t logbloom = log.bloom();
        logsbloom |= logbloom;
    }
    m_logsBloom = logsbloom;
}


xeth_local_receipt_t::xeth_local_receipt_t(enum_ethtx_version _version, enum_ethreceipt_status _status, uint64_t _gasused, evm_common::xevm_logs_t const & _logs)
: xeth_receipt_t(_version, _status, _gasused, _logs) {

}


void xeth_local_receipt_t::set_block_hash(std::string const& blockhash) {
    m_block_hash = blockhash;
}
void xeth_local_receipt_t::set_block_number(uint64_t blocknumber) {
    m_block_number = blocknumber;
}
void xeth_local_receipt_t::set_transaction_index(uint32_t index) {
    m_transaction_index = index;
}
void xeth_local_receipt_t::set_tx_hash(std::string const& txhash) {
    m_tx_hash = txhash;
}
void xeth_local_receipt_t::set_from_address(common::xeth_address_t const& address) {
    m_from_address = address;
}
void xeth_local_receipt_t::set_to_address(common::xeth_address_t const& address) {
    m_to_address = address;
}
void xeth_local_receipt_t::set_contract_address(common::xeth_address_t const& address) {
    m_contract_address = address;
}
void xeth_local_receipt_t::set_used_gas(uint64_t used_gas) {
    m_used_gas = used_gas;
}
NS_END2
