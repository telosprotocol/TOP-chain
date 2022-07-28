// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xnative_contract_address.h"
#include "xevm_common/xeth/xeth_header.h"
#include "xevm_contract_runtime/xevm_sys_crosschain_contract_face.h"
#include "xevm_common/xcrosschain/xheco_snapshot.h"

NS_BEG4(top, contract_runtime, evm, sys_contract)

// TODO: delete
using evm_common::heco::snapshot;
using evm_common::heco::xheco_snap_info_t;

class xtop_evm_heco_client_contract : public xtop_evm_crosschain_syscontract_face<xtop_evm_heco_client_contract> {
public:
    xtop_evm_heco_client_contract();
    ~xtop_evm_heco_client_contract() override = default;

    bool init(const xbytes_t & rlp_bytes);
    bool sync(const xbytes_t & rlp_bytes);
    bool is_known(const u256 height, const xbytes_t & hash_bytes) const;
    bool is_confirmed(const u256 height, const xbytes_t & hash_bytes) const;
    bigint get_height() const;
    void reset();

private:
    bool verify(const xeth_header_t & prev_header, const xeth_header_t & new_header, snapshot & snap) const;
    bool record(const xeth_header_t & header, const snapshot & snap);
    bool rebuild(const xeth_header_t & header, const xheco_snap_info_t & last_info, const xheco_snap_info_t & cur_info);
    void release(const bigint number);
    // last hash @160
    h256 get_last_hash() const;
    bool set_last_hash(const h256 hash);
    // effective hashes @161
    h256 get_effective_hash(const bigint height) const;
    bool set_effective_hash(const bigint height, const h256 hash);
    bool remove_effective_hash(const bigint height);
    // all hashes @162
    std::set<h256> get_hashes(const bigint height) const;
    bool set_hashes(const bigint height, const std::set<h256> & hashes);
    bool remove_hashes(const bigint height);
    // all headers @163
    bool get_header(const h256 hash, xeth_header_t & header) const;
    bool set_header(const h256 hash, const xeth_header_t & header);
    bool remove_header(const h256 hash);
    // headers info @164
    bool get_snap_info(const h256 hash, xheco_snap_info_t & snap_info) const;
    bool set_snap_info(const h256 hash, const xheco_snap_info_t & snap_info);
    bool remove_snap_info(const h256 hash);
};
using xevm_heco_client_contract_t = xtop_evm_heco_client_contract;

NS_END4
