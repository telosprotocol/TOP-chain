// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xerror/xerror.h"
#include "xbasic/xmemory.hpp"
#include "xcontract_common/xaction_execution_param.h"
#include "xcontract_common/xcontract_state_fwd.h"
#include "xcontract_common/xproperties/xbasic_property.h"
#include "xcontract_common/xproperties/xproperty_access_control.h"
#include "xdata/xtransaction.h"
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
#include <type_traits>
#include <system_error>

NS_BEG2(top, contract_common)

enum class xtop_enum_action_type : uint8_t { invalid, source_action, target_action, confirm_action, self_action };
using xaction_type_t = xtop_enum_action_type;

enum class xtop_enum_property_name_type : uint8_t { invalid, full_name, short_name };
using xproperty_name_type_t = xtop_enum_property_name_type;

class xtop_contract_state {
private:
    common::xaccount_address_t m_action_account_address;
    observer_ptr<properties::xproperty_access_control_t> m_ac;
    observer_ptr<state_accessor::xstate_accessor_t> m_state_accessor;
    xcontract_execution_param_t const m_param;

public:
    xtop_contract_state(xtop_contract_state const &) = delete;
    xtop_contract_state & operator=(xtop_contract_state const &) = delete;
    xtop_contract_state(xtop_contract_state &&) = default;
    xtop_contract_state & operator=(xtop_contract_state &&) = default;
    ~xtop_contract_state() = default;

    // explicit xtop_contract_state(common::xaccount_address_t action_account_addr, observer_ptr<properties::xproperty_access_control_t> ac);
    explicit xtop_contract_state(common::xaccount_address_t action_account_addr, observer_ptr<state_accessor::xstate_accessor_t> sa, xcontract_execution_param_t const & execution_param);

    common::xaccount_address_t state_account_address() const;

    state_accessor::xtoken_t withdraw(state_accessor::properties::xproperty_identifier_t const & property_id,
                                      common::xsymbol_t const & symbol,
                                      uint64_t amount,
                                      std::error_code & ec);

    state_accessor::xtoken_t withdraw(state_accessor::properties::xproperty_identifier_t const & property_id,
                                      common::xsymbol_t const & symbol,
                                      uint64_t amount);

    void deposit(state_accessor::properties::xproperty_identifier_t const & property_id, state_accessor::xtoken_t tokens, std::error_code & ec);

    void deposit(state_accessor::properties::xproperty_identifier_t const & property_id, state_accessor::xtoken_t tokens);

    /// @brief Create property.
    /// @param property_id The property identifier.
    /// @param ec Log the error code in property creation process.
    void create_property(state_accessor::properties::xproperty_identifier_t const & property_id, std::error_code & ec);

    /// @brief Create property. If creation failed, xtop_error_t exception will be thrown.
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

    /// @brief Update property cell value. Only map and deque are supported.
    /// @param proprty_id Property ID.
    /// @param key Cell position key.
    /// @param value Cell's new value.
    /// @param ec Log the error code in the operation.
    template <state_accessor::properties::xproperty_type_t PropertyTypeV,
              typename std::enable_if<PropertyTypeV == state_accessor::properties::xproperty_type_t::map ||
                                      PropertyTypeV == state_accessor::properties::xproperty_type_t::deque>::type * = nullptr>
    void set_property_cell_value(state_accessor::properties::xtypeless_property_identifier_t const & proprty_id,
                                 typename state_accessor::properties::xkey_type_of_t<PropertyTypeV>::type const & key,
                                 typename state_accessor::properties::xvalue_type_of_t<PropertyTypeV>::type const & value,
                                 std::error_code & ec);

    /// @brief Update property cell value. Only map and deque are supported. Throws xtop_error_t exception if any error occurs.
    /// @param proprty_id Property ID.
    /// @param key Cell position key.
    /// @param value Cell's new value.
    template <state_accessor::properties::xproperty_type_t PropertyTypeV,
              typename std::enable_if<PropertyTypeV == state_accessor::properties::xproperty_type_t::map ||
                                      PropertyTypeV == state_accessor::properties::xproperty_type_t::deque>::type * = nullptr>
    void set_property_cell_value(state_accessor::properties::xtypeless_property_identifier_t const & property_id,
                                 typename state_accessor::properties::xkey_type_of_t<PropertyTypeV>::type const & key,
                                 typename state_accessor::properties::xvalue_type_of_t<PropertyTypeV>::type const & value) {
        std::error_code ec;
        set_property_cell_value<PropertyTypeV>(property_id, key, value, ec);
        top::error::throw_error(ec);
    }

