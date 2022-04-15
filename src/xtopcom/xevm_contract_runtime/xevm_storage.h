#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xcommon/xaccount_address_fwd.h"
#include "xdata/xproperty.h"
#include "xevm_contract_runtime/xevm_storage_base.h"
#include "xstate_accessor/xproperties/xproperty_identifier.h"

NS_BEG3(top, contract_runtime, evm)

const state_accessor::properties::xproperty_type_t evm_property_type = state_accessor::properties::xproperty_type_t::bytes;

class xtop_evm_storage : public xevm_storage_base_t {
public:
    explicit xtop_evm_storage(statectx::xstatectx_face_ptr_t const statectx) : m_statectx{statectx} {
    }
    xtop_evm_storage(xtop_evm_storage const &) = delete;
    xtop_evm_storage & operator=(xtop_evm_storage const &) = delete;
    xtop_evm_storage(xtop_evm_storage &&) = default;
    xtop_evm_storage & operator=(xtop_evm_storage &&) = default;
    virtual ~xtop_evm_storage() = default;

    xbytes_t storage_get(xbytes_t const & key) override {
        xassert(m_statectx != nullptr);
        auto tumple = decode_key(key);
        auto unit_state = m_statectx->load_unit_state(std::get<1>(tumple).value());
        assert(unit_state); // todo err
        auto state_observer = make_observer(unit_state->get_bstate().get());
        auto canvas = unit_state->get_canvas();
        state_accessor::xstate_accessor_t sa{state_observer, canvas};
        std::error_code ec;
        auto value = sa.get_property<evm_property_type>(std::get<0>(tumple), ec);
        assert(!ec);
        top::error::throw_error(ec);
        return value;
    }
    void storage_set(xbytes_t const & key, xbytes_t const & value) override {
        xassert(m_statectx != nullptr);
        auto tumple = decode_key(key);
        auto unit_state = m_statectx->load_unit_state(std::get<1>(tumple).value());
        assert(unit_state);
        auto state_observer = make_observer(unit_state->get_bstate().get());
        auto canvas = unit_state->get_canvas();
        state_accessor::xstate_accessor_t sa{state_observer, canvas};
        std::error_code ec;
        sa.set_property<evm_property_type>(std::get<0>(tumple), value, ec);
        top::error::throw_error(ec);
        return;
    }
    void storage_remove(xbytes_t const & key) override {
        xassert(m_statectx != nullptr);
        auto tumple = decode_key(key);
        auto unit_state = m_statectx->load_unit_state(std::get<1>(tumple).value());
        assert(unit_state);
        auto state_observer = make_observer(unit_state->get_bstate().get());
        auto canvas = unit_state->get_canvas();
        state_accessor::xstate_accessor_t sa{state_observer, canvas};
        state_accessor::properties::xproperty_identifier_t property{std::get<0>(tumple), evm_property_type};
        std::error_code ec;
        sa.clear_property(property, ec);
        top::error::throw_error(ec);
        return;
    }

private:
    statectx::xstatectx_face_ptr_t m_statectx;

    std::tuple<state_accessor::properties::xtypeless_property_identifier_t, common::xaccount_address_t, xbytes_t> decode_key(xbytes_t const & raw_bytes) {
        auto storage_key = decode_key_type(raw_bytes);
        state_accessor::properties::xtypeless_property_identifier_t property{};
        switch (storage_key.key_type) {
        case storage_key_type::Nonce:
            property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_NONCE, state_accessor::properties::xproperty_category_t::system};
            break;
        case storage_key_type::Balance:
            property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_BALANCE, state_accessor::properties::xproperty_category_t::system};
            break;
        case storage_key_type::Code:
            property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_CODE, state_accessor::properties::xproperty_category_t::system};
            break;
        case storage_key_type::Storage:
            property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_STORAGE, state_accessor::properties::xproperty_category_t::system};
            break;
        case storage_key_type::Generation:
            property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_GENERATION, state_accessor::properties::xproperty_category_t::system};
            break;
        default:
            assert(false);
        }
        common::xaccount_address_t account{storage_key.address};
        xbytes_t others = top::to_bytes(storage_key.extra_key);
        return std::tuple<state_accessor::properties::xtypeless_property_identifier_t, common::xaccount_address_t, xbytes_t>{property, account, others};
    }
};
using xevm_storage = xtop_evm_storage;

NS_END3