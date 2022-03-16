// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xcontract_common/xcontract_face.h"
#include "xcontract_common/xcontract_fwd.h"
#include "xcontract_common/xcontract_state.h"
#include "xcontract_common/xproperties/xproperty_initializer.h"
#include "xcontract_common/xproperties/xproperty_integer.h"
#include "xcontract_common/xproperties/xproperty_map.h"
#include "xcontract_common/xproperties/xproperty_string.h"
#include "xcontract_common/xproperties/xproperty_token.h"
#include "xstate_accessor/xtoken.h"

NS_BEG2(top, contract_common)

enum class xtop_contract_type: std::uint8_t {
    invalid,
    system,
    user,
};
using xcontract_type_t = xtop_contract_type;

struct xtop_contract_metadata {
    xtop_contract_metadata() = default;
    xtop_contract_metadata(xtop_contract_metadata const& meta) = default;
    xtop_contract_metadata(common::xaccount_address_t const& account, xcontract_type_t type = xcontract_type_t::system);

    xcontract_type_t m_type{ xcontract_type_t::invalid };
    common::xaccount_address_t m_account{};
};

class xtop_basic_contract : public xcontract_face_t {
protected:
    properties::xproperty_initializer_t m_property_initializer;
    observer_ptr<xcontract_execution_context_t> m_associated_execution_context{nullptr};
    xcontract_metadata_t m_contract_meta;

    properties::xtoken_property_t m_balance{this};
    properties::xtoken_property_t m_burn_balance{data::XPROPERTY_BALANCE_BURN, this};
    properties::xtoken_property_t m_lock_balance{data::XPROPERTY_BALANCE_LOCK, this};
    properties::xtoken_property_t m_tgas_balance{data::XPROPERTY_BALANCE_PLEDGE_TGAS, this};
    properties::xtoken_property_t m_vote_balance{data::XPROPERTY_BALANCE_PLEDGE_VOTE, this};

    properties::xuint64_property_t m_lock_tgas{data::XPROPERTY_LOCK_TGAS, this};
    properties::xstring_property_t m_used_tgas{data::XPROPERTY_USED_TGAS_KEY, this};
    properties::xstring_property_t m_last_tx_hour{data::XPROPERTY_LAST_TX_HOUR_KEY, this};

    properties::xmap_property_t<std::string, std::string> m_vote_detail{data::XPROPERTY_PLEDGE_VOTE_KEY, this};
    properties::xstring_property_t m_expire_vote_token{data::XPROPERTY_EXPIRE_VOTE_TOKEN_KEY, this};
    properties::xuint64_property_t m_unvote_num{data::XPROPERTY_UNVOTE_NUM, this};

    properties::xmap_property_t<std::string, std::string> m_tx_info{data::XPROPERTY_TX_INFO, this};
    properties::xuint64_property_t m_create_time{data::XPROPERTY_ACCOUNT_CREATE_TIME, this};
    properties::xmap_property_t<std::string, std::string> m_lock_token{data::XPROPERTY_LOCK_TOKEN_KEY, this};

protected:
    xtop_basic_contract() = default;

public:
    xtop_basic_contract(xtop_basic_contract const &) = delete;
    xtop_basic_contract & operator=(xtop_basic_contract const &) = delete;
    xtop_basic_contract(xtop_basic_contract &&) = default;
    xtop_basic_contract & operator=(xtop_basic_contract &&) = default;
    ~xtop_basic_contract() override = default;

    common::xaccount_address_t address() const override final;
    common::xaccount_address_t sender() const override final;
    common::xaccount_address_t recver() const override final;

    void register_property(properties::xbasic_property_t * property) override final;

    common::xnetwork_id_t network_id() const override;

    uint64_t balance() const override;
    state_accessor::xtoken_t withdraw(std::uint64_t amount) override;
    void deposit(state_accessor::xtoken_t token) override;

    observer_ptr<xcontract_state_t> contract_state() const noexcept override;

    void reset_execution_context(observer_ptr<xcontract_execution_context_t> exe_ctx) override final;

