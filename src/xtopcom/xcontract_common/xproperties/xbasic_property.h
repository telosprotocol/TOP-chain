// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xcommon/xaddress.h"
#include "xcontract_common/xcontract_fwd.h"
#include "xcontract_common/xcontract_state.h"
#include "xstate_accessor/xproperties/xproperty_identifier.h"


NS_BEG3(top, contract_common, properties)

class xtop_basic_property {
    observer_ptr<xcontract_face_t> m_associated_contract{ nullptr };
    std::unique_ptr<xcontract_state_t> m_state_owned{nullptr};
    observer_ptr<xcontract_state_t> m_state{nullptr};
    state_accessor::properties::xproperty_identifier_t m_id;
    common::xaccount_address_t m_owner;

public:
    xtop_basic_property(xtop_basic_property const &) = delete;
    xtop_basic_property & operator=(xtop_basic_property const &) = delete;
    xtop_basic_property(xtop_basic_property &&) = default;
    xtop_basic_property & operator=(xtop_basic_property &&) = default;
    virtual ~xtop_basic_property() = default;

protected:
    xtop_basic_property() = default;

    xtop_basic_property(std::string const & name,
                        state_accessor::properties::xproperty_type_t type,
                        observer_ptr<xcontract_face_t> associated_contract) noexcept;

    xtop_basic_property(std::string const & name,
                        state_accessor::properties::xproperty_type_t type,
                        std::unique_ptr<xcontract_state_t> state_owned);

public:
    void initialize();

    state_accessor::properties::xproperty_identifier_t const & id() const;
    state_accessor::properties::xtypeless_property_identifier_t typeless_id() const;
    common::xaccount_address_t owner() const;
    common::xaccount_address_t accessor() const;

protected:
    observer_ptr<xcontract_state_t const> associated_state() const noexcept;
    observer_ptr<xcontract_state_t> associated_state() noexcept;
};
using xbasic_property_t = xtop_basic_property;

NS_END3
