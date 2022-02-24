// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xerror/xerror.h"
#include "xbasic/xmemory.hpp"
#include "xcontract_common/xcontract_execution_param.h"
#include "xcontract_common/xcontract_state_fwd.h"
#include "xcontract_common/xproperties/xbasic_property.h"
#include "xdata/xtransaction.h"
#include "xstake/xstake_algorithm.h"
#include "xstate_accessor/xproperties/xproperty_identifier.h"
#include "xstate_accessor/xstate_accessor.h"
#include "xstate_accessor/xtoken.h"

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

#include "xvledger/xvstate.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include <cassert>
#include <system_error>
#include <type_traits>

NS_BEG2(top, contract_common)

enum class xtop_enum_action_type : uint8_t { invalid, source_action, target_action, confirm_action, self_action };
using xaction_type_t = xtop_enum_action_type;

enum class xtop_enum_property_name_type : uint8_t { invalid, full_name, short_name };
using xproperty_name_type_t = xtop_enum_property_name_type;

class xtop_contract_state {
private:
    common::xaccount_address_t m_action_account_address;
    std::unique_ptr<state_accessor::xstate_accessor_t> m_state_accessor_owned{nullptr};
    observer_ptr<state_accessor::xstate_accessor_t> m_state_accessor;
    xcontract_execution_param_t const m_param{};

public:
    xtop_contract_state() = default;
    xtop_contract_state(xtop_contract_state const &) = delete;
    xtop_contract_state & operator=(xtop_contract_state const &) = delete;
    xtop_contract_state(xtop_contract_state &&) = default;
    xtop_contract_state & operator=(xtop_contract_state &&) = default;
    ~xtop_contract_state() = default;

    explicit xtop_contract_state(common::xaccount_address_t action_account_addr,
                                 observer_ptr<state_accessor::xstate_accessor_t> sa,
                                 xcontract_execution_param_t const & execution_param);

private:
    explicit xtop_contract_state(common::xaccount_address_t const & account_address);
    explicit xtop_contract_state(common::xaccount_address_t const & account_address, uint64_t const height);

public:

    static std::unique_ptr<xtop_contract_state> build_from(common::xaccount_address_t const & account_address);
    static std::unique_ptr<xtop_contract_state> build_from(common::xaccount_address_t const & account_address, std::error_code & ec);

    static std::unique_ptr<xtop_contract_state> build_from(common::xaccount_address_t const & account_address, uint64_t const height);
    static std::unique_ptr<xtop_contract_state> build_from(common::xaccount_address_t const & account_address, uint64_t const height, std::error_code & ec);

    /// @brief Get state account address.
    /// @return State account address.
    common::xaccount_address_t state_account_address() const;

    /// @brief Get state height.
    /// @param address Address to check. Check state address if default empty address.
    /// @return State height.
    uint64_t state_height(common::xaccount_address_t const & address) const;

    /// @brief Check if block object exists or not.
    /// @param address Address to check.
    /// @param height block height to check.
    /// @return 'true' if block object exists; otherwise 'false'.
    bool block_exist(common::xaccount_address_t const & user, uint64_t height) const;

    /// @brief Create property.
    /// @param property_id The property identifier.
    /// @param ec Log the error code in property creation process.
    void create_property(state_accessor::properties::xproperty_identifier_t const & property_id, std::error_code & ec);

    /// @brief Create property. Throw xtop_error_t exception when error occurs.
    /// @param property_id The property identifier.
    void create_property(state_accessor::properties::xproperty_identifier_t const & property_id);

    /// @brief Check if the property identified by the property ID exists or not.
    /// @param property_id Property ID.
    /// @param ec Log the error code in the operation.
    /// @return 'true' if property exists; otherwise 'false'.
    bool property_exist(state_accessor::properties::xproperty_identifier_t const & property_id, std::error_code & ec) const;

    /// @brief Check if the property identified by the property ID exists or not. Throw xtop_error_t exception when error occurs.
    /// @param property_id property_id Property ID.
    /// @return 'true' if property exists; otherwise 'false'.
    bool property_exist(state_accessor::properties::xproperty_identifier_t const & property_id) const;

    /// @brief Clear property. This operation liks STL container's clear() API.
    /// @param property_id Property ID.
    /// @param ec Log the error code in the operation.
    void clear_property(state_accessor::properties::xproperty_identifier_t const & property_id, std::error_code & ec);

    /// @brief Clear property. This operation liks STL container's clear() API. Throws xtop_error_t exception when any error occurs.
    /// @param property_id Property ID.
    void clear_property(state_accessor::properties::xproperty_identifier_t const & property_id);

