// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcontract_common/xcontract_face.h"
#include "xcontract_common/xcontract_fwd.h"
#include "xcontract_common/xstateless_contract_face.h"

NS_BEG2(top, contract_common)

class xtop_basic_stateless_contract : public xstateless_contract_face_t {
protected:
    observer_ptr<xstateless_contract_execution_context_t> m_associated_execution_context{nullptr};

public:
    xtop_basic_stateless_contract() = default;
    xtop_basic_stateless_contract(xtop_basic_stateless_contract const &) = delete;
    xtop_basic_stateless_contract & operator=(xtop_basic_stateless_contract const &) = delete;
    xtop_basic_stateless_contract(xtop_basic_stateless_contract &&) = default;
    xtop_basic_stateless_contract & operator=(xtop_basic_stateless_contract &&) = default;
    ~xtop_basic_stateless_contract() override = default;

    observer_ptr<data::xunit_bstate_t> unitstate_owned() const noexcept {
        return m_associated_execution_context->unitstate_owned();
    }

    observer_ptr<data::xunit_bstate_t> unitstate_other() const noexcept {
        return m_associated_execution_context->unitstate_other();
    }

    void reset_execution_context(observer_ptr<xstateless_contract_execution_context_t> exe_ctx) override final {
        m_associated_execution_context = exe_ctx;
    }
};

NS_END2
