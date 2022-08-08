// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xevm_contract_runtime/xevm_sys_contract_face.h"

NS_BEG4(top, contract_runtime, evm, sys_contract)

class xtop_delegate_usdt_contract : public xevm_syscontract_face_t {
public:
    xtop_delegate_usdt_contract() = default;
    xtop_delegate_usdt_contract(xtop_delegate_usdt_contract const &) = delete;
    xtop_delegate_usdt_contract & operator=(xtop_delegate_usdt_contract const &) = delete;
    xtop_delegate_usdt_contract(xtop_delegate_usdt_contract &&) = default;
    xtop_delegate_usdt_contract & operator=(xtop_delegate_usdt_contract &&) = default;
    ~xtop_delegate_usdt_contract() override = default;

    XATTRIBUTE_NODISCARD bool execute(xbytes_t input,
                                      uint64_t target_gas,
                                      sys_contract_context const & context,
                                      bool is_static,
                                      observer_ptr<statectx::xstatectx_face_t> state_ctx,
                                      sys_contract_precompile_output & output,
                                      sys_contract_precompile_error & err) override;
};
using xdelegate_usdt_contract_t = xtop_delegate_usdt_contract;

NS_END4