    /// @brief Get property cell value. Only map and deque are supported.
    /// @param property_id Property ID.
    /// @param key Cell position key.
    /// @param ec Log the error code in the operation.
    /// @return Cell value.
    template <state_accessor::properties::xproperty_type_t PropertyTypeV,
              typename std::enable_if<PropertyTypeV == state_accessor::properties::xproperty_type_t::map ||
                                      PropertyTypeV == state_accessor::properties::xproperty_type_t::deque>::type * = nullptr>
    typename state_accessor::properties::xvalue_type_of_t<PropertyTypeV>::type get_property_cell_value(state_accessor::properties::xtypeless_property_identifier_t const & property_id,
                                                                                                       typename state_accessor::properties::xkey_type_of_t<PropertyTypeV>::type const & key,
                                                                                                       std::error_code & ec) const;

    /// @brief Get property cell value. Only map and deque are supported. Throws xtop_error_t exception when any error occurs.
    /// @param property_id Property ID.
    /// @param key Cell position key.
    /// @return Cell value.
    template <state_accessor::properties::xproperty_type_t PropertyTypeV,
              typename std::enable_if<PropertyTypeV == state_accessor::properties::xproperty_type_t::map ||
                                      PropertyTypeV == state_accessor::properties::xproperty_type_t::deque>::type * = nullptr>
    typename state_accessor::properties::xvalue_type_of_t<PropertyTypeV>::type get_property_cell_value(state_accessor::properties::xtypeless_property_identifier_t const & property_id,
                                                                                                       typename state_accessor::properties::xkey_type_of_t<PropertyTypeV>::type const & key) const {
        std::error_code ec;
        auto r = get_property_cell_value<PropertyTypeV>(property_id, key, ec);
        top::error::throw_error(ec);
        return r;
    }

    /// @brief Check if property cell key exist. Only map and deque are supported.
    /// @param property_id Property ID.
    /// @param key Cell position key.
    /// @param ec Log the error code in the operation.
    /// @return exist or not.
    template <state_accessor::properties::xproperty_type_t PropertyTypeV,
              typename std::enable_if<PropertyTypeV == state_accessor::properties::xproperty_type_t::map ||
                                      PropertyTypeV == state_accessor::properties::xproperty_type_t::deque>::type * = nullptr>
    bool exist_property_cell_key(state_accessor::properties::xtypeless_property_identifier_t const & property_id,
                                 typename state_accessor::properties::xkey_type_of_t<PropertyTypeV>::type const & key,
                                 std::error_code & ec) const;

    /// @brief Check if property cell key exist. Only map and deque are supported. Throws xtop_error_t exception if any error occurs.
    /// @param property_id Property ID.
    /// @param key Cell position key.
    /// @return exist or not.
    template <state_accessor::properties::xproperty_type_t PropertyTypeV,
              typename std::enable_if<PropertyTypeV == state_accessor::properties::xproperty_type_t::map ||
                                      PropertyTypeV == state_accessor::properties::xproperty_type_t::deque>::type * = nullptr>
    bool exist_property_cell_key(state_accessor::properties::xtypeless_property_identifier_t const & property_id,
                                 typename state_accessor::properties::xkey_type_of_t<PropertyTypeV>::type const & key) const {
        std::error_code ec;
        auto r = this->exist_property_cell_key<PropertyTypeV>(property_id, key, ec);
        top::error::throw_error(ec);
        return r;
    }

