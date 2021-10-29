// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xproperties/xbasic_property.h"

#include "xbasic/xutility.h"
#include "xcontract_common/xcontract_face.h"

NS_BEG3(top, contract_common, properties)

static state_accessor::properties::xproperty_category_t lookup_property_category(std::string const & name,
                                                                                 state_accessor::properties::xproperty_type_t const type) {
    static std::unordered_map<std::string, std::unordered_map<state_accessor::properties::xproperty_type_t, state_accessor::properties::xproperty_category_t>> const dict{
        {data::XPROPERTY_BALANCE_AVAILABLE, {{state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xproperty_category_t::system}}},
        {data::XPROPERTY_BALANCE_BURN, {{state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xproperty_category_t::system}}},
        {data::XPROPERTY_BALANCE_LOCK, {{state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xproperty_category_t::system}}},
        {data::XPROPERTY_BALANCE_PLEDGE_TGAS, {{state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xproperty_category_t::system}}},
        {data::XPROPERTY_BALANCE_PLEDGE_VOTE, {{state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xproperty_category_t::system}}},
        {data::XPROPERTY_LOCK_TGAS, {{state_accessor::properties::xproperty_type_t::uint64, state_accessor::properties::xproperty_category_t::system}}},
        {data::XPROPERTY_USED_TGAS_KEY, {{state_accessor::properties::xproperty_type_t::string, state_accessor::properties::xproperty_category_t::system}}},
        {data::XPROPERTY_LAST_TX_HOUR_KEY, {{state_accessor::properties::xproperty_type_t::string, state_accessor::properties::xproperty_category_t::system}}},
        {data::XPROPERTY_PLEDGE_VOTE_KEY, {{state_accessor::properties::xproperty_type_t::map, state_accessor::properties::xproperty_category_t::system}}},
        {data::XPROPERTY_EXPIRE_VOTE_TOKEN_KEY, {{state_accessor::properties::xproperty_type_t::string, state_accessor::properties::xproperty_category_t::system}}},
        {data::XPROPERTY_UNVOTE_NUM, {{state_accessor::properties::xproperty_type_t::uint64, state_accessor::properties::xproperty_category_t::system}}},
        {data::XPROPERTY_TX_INFO, {{state_accessor::properties::xproperty_type_t::map, state_accessor::properties::xproperty_category_t::system}}},
        {data::XPROPERTY_ACCOUNT_CREATE_TIME, {{state_accessor::properties::xproperty_type_t::uint64, state_accessor::properties::xproperty_category_t::system}}},
        {data::XPROPERTY_LOCK_TOKEN_KEY, {{state_accessor::properties::xproperty_type_t::map, state_accessor::properties::xproperty_category_t::system}}}
    };

    state_accessor::properties::xproperty_category_t property_category{state_accessor::properties::xproperty_category_t::user};
    do {
        auto const it = dict.find(name);
        if (it == std::end(dict)) {
            break;
        }

        auto const & inner_dict = top::get<std::unordered_map<state_accessor::properties::xproperty_type_t, state_accessor::properties::xproperty_category_t>>(*it);
        auto const inner_it = inner_dict.find(type);
        if (inner_it == std::end(inner_dict)) {
            break;
        }

        property_category = top::get<state_accessor::properties::xproperty_category_t>(*inner_it);
    } while (false);

    return property_category;
}

xtop_basic_property::xtop_basic_property(std::string const & name, state_accessor::properties::xproperty_type_t type, observer_ptr<xcontract_face_t> associated_contract) noexcept
  : m_associated_contract{associated_contract}
  , m_id{ name, type, lookup_property_category(name, type) }
  , m_owner{ associated_contract->address() } {
    assert(m_associated_contract != nullptr);
    m_associated_contract->register_property(this);
}

xtop_basic_property::xtop_basic_property(std::string const & name, state_accessor::properties::xproperty_type_t type, std::unique_ptr<xcontract_state_t> state_owned)
  : m_state_owned{std::move(state_owned)}
  , m_state{make_observer(m_state_owned.get())}
  , m_id{name, type, lookup_property_category(name, type)}
  , m_owner{m_state_owned->state_account_address()} {
}

void xtop_basic_property::initialize() {
    assert(m_associated_contract != nullptr);
    if (m_id.category() != state_accessor::properties::xproperty_category_t::system) {
        m_associated_contract->contract_state()->create_property(m_id);
    }
}

state_accessor::properties::xproperty_identifier_t const & xtop_basic_property::id() const {
    return m_id;
}

state_accessor::properties::xtypeless_property_identifier_t xtop_basic_property::typeless_id() const {
    return static_cast<state_accessor::properties::xtypeless_property_identifier_t>(m_id);
}

common::xaccount_address_t xtop_basic_property::owner() const {
    return m_owner;
}

common::xaccount_address_t xtop_basic_property::accessor() const {
    assert(m_associated_contract != nullptr);
    return m_associated_contract->contract_state()->state_account_address();
}

observer_ptr<xcontract_state_t const> xtop_basic_property::associated_state() const noexcept {
    if (m_associated_contract != nullptr) {
        auto assoc_state = m_associated_contract->contract_state();
        if (assoc_state != nullptr) {
            return assoc_state;
        }
    }

    assert(m_state);
    return m_state;
}

observer_ptr<xcontract_state_t> xtop_basic_property::associated_state() noexcept {
    if (m_associated_contract != nullptr) {
        auto assoc_state = m_associated_contract->contract_state();
        if (assoc_state != nullptr) {
            return assoc_state;
        }
    }

    assert(m_state);
    return m_state;
}

NS_END3
