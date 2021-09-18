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

public:
    xtop_contract_state(xtop_contract_state const &) = delete;
    xtop_contract_state & operator=(xtop_contract_state const &) = delete;
    xtop_contract_state(xtop_contract_state &&) = default;
    xtop_contract_state & operator=(xtop_contract_state &&) = default;
    ~xtop_contract_state() = default;

    explicit xtop_contract_state(common::xaccount_address_t action_account_addr, observer_ptr<properties::xproperty_access_control_t> ac);

    common::xaccount_address_t state_account_address() const;
    observer_ptr<properties::xproperty_access_control_t> const & access_control() const;



    std::string src_code(std::error_code & ec) const;
    std::string src_code() const;
    xbyte_buffer_t bin_code(std::error_code & ec) const;

    void deploy_src_code(std::string code, std::error_code & ec);
    void deploy_src_code(std::string code);

    void deploy_bin_code(xbyte_buffer_t bin_code, std::error_code & ec);
    void deploy_bin_code(xbyte_buffer_t bin_code);

    std::string binlog(std::error_code & ec) const;
    std::string binlog() const;
    std::string fullstate_bin() const;

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