    /// @brief Update property cell value. Only map and deque are supported.
    /// @param proprty_id Property ID.
    /// @param key Cell position key.
    /// @param value Cell's new value.
    /// @param ec Log the error code in the operation.
    template <state_accessor::properties::xproperty_type_t PropertyTypeV>
    void set_property_cell_value(state_accessor::properties::xtypeless_property_identifier_t const & property_id,
                                 typename state_accessor::properties::xkey_type_of_t<PropertyTypeV>::type const & key,
                                 typename state_accessor::properties::xvalue_type_of_t<PropertyTypeV>::type const & value,
                                 std::error_code & ec) {
        assert(!ec);
        assert(m_state_accessor != nullptr);

        m_state_accessor->set_property_cell_value<PropertyTypeV>(property_id, key, value, ec);
    }

    /// @brief Update property cell value. Only map and deque are supported. Throws xtop_error_t exception if any error occurs.
    /// @param proprty_id Property ID.
    /// @param key Cell position key.
    /// @param value Cell's new value.
    template <state_accessor::properties::xproperty_type_t PropertyTypeV>
    void set_property_cell_value(state_accessor::properties::xtypeless_property_identifier_t const & property_id,
                                 typename state_accessor::properties::xkey_type_of_t<PropertyTypeV>::type const & key,
                                 typename state_accessor::properties::xvalue_type_of_t<PropertyTypeV>::type const & value) {
        std::error_code ec;
        set_property_cell_value<PropertyTypeV>(property_id, key, value, ec);
        assert(!ec);
        top::error::throw_error(ec);
    }

    /// @brief Get property cell value. Only map and deque are supported.
    /// @param property_id Property ID.
    /// @param key Cell position key.
    /// @param ec Log the error code in the operation.
    /// @return Cell value.
    template <state_accessor::properties::xproperty_type_t PropertyTypeV>
    typename state_accessor::properties::xvalue_type_of_t<PropertyTypeV>::type get_property_cell_value(
        state_accessor::properties::xtypeless_property_identifier_t const & property_id,
        typename state_accessor::properties::xkey_type_of_t<PropertyTypeV>::type const & key,
        std::error_code & ec) const {
        assert(!ec);
        assert(m_state_accessor != nullptr);

        return m_state_accessor->get_property_cell_value<PropertyTypeV>(property_id, key, ec);
    }

    /// @brief Get property cell value. Only map and deque are supported. Throws xtop_error_t exception when any error occurs.
    /// @param property_id Property ID.
    /// @param key Cell position key.
    /// @return Cell value.
    template <state_accessor::properties::xproperty_type_t PropertyTypeV>
    typename state_accessor::properties::xvalue_type_of_t<PropertyTypeV>::type get_property_cell_value(
        state_accessor::properties::xtypeless_property_identifier_t const & property_id,
        typename state_accessor::properties::xkey_type_of_t<PropertyTypeV>::type const & key) const {
        std::error_code ec;
        auto r = get_property_cell_value<PropertyTypeV>(property_id, key, ec);
        assert(!ec);
        top::error::throw_error(ec);
        return r;
    }

    /// @brief Check if property cell key exist. Only map and deque are supported.
    /// @param property_id Property ID.
    /// @param key Cell position key.
    /// @param ec Log the error code in the operation.
    /// @return exist or not.
    template <state_accessor::properties::xproperty_type_t PropertyTypeV>
    bool exist_property_cell_key(state_accessor::properties::xtypeless_property_identifier_t const & property_id,
                                 typename state_accessor::properties::xkey_type_of_t<PropertyTypeV>::type const & key,
                                 std::error_code & ec) const {
        assert(!ec);
        assert(m_state_accessor != nullptr);

        return m_state_accessor->exist_property_cell_key<PropertyTypeV>(property_id, key, ec);
    }

    /// @brief Check if property cell key exist. Only map and deque are supported. Throws xtop_error_t exception if any error occurs.
    /// @param property_id Property ID.
    /// @param key Cell position key.
    /// @return exist or not.
    template <state_accessor::properties::xproperty_type_t PropertyTypeV>
    bool exist_property_cell_key(state_accessor::properties::xtypeless_property_identifier_t const & property_id,
                                 typename state_accessor::properties::xkey_type_of_t<PropertyTypeV>::type const & key) const {
        std::error_code ec;
        auto r = this->exist_property_cell_key<PropertyTypeV>(property_id, key, ec);
        assert(!ec);
        top::error::throw_error(ec);
        return r;
    }