    /// @brief Set property.
    /// @param property_id Property ID.
    /// @param value Value to be set.
    /// @param ec Log the error code in the operation.
    template <state_accessor::properties::xproperty_type_t PropertyTypeV>
    void set_property(state_accessor::properties::xtypeless_property_identifier_t const & property_id,
                      typename state_accessor::properties::xtype_of_t<PropertyTypeV>::type const & value,
                      std::error_code & ec) {
        assert(m_state_accessor != nullptr);
        assert(!ec);

        m_state_accessor->set_property<PropertyTypeV>(property_id, value, ec);
    }

    /// @brief Set property.
    /// @param property_id Property ID.
    /// @param value Value to be set.
    /// @param ec Log the error code in the operation.
    template <state_accessor::properties::xproperty_type_t PropertyTypeV>
    void set_property(state_accessor::properties::xtypeless_property_identifier_t const & property_id,
                      typename state_accessor::properties::xtype_of_t<PropertyTypeV>::type const & value) {
        std::error_code ec;
        set_property<PropertyTypeV>(property_id, value, ec);
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

    /// @brief Get property.
    /// @param property_id Property ID.
    /// @param ec Log the error code in the operation.
    /// @return Property value.
    template <state_accessor::properties::xproperty_type_t PropertyTypeV>
    typename state_accessor::properties::xtype_of_t<PropertyTypeV>::type get_property(state_accessor::properties::xtypeless_property_identifier_t const & property_id) const {
        std::error_code ec;
        auto r = get_property<PropertyTypeV>(property_id, ec);
        top::error::throw_error(ec);
        return r;
    }

    /// @brief Get code. If current state is a contract state, returns the contract code.
    /// @param ec Log the error code.
    /// @return The bytecode.
    xbyte_buffer_t bin_code(state_accessor::properties::xproperty_identifier_t const & property_id, std::error_code & ec) const;

    /// @brief Deploy bytecode.
    /// @param bin_code The bytecode to be deployed.
    /// @param ec Log the error code in the deployment logic.
    void deploy_bin_code(state_accessor::properties::xproperty_identifier_t const & property_id, xbyte_buffer_t bin_code, std::error_code & ec);

    /// @brief Deploy bytecode. If error occurs, xtop_error_t exception will be thrown.
    /// @param bin_code The bytecode to be deployed.
    void deploy_bin_code(state_accessor::properties::xproperty_identifier_t const & property_id, xbyte_buffer_t bin_code);

    /// @brief Get the balance from the state.
    /// @param ec Log the error code in the call.
    /// @return The token object.
    uint64_t balance(state_accessor::properties::xproperty_identifier_t const & property_id,
                     common::xsymbol_t const & symbol,
                     std::error_code & ec) const;

    /// @brief Get the balance from the state. Throw xtop_error_t exception when any error occurs.
    /// @return The token object.
    uint64_t balance(state_accessor::properties::xproperty_identifier_t const & property_id, common::xsymbol_t const & symbol) const;

    std::string binlog(std::error_code & ec) const;
    std::string binlog() const;
    std::string fullstate_bin() const;

    common::xlogic_time_t time() const;
    common::xlogic_time_t timestamp() const;

    uint256_t latest_sendtx_hash(std::error_code& ec) const;
    uint256_t latest_sendtx_hash() const;
    void latest_sendtx_hash(uint256_t hash, std::error_code& ec);
    void latest_sendtx_hash(uint256_t hash);
    uint64_t  latest_sendtx_nonce(std::error_code& ec) const;
    uint64_t  latest_sendtx_nonce() const;
    void latest_sendtx_nonce(uint64_t nonce, std::error_code& ec);
    void latest_sendtx_nonce(uint64_t nonce);

    uint256_t latest_followup_tx_hash(std::error_code& ec) const;
    uint256_t latest_followup_tx_hash() const;
    void latest_followup_tx_hash(uint256_t hash, std::error_code& ec);
    void latest_followup_tx_hash(uint256_t hash);
    uint64_t  latest_followup_tx_nonce(std::error_code& ec) const;
    uint64_t  latest_followup_tx_nonce() const;
    void latest_followup_tx_nonce(uint64_t nonce, std::error_code& ec);
    void latest_followup_tx_nonce(uint64_t nonce);
};
using xcontract_state_t = xtop_contract_state;

#include "xcontract_common/xcontract_state_impl.h"

NS_END2