    xcontract_type_t type() const;
    data::enum_xaction_type action_type() const;
    data::enum_xaction_type source_action_type() const;
    data::enum_xaction_type target_action_type() const;
    xbyte_buffer_t action_data() const;
    state_accessor::xtoken_t last_action_asset(std::error_code & ec) const;
    void asset_to_next_action(state_accessor::xtoken_t token);
    data::enum_xtransaction_type transaction_type() const;
    common::xlogic_time_t time() const;
    common::xlogic_time_t timestamp() const;
    std::string const & random_seed() const noexcept;
    uint64_t state_height(common::xaccount_address_t const & address = common::xaccount_address_t{}) const;
    bool block_exist(common::xaccount_address_t const & address, uint64_t height) const;
    std::vector<xfollowup_transaction_datum_t> followup_transaction() const;
    void exec_delay_followup();

protected:
    observer_ptr<properties::xproperty_initializer_t const> property_initializer() const noexcept;

    template <typename ConcretePropertyT>
    ConcretePropertyT get_property(state_accessor::properties::xtypeless_property_identifier_t const & property_id,
                                   common::xaccount_address_t const & account_address,
                                   std::error_code & ec) const {
        assert(!ec);
        assert(m_associated_execution_context != nullptr);
        assert(m_associated_execution_context->contract_state() != nullptr);
        auto status_of_other_account = xcontract_state_t::build_from(account_address, ec);
        if (ec) {
            return {};
        }

        try {
            return ConcretePropertyT{property_id.full_name(), std::move(status_of_other_account)};
        } catch (top::error::xtop_error_t const & eh) {
            ec = eh.code();
            return {};
        }
    }

    template <typename ConcretePropertyT>
    ConcretePropertyT get_property(state_accessor::properties::xtypeless_property_identifier_t const & property_id,
                                   common::xaccount_address_t const & account_address,
                                   uint64_t const height,
                                   std::error_code & ec) const {
        assert(!ec);
        assert(m_associated_execution_context != nullptr);
        assert(m_associated_execution_context->contract_state() != nullptr);
        auto status_of_other_account = xcontract_state_t::build_from(account_address, height, ec);
        if (ec) {
            return {};
        }

        try {
            return ConcretePropertyT{property_id.full_name(), std::move(status_of_other_account)};
        } catch (top::error::xtop_error_t const & eh) {
            ec = eh.code();
            return {};
        }
    }

    template <typename ConcretePropertyT>
    ConcretePropertyT get_property(state_accessor::properties::xtypeless_property_identifier_t const & property_id, common::xaccount_address_t const & account_address) const {
        std::error_code ec;
        auto r = get_property<ConcretePropertyT>(property_id, account_address, ec);
        top::error::throw_error(ec);
        return r;
    }

    template <typename ConcretePropertyT>
    ConcretePropertyT get_property(state_accessor::properties::xtypeless_property_identifier_t const & property_id, common::xaccount_address_t const & account_address, uint64_t const height) const {
        std::error_code ec;
        auto r = get_property<ConcretePropertyT>(property_id, account_address, height, ec);
        top::error::throw_error(ec);
        return r;
    }

    xbyte_buffer_t receipt_data(std::string const & key) const override final;
    void write_receipt_data(std::string const & key, xbyte_buffer_t value, std::error_code & ec) override final;
    void call(common::xaccount_address_t const & target_addr,
              std::string const & method_name,
              std::string const & method_params,
              xfollowup_transaction_schedule_type_t type) override;
    void call(common::xaccount_address_t const & target_addr,
              std::string const & source_method_name,
              std::string const & source_method_params,
              std::string const & target_method_name,
              std::string const & target_method_params,
              xfollowup_transaction_schedule_type_t type) override;
    void sync_call(common::xaccount_address_t const & target_addr,
                   std::string const & target_method_name,
                   std::string const & target_method_params,
                   xfollowup_transaction_schedule_type_t type = xfollowup_transaction_schedule_type_t::invalid);
    void sync_call(common::xaccount_address_t const & target_addr,
                   std::string const & source_method_name,
                   std::string const & source_method_params,
                   std::string const & target_method_name,
                   std::string const & target_method_params,
                   xfollowup_transaction_schedule_type_t type = xfollowup_transaction_schedule_type_t::invalid);
    void transfer(common::xaccount_address_t const & target_addr, uint64_t amount, xfollowup_transaction_schedule_type_t type, std::error_code & ec);
    void transfer(common::xaccount_address_t const & target_addr, uint64_t amount, xfollowup_transaction_schedule_type_t type);
    void delay_followup(xfollowup_transaction_delay_param_t const & param);
    void delay_followup(std::vector<xfollowup_transaction_delay_param_t> const & params);
};

NS_END2
