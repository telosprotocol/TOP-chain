// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xnative_contract_address.h"
#include "xevm_common/xeth/xeth_header.h"
#include "xevm_contract_runtime/xevm_sys_contract_face.h"

NS_BEG4(top, contract_runtime, evm, sys_contract)

class xtop_evm_heco_client_contract : public xevm_crosschain_syscontract_face_t {
public:
    xtop_evm_heco_client_contract();
    ~xtop_evm_heco_client_contract() override = default;

    bool execute(xbytes_t input,
                 uint64_t target_gas,
                 sys_contract_context const & context,
                 bool is_static,
                 observer_ptr<statectx::xstatectx_face_t> state_ctx,
                 sys_contract_precompile_output & output,
                 sys_contract_precompile_error & err) override;

private:
    virtual bool init(const xbytes_t & rlp_bytes) override;
    virtual bool sync(const xbytes_t & rlp_bytes) override;
    virtual bool is_known(const evm_common::u256 height, const xbytes_t & hash_bytes) const override;
    virtual bool is_confirmed(const evm_common::u256 height, const xbytes_t & hash_bytes) const override;
    virtual evm_common::bigint get_height() const override;
    virtual void reset() override;
};
using xevm_heco_client_contract_t = xtop_evm_heco_client_contract;

NS_END4
