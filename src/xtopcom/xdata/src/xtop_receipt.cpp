// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xtop_receipt.h"

#include "xbasic/xhex.h"
#include "xcommon/xerror/xerror.h"

#include <string>

NS_BEG2(top, data)
void xtop_store_receipt_t::streamRLP(evm_common::RLPStream & _s) const {
    _s.appendList(1);
    _s.appendList(m_logs.size());
    for (auto & log : m_logs) {
        log.streamRLP(_s);
    }
}

void xtop_store_receipt_t::decodeRLP(evm_common::RLP const & _r, std::error_code & ec) {
    if (false == _r.isList()) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xtop_store_receipt_t::decodeRLP fail item count,%d", _r.itemCount());
        return;
    }
    int field = 0;
    try {
         if (false == _r[field].isList()) {
            ec = common::error::xerrc_t::invalid_rlp_stream;
            xerror("xeth_store_receipt_t::decodeRLP fail loglist");
            return;
        }
        for (auto const & i : _r[field]) {
            evm_common::RLP _rlog(i);
            common::xtop_log_t _log;
            _log.decodeRLP(_rlog, ec);
            if (ec) {
                xerror("xtop_store_receipt_t::decodeRLP fail log decode");
                return;
            }
            m_logs.emplace_back(_log);
        }
    } catch (...) {
        xwarn("xtop_store_receipt_t::decodeRLP invalid", 0, top::to_hex(_r[0].toString()).c_str());
        ec = common::error::xerrc_t::invalid_rlp_stream;
    }
    return;
}

xbytes_t xtop_store_receipt_t::encodeBytes() const {
    xbytes_t _bytes;
    evm_common::RLPStream _s;
    streamRLP(_s);
    return _s.out();
}

void xtop_store_receipt_t::decodeBytes(xbytes_t const & _d, std::error_code & ec) {
    if (_d.size() < 1) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xtop_store_receipt_t::decodeBytes fail bytes,%zu", _d.size());
        return;
    }
    evm_common::RLP _r(_d);
    decodeRLP(_r, ec);
}

void xtop_store_receipt_t::create_bloom() {
    evm_common::xbloom9_t logsbloom;
    for (auto & log : m_logs) {
        evm_common::xbloom9_t logbloom = log.bloom();
        logsbloom |= logbloom;
    }
    m_logsBloom = logsbloom;
}

NS_END2