// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

#include "xstatectx/xstatectx_face.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include "xdata/xtop_action_fwd.h"
#include "xtxexecutor/xvm_face.h"

NS_BEG2(top, tvm)

class xtop_vm : public txexecutor::xvm_face_t {
private:
    statectx::xstatectx_face_ptr_t m_statectx;

public:
    xtop_vm(statectx::xstatectx_face_ptr_t const statectx);

public:
    txexecutor::enum_execute_result_type execute(txexecutor::xvm_input_t const & input, txexecutor::xvm_output_t & output) override;
    evm_common::xevm_transaction_result_t execute_action(std::unique_ptr<data::xbasic_top_action_t const> action, txexecutor::xvm_para_t const & vm_para);
};
using xtvm_t = xtop_vm;

NS_END2