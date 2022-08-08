#pragma once

#include "xevm_common/xeth/xeth_header.h"

NS_BEG3(top, evm_common, heco)

struct xheco_snapshot_t {
    bool init_with_epoch(const eth::xeth_header_t & header);
    h256 digest() const;
    bool apply(const eth::xeth_header_t & header, bool check);
    bool inturn(uint64_t number, xbytes_t validator);
    void print() const;

    uint64_t number{0};
    h256 hash;
    std::set<xbytes_t> validators;
    std::map<uint64_t, xbytes_t> recents;
};

struct xheco_snap_info_t {
    xheco_snap_info_t() = default;
    xheco_snap_info_t(h256 snap_hash_, h256 parent_hash_, bigint number_);

    bytes encode_rlp() const;
    bool decode_rlp(const bytes & input);

    h256 snap_hash;
    h256 parent_hash;
    bigint number;
};

NS_END3
