// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xcontract_common/xcontract_execution_context.h"
#include "xcontract_runtime/xtransaction_execution_result.h"
#include "xdata/xaction.h"
#include "xdata/xtransaction.h"

NS_BEG3(top, contract_runtime, lua)

class xtop_engine_context {
public:
    xtop_engine_context(data::xtransaction_ptr_t const & tx,
                        observer_ptr<contract_common::xcontract_execution_context_t> contract_context,
                        observer_ptr<xtransaction_execution_result_t> trace_ptr);

    data::xaction_t current_action;
    common::xaccount_address_t parent_account;
    common::xaccount_address_t contract_account;
    common::xaccount_address_t source_account;
    // std::shared_ptr<xcontract_helper> m_contract_helper;
    observer_ptr<xtransaction_execution_result_t> execution_status;
    observer_ptr<contract_common::xcontract_execution_context_t> contract_context;
};
using xengine_context_t = xtop_engine_context;

NS_END3
