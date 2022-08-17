// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xnative_contract_address.h"
#include "xevm_common/xcrosschain/xeth_header.h"
#include "xevm_common/xcrosschain/xvalidators_snapshot.h"
#include "xevm_contract_runtime/xevm_sys_crosschain_contract_face.h"

NS_BEG4(top, contract_runtime, evm, sys_contract)

using evm_common::xvalidators_snapshot_t;
using evm_common::xvalidators_snap_info_t;

class xtop_evm_bsc_client_contract : public xtop_evm_crosschain_syscontract_face<xtop_evm_bsc_client_contract> {
public:
    xtop_evm_bsc_client_contract() = default;
    ~xtop_evm_bsc_client_contract() override = default;

    bool init(const xbytes_t & rlp_bytes, state_ptr state);
    bool sync(const xbytes_t & rlp_bytes, state_ptr state);
    bool is_known(const u256 height, const xbytes_t & hash_bytes, state_ptr state) const;
    bool is_confirmed(const u256 height, const xbytes_t & hash_bytes, state_ptr state) const;
    bigint get_height(state_ptr state) const;
    xheader_status_t query_height(const u256 height, state_ptr state) const;
    bool reset(state_ptr state);
    bool disable_reset(state_ptr state);

private:
    bool verify(const xeth_header_t & prev_header, const xeth_header_t & new_header, xvalidators_snapshot_t & snap, state_ptr state) const;
    bool record(const xeth_header_t & header, const xvalidators_snapshot_t & snap, state_ptr state);
    bool rebuild(const xeth_header_t & header, const xvalidators_snap_info_t & last_info, const xvalidators_snap_info_t & cur_info, state_ptr state);
    void release(const bigint number, state_ptr state);
    // last hash @160
    h256 get_last_hash(state_ptr state) const;
    bool set_last_hash(const h256 hash, state_ptr state);
    // effective hashes @161
    h256 get_effective_hash(const bigint height, state_ptr state) const;
    bool set_effective_hash(const bigint height, const h256 hash, state_ptr state);
    bool remove_effective_hash(const bigint height, state_ptr state);
    // all hashes @162
    std::set<h256> get_hashes(const bigint height, state_ptr state) const;
    bool set_hashes(const bigint height, const std::set<h256> & hashes, state_ptr state);
    bool remove_hashes(const bigint height, state_ptr state);
    // all headers @163
    bool get_header(const h256 hash, xeth_header_t & header, state_ptr state) const;
    bool set_header(const h256 hash, const xeth_header_t & header, state_ptr state);
    bool remove_header(const h256 hash, state_ptr state);
    // headers info @164
    bool get_snap_info(const h256 hash, xvalidators_snap_info_t & snap_info, state_ptr state) const;
    bool set_snap_info(const h256 hash, const xvalidators_snap_info_t & snap_info, state_ptr state);
    bool remove_snap_info(const h256 hash, state_ptr state);
    // flag @165
    int get_flag(state_ptr state) const;
    bool set_flag(state_ptr state);
};
using xevm_bsc_client_contract_t = xtop_evm_bsc_client_contract;

NS_END4
