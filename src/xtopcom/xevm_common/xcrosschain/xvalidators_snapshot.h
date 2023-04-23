#pragma once

#include "xevm_common/xcrosschain/xeth_header.h"

NS_BEG2(top, evm_common)

struct xvalidators_snapshot_t {
    bool init_with_epoch(xeth_header_t const & header);
    bool init_with_double_epoch(xeth_header_t const & header1, const xeth_header_t & header2);
    h256 digest() const;
    bool apply(xeth_header_t const & header, bool check);
    bool apply_with_chainid(xeth_header_t const & header, const bigint & chainid, bool check_inturn);
    bool inturn(uint64_t num, common::xeth_address_t const & validator, bool use_old) const;
    void print() const;

    uint64_t number{0};
    h256 hash;
    std::set<common::xeth_address_t> validators;
    std::set<common::xeth_address_t> last_validators;
    std::map<uint64_t, common::xeth_address_t> recents;
};

struct xvalidators_snap_info_t {
    xvalidators_snap_info_t() = default;
    xvalidators_snap_info_t(h256 const & snap_hash, h256 const & parent_hash, uint64_t number);

    xbytes_t encode_rlp() const;
    bool decode_rlp(xbytes_t const & input);

    h256 snap_hash;
    h256 parent_hash;
    bigint number;
};

NS_END2
