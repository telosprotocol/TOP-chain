// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xnative_contract_address.h"
#include "xevm_common/xeth/xeth_header.h"
#include "xevm_contract_runtime/xevm_sys_contract_face.h"

NS_BEG4(top, contract_runtime, evm, sys_contract)

class xtop_evm_bsc_client_contract : public xtop_evm_crosschain_syscontract_face<xtop_evm_bsc_client_contract> {
public:
    xtop_evm_bsc_client_contract();
    ~xtop_evm_bsc_client_contract() override = default;

    bool init(const xbytes_t & rlp_bytes);
    bool sync(const xbytes_t & rlp_bytes);
    bool is_known(const evm_common::u256 height, const xbytes_t & hash_bytes) const;
    bool is_confirmed(const evm_common::u256 height, const xbytes_t & hash_bytes) const;
    evm_common::bigint get_height() const;
    void reset();
};
using xevm_bsc_client_contract_t = xtop_evm_bsc_client_contract;

NS_END4
