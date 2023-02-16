// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xcommon/xtoken_metadata.h"
#include "xcommon/common.h"
#include "xstatectx/xstatectx_face.h"

#include <memory>

NS_BEG2(top, state_reset)

/// tablestate reseter base hold statectx relevant
/// provide api to reset each specific properties.
/// while not care how this data come from (no json parse code here)
class xstate_tablestate_reseter_base {
public:
    virtual ~xstate_tablestate_reseter_base() = default;

    xstate_tablestate_reseter_base(statectx::xstatectx_face_ptr_t statectx_ptr);

    //! In override methods, parameter cnt must have default value = 0;
    virtual bool exec_reset_tablestate(std::size_t cnt = 0) = 0;

protected:
    void account_set_tep1_token(std::string const & account_address, common::xtoken_id_t const & token_id, evm_common::u256 const & token_value);
    void account_set_top_balance(std::string const & account_address, std::string const & property_name, uint64_t property_value);
    void account_set_property(std::string const & account_address, std::string const & property_name, std::string const & property_type, std::string const & property_value);
    void account_set_state(std::string const & account_address, std::string const & hex_state_data);

    statectx::xstatectx_face_ptr_t m_statectx_ptr;
    base::xvaccount_t m_table_account;  // todo use xtable_address_t
};

using xstate_tablestate_reseter_base_ptr = std::unique_ptr<xstate_tablestate_reseter_base>;

NS_END2