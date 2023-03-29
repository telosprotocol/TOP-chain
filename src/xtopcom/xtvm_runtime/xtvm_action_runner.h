// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xtop_action_fwd.h"
#include "xevm_common/xevm_transaction_result.h"

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

#include "xtvm_runtime/xtvm_context.h"

#include <memory>

NS_BEG2(top, tvm)

class xtop_vm_action_runner {
private:
    statectx::xstatectx_face_ptr_t m_statectx;

public:
    xtop_vm_action_runner(statectx::xstatectx_face_ptr_t const statectx);
    evm_common::xevm_transaction_result_t execute_action(std::unique_ptr<xtvm_context_t> context);
};
using xtvm_action_runner_t = xtop_vm_action_runner;

NS_END2