    /// @brief Remove property cell at the position key. Only map and deque are supported.
    /// @param property_id Property ID.
    /// @param key Cell position key.
    /// @param ec Log the error code in the operation.
    template <state_accessor::properties::xproperty_type_t PropertyTypeV>
    void remove_property_cell(state_accessor::properties::xtypeless_property_identifier_t const& property_id,
                              typename state_accessor::properties::xkey_type_of_t<PropertyTypeV>::type const& key,
                              std::error_code& ec) const {
        assert(!ec);
        assert(m_state_accessor != nullptr);

        m_state_accessor->remove_property_cell<PropertyTypeV>(property_id, key, ec);
    }

    /// @brief Remove property cell at the position key. Only map and deque are supported.
    /// @param property_id Property ID.
    /// @param key Cell position key.
    /// @param ec Log the error code in the operation.
    template <state_accessor::properties::xproperty_type_t PropertyTypeV>
    void remove_property_cell(state_accessor::properties::xtypeless_property_identifier_t const& property_id,
                              typename state_accessor::properties::xkey_type_of_t<PropertyTypeV>::type const& key) const {
        std::error_code ec;
        this->remove_property_cell<PropertyTypeV>(property_id, key, ec);
        assert(!ec);
        top::error::throw_error(ec);
    }

    /// @brief Set property.
    /// @param property_id Property ID.
    /// @param value Value to be set.
    /// @param ec Log the error code in the operation.
    template <state_accessor::properties::xproperty_type_t PropertyTypeV>
    void set_property(state_accessor::properties::xtypeless_property_identifier_t const & property_id,
                      typename state_accessor::properties::xtype_of_t<PropertyTypeV>::type const & value,
                      std::error_code & ec) {
        assert(!ec);
        assert(m_state_accessor != nullptr);
        m_state_accessor->set_property<PropertyTypeV>(property_id, value, ec);
    }

    /// @brief Set property. Throw xtop_error_t exception when error occurs.
    /// @param property_id Property ID.
    /// @param value Value to be set.
    template <state_accessor::properties::xproperty_type_t PropertyTypeV>
    void set_property(state_accessor::properties::xtypeless_property_identifier_t const & property_id,
                      typename state_accessor::properties::xtype_of_t<PropertyTypeV>::type const & value) {
        std::error_code ec;
        set_property<PropertyTypeV>(property_id, value, ec);
        assert(!ec);
        top::error::throw_error(ec);
    }

    /// @brief Get property.
    /// @param property_id Property ID.
    /// @param ec Log the error code in the operation.
    /// @return Property value.
    template <state_accessor::properties::xproperty_type_t PropertyTypeV>
    typename state_accessor::properties::xtype_of_t<PropertyTypeV>::type get_property(state_accessor::properties::xtypeless_property_identifier_t const & property_id,
                                                                                      std::error_code & ec) const {
        assert(m_state_accessor != nullptr);
        assert(!ec);
        return m_state_accessor->get_property<PropertyTypeV>(property_id, ec);
    }

    /// @brief Get property. Throw xtop_error_t exception when error occurs.
    /// @param property_id Property ID.
    /// @return Property value.
    template <state_accessor::properties::xproperty_type_t PropertyTypeV>
    typename state_accessor::properties::xtype_of_t<PropertyTypeV>::type get_property(state_accessor::properties::xtypeless_property_identifier_t const & property_id) const {
        std::error_code ec;
        auto r = get_property<PropertyTypeV>(property_id, ec);
        assert(!ec);
        top::error::throw_error(ec);
        return r;
    }

    /// @brief Get property of specific address.
    /// @param property_id Property ID.
    /// @param address Address to get.
    /// @param ec Log the error code in the operation.
    /// @return Property value.
    template <state_accessor::properties::xproperty_type_t PropertyTypeV>
    typename state_accessor::properties::xtype_of_t<PropertyTypeV>::type get_property(state_accessor::properties::xtypeless_property_identifier_t const & property_id,
                                                                                      common::xaccount_address_t const & address,
                                                                                      std::error_code & ec) const {
        assert(!ec);
        assert(m_state_accessor != nullptr);
        return m_state_accessor->get_property<PropertyTypeV>(property_id, address, ec);
    }

    /// @brief Get property of specific address. Throw xtop_error_t exception when error occurs.
    /// @param property_id Property ID.
    /// @param address Address to get.
    /// @return Property value.
    template <state_accessor::properties::xproperty_type_t PropertyTypeV>
    typename state_accessor::properties::xtype_of_t<PropertyTypeV>::type get_property(state_accessor::properties::xtypeless_property_identifier_t const & property_id,
                                                                                      common::xaccount_address_t const & address) const {
        std::error_code ec;
        auto r = get_property<PropertyTypeV>(property_id, address, ec);
        assert(!ec);
        top::error::throw_error(ec);
        return r;
    }

