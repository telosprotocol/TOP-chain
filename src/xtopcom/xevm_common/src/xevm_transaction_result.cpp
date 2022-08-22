// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/xevm_transaction_result.h"
#include "xcommon/xerror/xerror.h"

NS_BEG2(top, evm_common)

xevm_log_t::xevm_log_t(common::xeth_address_t const& _address, xh256s_t const& _topics, xbytes_t const& _data) : address(_address), topics(_topics), data(_data) {
}

xevm_log_t::xevm_log_t(common::xeth_address_t const & address, xh256s_t topics) : xevm_log_t{address, std::move(topics), {}} {
}

// TODO(jimmy) remove xstream later
int32_t xevm_log_t::do_write(base::xstream_t & stream) {
    const int32_t begin_size = stream.size();
    stream.write_compact_var(address.to_string());
    uint32_t topics_num = (uint32_t)topics.size();
    stream.write_compact_var(topics_num);
    for (uint32_t i = 0; i < topics_num; i++) {
        std::string str = top::to_string(topics[i].asBytes());
        stream.write_compact_var(str);
    }
    stream.write_compact_var(top::to_string(data));
    return (stream.size() - begin_size);
}

int32_t xevm_log_t::do_read(base::xstream_t & stream) {
    const int32_t begin_size = stream.size();
    std::string addr;
    stream.read_compact_var(addr);
    std::error_code ec;
    address = common::xeth_address_t::build_from(top::to_bytes(addr), ec);
    if (ec) {
        xerror("xevm_log_t::do_read fail-build from addr");
        return -1;
    }
    uint32_t topics_num;
    stream.read_compact_var(topics_num);
    for (uint32_t j = 0; j < topics_num; j++) {
        std::string topic_str;
        stream.read_compact_var(topic_str);
        xh256_t topic(top::to_bytes(topic_str));
        topics.push_back(topic);
    }
    std::string data_str;
    stream.read_compact_var(data_str);
    data = top::to_bytes(data_str);
    return (begin_size - stream.size());
}

void xevm_log_t::streamRLP(RLPStream& _s) const {
    _s.appendList(3);
    _s << address.to_bytes();
    _s << topics;
    _s << data;
}

void xevm_log_t::decodeRLP(RLP const& _r, std::error_code & ec) {
    if (_r.itemCount() != 3) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xevm_log_t::decodeRLP fail item count");
        return;
    }
    xbytes_t address_bytes = _r[0].toBytes();
    address = common::xeth_address_t::build_from(address_bytes, ec);
    if (ec) {
        return;
    }
    topics = _r[1].toVector<xh256_t>();
    data = _r[2].toBytes();
    return;
}

xbloom9_t xevm_log_t::bloom() const {
    xbloom9_t ret;
    ret.add(address.to_bytes());
    for (auto & t: topics) {
        ret.add(t.asBytes());
    }
    return ret;
}


NS_END2
