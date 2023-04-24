// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xtop_log.h"

#include "xbasic/xfixed_hash.h"
#include "xcommon/xbloom9.h"
#include "xcommon/xerror/xerror.h"

NS_BEG2(top, common)

xtop_log_t::xtop_log_t(xaccount_address_t const & _address, xh256s_t const & _topics, xbytes_t const & _data) : address(_address), topics(_topics), data(_data) {
}

void xtop_log_t::streamRLP(evm_common::RLPStream & _s) const {
    _s.appendList(3);
    _s << address.to_string();
    _s << topics;
    _s << data;
}

void xtop_log_t::decodeRLP(evm_common::RLP const & _r, std::error_code & ec) {
    assert(!ec);
    if (_r.itemCount() != 3) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xtop_log_t::decodeRLP fail item count");
        return;
    }
    std::string address_str = _r[0].toString();
    address = xaccount_address_t::build_from(address_str, ec);
    if (ec) {
        return;
    }
    topics = _r[1].toVector<xh256_t>();
    data = _r[2].toBytes();
    return;
}

evm_common::xbloom9_t xtop_log_t::bloom() const {
    evm_common::xbloom9_t ret;
    ret.add(to_bytes(address.to_string()));
    for (auto & t : topics) {
        ret.add(t.asBytes());
    }
    return ret;
}

NS_END2