    /// @brief Get Property size.
    /// @param property_id Property ID.
    /// @param ec Log the error.
    /// @return Property size.
    size_t property_size(state_accessor::properties::xproperty_identifier_t const & property_id, std::error_code & ec) const;

    /// @brief Property size.
    /// @param property_id Property ID.
    /// @return Property size.
    size_t property_size(state_accessor::properties::xproperty_identifier_t const & property_id) const;

    /// @brief Get code. If current state is a contract state, returns the contract code.
    /// @param property_id Property ID.
    /// @param ec Log the error code.
    /// @return The bytecode.
    xbyte_buffer_t bin_code(state_accessor::properties::xproperty_identifier_t const & property_id, std::error_code & ec) const;

    /// @brief Get code. If current state is a contract state, returns the contract code. If error occurs, xtop_error_t exception will be thrown.
    /// @param property_id Property ID.
    /// @return The bytecode.
    xbyte_buffer_t bin_code(state_accessor::properties::xproperty_identifier_t const & property_id) const;

    /// @brief Deploy bytecode.
    /// @param property_id Property ID.
    /// @param bin_code The bytecode to be deployed.
    /// @param ec Log the error code in the deployment logic.
    void deploy_bin_code(state_accessor::properties::xproperty_identifier_t const & property_id, xbyte_buffer_t bin_code, std::error_code & ec);

    /// @brief Deploy bytecode. If error occurs, xtop_error_t exception will be thrown.
    /// @param property_id Property ID.
    /// @param bin_code The bytecode to be deployed.
    void deploy_bin_code(state_accessor::properties::xproperty_identifier_t const & property_id, xbyte_buffer_t bin_code);

    /// @brief Get code of canvas.
    /// @param ec Log the error code in the deployment logic.
    /// @return The bincode of canvas.
    std::string binlog(std::error_code & ec) const;

    /// @brief Get code of canvas. If error occurs, xtop_error_t exception will be thrown.
    /// @return The bincode of canvas.
    std::string binlog() const;

    /// @brief Get the size of state change binlog.
    /// @param ec Log the error code in getting binlog.
    /// @return The binlog data size.
    size_t binlog_size(std::error_code & ec) const;

    /// @brief Get the size of state change binlog.
    /// @return The binlog data size.
    size_t binlog_size() const;

    /// @brief Get full state bin of state. If error occurs, xtop_error_t exception will be thrown.
    /// @param ec Log the error code in the deployment logic.
    /// @return The bincode of state.
    std::string fullstate_bin(std::error_code & ec) const;

    /// @brief Get full state bin of state. If error occurs, xtop_error_t exception will be thrown.
    /// @return The bincode of state.
    std::string fullstate_bin() const;

    /// @brief Get the balance from the state.
    /// @param property_id Property ID.
    /// @param symbol Simbol of the token.
    /// @param ec Log the error code in the call.
    /// @return The token amount.
    uint64_t balance(state_accessor::properties::xproperty_identifier_t const & property_id, common::xsymbol_t const & symbol, std::error_code & ec) const;

    /// @brief Get the balance from the state. Throw xtop_error_t exception when any error occurs.
    /// @param property_id Property ID.
    /// @param symbol Simbol of the token.
    /// @return The token amount.
    uint64_t balance(state_accessor::properties::xproperty_identifier_t const & property_id, common::xsymbol_t const & symbol) const;

    /// @brief Deposit tokens into state.
    /// @param property_id Property ID.
    /// @param tokens Tokens to deposit.
    /// @param ec Log the error code in the call.
    void deposit(state_accessor::properties::xproperty_identifier_t const & property_id, state_accessor::xtoken_t tokens, std::error_code & ec);

    /// @brief Deposit tokens into state. Throw xtop_error_t exception when any error occurs.
    /// @param property_id Property ID.
    /// @param tokens Tokens to deposit.
    void deposit(state_accessor::properties::xproperty_identifier_t const & property_id, state_accessor::xtoken_t tokens);

    /// @brief Withdraw tokens from state.
    /// @param property_id Property ID.
    /// @param symbol Simbol of the token.
    /// @param ec Log the error code in the call.
    /// @return The token object.
    state_accessor::xtoken_t withdraw(state_accessor::properties::xproperty_identifier_t const & property_id,
                                      common::xsymbol_t const & symbol,
                                      uint64_t amount,
                                      std::error_code & ec);

