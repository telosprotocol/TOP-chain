// Copyright (c) 2017-2021 Telos Foundation & contributors
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

#include "xvledger/xvstate.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xerror/xthrow_error.h"
#include "xbasic/xmemory.hpp"
#include "xcontract_common/xcontract_state_fwd.h"
#include "xcontract_common/xproperties/xbasic_property.h"
#include "xcontract_common/xproperties/xproperty_access_control.h"
#include "xdata/xtransaction.h"
#include "xstate_accessor/xproperties/xproperty_identifier.h"
#include "xstate_accessor/xstate_accessor.h"

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

public:
    xtop_contract_state(xtop_contract_state const &) = delete;
    xtop_contract_state & operator=(xtop_contract_state const &) = delete;
    xtop_contract_state(xtop_contract_state &&) = default;
    xtop_contract_state & operator=(xtop_contract_state &&) = default;
    ~xtop_contract_state() = default;

    explicit xtop_contract_state(common::xaccount_address_t action_account_addr, observer_ptr<properties::xproperty_access_control_t> ac);
    explicit xtop_contract_state(common::xaccount_address_t action_account_addr, observer_ptr<state_accessor::xstate_accessor_t> sa);

    common::xaccount_address_t state_account_address() const;

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

    /// @brief Get code. If current state is a contract state, returns the contract code.
    /// @param ec Log the error code.
    /// @return The bytecode.
    xbyte_buffer_t bin_code(std::error_code & ec) const;

    /// @brief Deploy bytecode.
    /// @param bin_code The bytecode to be deployed.
    /// @param ec Log the error code in the deployment logic.
    void deploy_bin_code(xbyte_buffer_t bin_code, std::error_code & ec);

    /// @brief Deploy bytecode. If error occurs, xtop_error_t exception will be thrown.
    /// @param bin_code The bytecode to be deployed.
    void deploy_bin_code(xbyte_buffer_t bin_code);

    /// @brief Get the balance from the state.
    /// @param ec Log the error code in the call.
    /// @return The token object.
    state_accessor::xtoken_t const & balance(std::string const & symbol, std::error_code & ec) const;

    /// @brief Get the balance from the state. Throw xtop_error_t exception when any error occurs.
    /// @return The token object.
    state_accessor::xtoken_t const & balance(std::string const & symbol) const;

    std::string binlog(std::error_code & ec) const;
    std::string binlog() const;
    std::string fullstate_bin() const;

    uint64_t token_withdraw(uint64_t amount, std::error_code& ec) const;
    uint64_t token_withdraw(uint64_t amount) const;

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

NS_END2
