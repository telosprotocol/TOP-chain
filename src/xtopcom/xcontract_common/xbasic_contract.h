// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xcontract_common/xcontract_state_fwd.h"
#include "xcontract_common/xcontract_fwd.h"
#include "xcontract_common/xcontract_face.h"


NS_BEG2(top, contract_common)

enum class xtop_contract_type: std::uint8_t {
    invalid,
    system,
    user,
};
using xcontract_type_t = xtop_contract_type;

struct xtop_contract_metadata {
    xtop_contract_metadata() = default;
    xtop_contract_metadata(xtop_contract_metadata const& meta);

    xcontract_type_t m_type{ xcontract_type_t::invalid };
    common::xaccount_address_t m_account{};
};

class xtop_basic_contract : public xcontract_face_t {
protected:
    observer_ptr<xcontract_execution_context_t> m_associated_execution_context{nullptr};
    observer_ptr<xcontract_state_t> m_state{nullptr};
    xcontract_metadata_t m_contract_meta;

public:
    xtop_basic_contract(xtop_basic_contract const &) = delete;
    xtop_basic_contract & operator=(xtop_basic_contract const &) = delete;
    xtop_basic_contract(xtop_basic_contract &&) = default;
    xtop_basic_contract & operator=(xtop_basic_contract &&) = default;
    ~xtop_basic_contract() override = default;

    common::xaccount_address_t address() const override final;
    common::xaccount_address_t sender() const override final;
    common::xaccount_address_t recver() const override final;
    observer_ptr<xcontract_state_t> const & state() const noexcept;
    xcontract_type_t type() const;

protected:
    xtop_basic_contract() = default;

    void reset_execution_context(observer_ptr<xcontract_execution_context_t> exe_ctx) override final;
    bool at_source_action_stage() const noexcept override final;
    bool at_target_action_stage() const noexcept override final;
    bool at_confirm_action_stage() const noexcept override final;

    xbyte_buffer_t const & receipt_data(std::string const & key, std::error_code & ec) const override final;
    void write_receipt_data(std::string const & key, xbyte_buffer_t value, std::error_code & ec) override final;
    void call(common::xaccount_address_t const & target_addr,
              std::string const & method_name,
              std::string const & method_params,
              xfollowup_transaction_schedule_type_t type) override;
};

NS_END2
