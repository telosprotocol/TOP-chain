#pragma once

#include "xevm_common/xcrosschain/xeth_header.h"

NS_BEG2(top, evm_common)

struct xvalidators_snapshot_t {
    bool init_with_epoch(const xeth_header_t & header);
    h256 digest() const;
    bool apply(const xeth_header_t & header, bool check);
    bool apply_with_chainid(const xeth_header_t & header, const bigint chainid, bool check_inturn);
    bool inturn(uint64_t number, xbytes_t validator);
    void print() const;

    uint64_t number{0};
    h256 hash;
    std::set<xbytes_t> validators;
    std::map<uint64_t, xbytes_t> recents;
};

struct xvalidators_snap_info_t {
    xvalidators_snap_info_t() = default;
    xvalidators_snap_info_t(h256 snap_hash_, h256 parent_hash_, bigint number_);

    bytes encode_rlp() const;
    bool decode_rlp(const bytes & input);

    h256 snap_hash;
    h256 parent_hash;
    bigint number;
};

NS_END2