    /// @brief Withdraw tokens from state. Throw xtop_error_t exception when any error occurs.
    /// @param property_id Property ID.
    /// @param symbol Simbol of the token.
    /// @return The token object.
    state_accessor::xtoken_t withdraw(state_accessor::properties::xproperty_identifier_t const & property_id, common::xsymbol_t const & symbol, uint64_t amount);

    /// @brief Transfer tokens from one property the other property in same account state. Throw xtop_error_t exception when any error occurs.
    /// @param from From property.
    /// @param to To property.
    /// @param amount Tranfer amount.
    /// @param ec Log the error code in the call.
    void transfer_internal(state_accessor::properties::xproperty_identifier_t from, state_accessor::properties::xproperty_identifier_t to, uint64_t amount, std::error_code & ec);

    /// @brief Transfer tokens from one property the other property in same account state.
    /// @param from From property.
    /// @param to To property.
    /// @param amount Tranfer amount.
    void transfer_internal(state_accessor::properties::xproperty_identifier_t from, state_accessor::properties::xproperty_identifier_t to, uint64_t amount);

    /// @brief Get the consensus time height from param.
    /// @return Time height.
    common::xlogic_time_t time() const noexcept;

    /// @brief Get the consensus timestamp height from param.
    /// @return Timestamp.
    common::xlogic_time_t timestamp() const noexcept;

    /// @brief Get the time of day second from param.
    /// @return Timestamp.
    common::xlogic_time_t time_of_day() const noexcept;

    /// @brief Get total lock tgas of system.
    /// @return Total tgas.
    uint64_t system_lock_tgas() const noexcept;

    /// @brief Get random_seed of system.
    /// @return Random_seed.
    std::string const & random_seed() const noexcept;

    /* ----------special bstate process interface ---------- */
private:
    uint64_t m_latest_followup_tx_nonce{0};
    uint256_t m_latest_followup_tx_hash{};

public:
    // map, string key
    uint256_t latest_sendtx_hash(std::error_code & ec) const;
    uint256_t latest_sendtx_hash() const;
    void latest_sendtx_hash(uint256_t hash, std::error_code & ec);
    void latest_sendtx_hash(uint256_t hash);
    // map, string key
    uint64_t latest_sendtx_nonce(std::error_code & ec) const;
    uint64_t latest_sendtx_nonce() const;
    void latest_sendtx_nonce(uint64_t nonce, std::error_code & ec);
    void latest_sendtx_nonce(uint64_t nonce);
    // map, string key
    uint256_t latest_followup_tx_hash() const;
    void latest_followup_tx_hash(uint256_t hash);
    // map, string key
    uint64_t latest_followup_tx_nonce() const;
    void latest_followup_tx_nonce(uint64_t nonce);
    // map, string key
    uint64_t unconfirm_sendtx_num(std::error_code & ec) const;
    uint64_t unconfirm_sendtx_num() const;
    void unconfirm_sendtx_num(uint64_t num, std::error_code & ec);
    void unconfirm_sendtx_num(uint64_t num);
    // uint64
    uint64_t lock_tgas(std::error_code & ec) const;
    uint64_t lock_tgas() const;
    void lock_tgas(uint64_t amount, std::error_code & ec);
    void lock_tgas(uint64_t amount);
    // string
    uint64_t used_tgas(std::error_code & ec) const;
    uint64_t used_tgas() const;
    void used_tgas(uint64_t amount, std::error_code & ec);
    void used_tgas(uint64_t amount);
    // string
    uint64_t disk(std::error_code & ec) const;
    uint64_t disk() const;
    void disk(uint64_t amount, std::error_code & ec);
    void disk(uint64_t amount);
    // string
    uint64_t last_tx_hour(std::error_code & ec) const;
    uint64_t last_tx_hour() const;
    void last_tx_hour(uint64_t hour, std::error_code & ec);
    void last_tx_hour(uint64_t hour);
    // map
    std::map<std::string, xstake::xreward_dispatch_task> delay_followup(std::error_code & ec) const;
    std::map<std::string, xstake::xreward_dispatch_task> delay_followup() const;
    void delay_followup(xstake::xreward_dispatch_task const & task, std::error_code & ec);
    void delay_followup(xstake::xreward_dispatch_task const & task);
    void delay_followup(std::vector<xstake::xreward_dispatch_task> const & tasks, std::error_code & ec);
    void delay_followup(std::vector<xstake::xreward_dispatch_task> const & tasks);
};
using xcontract_state_t = xtop_contract_state;

NS_END2
