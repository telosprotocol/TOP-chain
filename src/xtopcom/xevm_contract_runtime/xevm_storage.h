#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xcommon/xaccount_address_fwd.h"
#include "xdata/xproperty.h"
#include "xevm_contract_runtime/xevm_storage_base.h"
#include "xstate_accessor/xproperties/xproperty_identifier.h"

NS_BEG3(top, contract_runtime, evm)

const state_accessor::properties::xproperty_type_t evm_property_type_bytes = state_accessor::properties::xproperty_type_t::bytes;
const state_accessor::properties::xproperty_type_t evm_property_type_map = state_accessor::properties::xproperty_type_t::map;

class xtop_evm_storage : public xevm_storage_base_t {
public:
    explicit xtop_evm_storage(statectx::xstatectx_face_ptr_t const statectx) : m_statectx{statectx} {
    }
    xtop_evm_storage(xtop_evm_storage const &) = delete;
    xtop_evm_storage & operator=(xtop_evm_storage const &) = delete;
    xtop_evm_storage(xtop_evm_storage &&) = default;
    xtop_evm_storage & operator=(xtop_evm_storage &&) = default;
    ~xtop_evm_storage() = default;

    xbytes_t storage_get(xbytes_t const & key) override {
        xassert(m_statectx != nullptr);
        auto storage_key = decode_key_type(key);

        // try { // todo try catch sa/ubs error and return result = {} empty
        // iii_storage_get(storage_key.key_type, storage_key.)
        // }
        // catch (){
        // ...
        // }
        auto unit_state = m_statectx->load_unit_state(storage_key.address);
        assert(unit_state);  // todo return result = {} empty
        auto state_observer = make_observer(unit_state->get_bstate().get());
        auto canvas = unit_state->get_canvas();
        state_accessor::xstate_accessor_t sa{state_observer, canvas};
        std::error_code ec;

        if (storage_key.key_type == storage_key_type::Nonce) {
            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_TX_INFO, state_accessor::properties::xproperty_category_t::system};
            auto value = sa.get_property_cell_value<evm_property_type_map>(property, data::XPROPERTY_TX_INFO_LATEST_SENDTX_NUM, ec);  // uint64_t in string
            auto value_uint64 = base::xstring_utl::touint64(top::to_string(value));
            // todo u64 -> u256 than serilize to bytes(BE/LE)
            assert(!ec);
            top::error::throw_error(ec);
            return value;

        } else if (storage_key.key_type == storage_key_type::Balance) {
            auto value_uint64 = unit_state->tep_balance(data::XPROPERTY_ASSET_ETH);
            // todo u64 -> u256 than serilize to bytes(BE/LE)

            // todo delete this
            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_BALANCE, state_accessor::properties::xproperty_category_t::system};
            auto value = sa.get_property<evm_property_type_bytes>(property, ec);
            assert(!ec);
            top::error::throw_error(ec);
            return value;

        } else if (storage_key.key_type == storage_key_type::Code) {
            // todo add contract_manager lru cache.
            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_CODE, state_accessor::properties::xproperty_category_t::system};
            auto value = sa.get_property<evm_property_type_bytes>(property, ec);
            assert(!ec);
            top::error::throw_error(ec);
            return value;

        } else if (storage_key.key_type == storage_key_type::Storage) {
            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_STORAGE, state_accessor::properties::xproperty_category_t::system};
            auto value = sa.get_property_cell_value<evm_property_type_map>(property, storage_key.extra_key, ec);
            assert(!ec);
            top::error::throw_error(ec);
            return value;

        } else if (storage_key.key_type == storage_key_type::Generation) {
            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_GENERATION, state_accessor::properties::xproperty_category_t::system};
            auto value = sa.get_property_cell_value<evm_property_type_map>(property, storage_key.extra_key, ec);
            assert(!ec);
            top::error::throw_error(ec);
            return value;

        } else {
            xassert(false);
        }
        return {};
    }
    void storage_set(xbytes_t const & key, xbytes_t const & value) override {
        xassert(m_statectx != nullptr);
        auto storage_key = decode_key_type(key);

        // try { // todo try catch sa/ubs error and return result = {} empty
        // iii_storage_get(storage_key.key_type, storage_key.)
        // }
        // catch (){
        // ...
        // }

        auto unit_state = m_statectx->load_unit_state(storage_key.address);
        assert(unit_state);  // todo return result = {} empty
        auto state_observer = make_observer(unit_state->get_bstate().get());
        auto canvas = unit_state->get_canvas();
        state_accessor::xstate_accessor_t sa{state_observer, canvas};
        std::error_code ec;

        if (storage_key.key_type == storage_key_type::Nonce) {
            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_TX_INFO, state_accessor::properties::xproperty_category_t::system};

            // todo bytes(BE/LE) -> u256 -> u64 to set.
            // auto value_u256;
            auto nonce_uint64 = base::xstring_utl::touint64(top::to_string(value));
            auto nonce = base::xstring_utl::tostring(nonce_uint64);
            auto nonce_bytes = top::to_bytes(nonce_uint64);
            sa.set_property_cell_value<evm_property_type_map>(property, data::XPROPERTY_TX_INFO_LATEST_SENDTX_NUM, nonce_bytes, ec);
            assert(!ec);
            top::error::throw_error(ec);

        } else if (storage_key.key_type == storage_key_type::Balance) {
            // todo bytes(BE/LE) -> u256 -> u64 to set.
            //? how unit_bstate to set balance?

            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_BALANCE, state_accessor::properties::xproperty_category_t::system};
            sa.set_property<evm_property_type_bytes>(property, value, ec);
            assert(!ec);
            top::error::throw_error(ec);

        } else if (storage_key.key_type == storage_key_type::Code) {
            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_CODE, state_accessor::properties::xproperty_category_t::system};
            sa.set_property<evm_property_type_bytes>(property, value, ec);
            assert(!ec);
            top::error::throw_error(ec);

        } else if (storage_key.key_type == storage_key_type::Storage) {
            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_STORAGE, state_accessor::properties::xproperty_category_t::system};
            sa.set_property_cell_value<evm_property_type_map>(property, storage_key.extra_key, value, ec);
            assert(!ec);
            top::error::throw_error(ec);

        } else if (storage_key.key_type == storage_key_type::Generation) {
            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_GENERATION, state_accessor::properties::xproperty_category_t::system};
            sa.set_property_cell_value<evm_property_type_map>(property, storage_key.extra_key, value, ec);
            assert(!ec);
            top::error::throw_error(ec);

        } else {
            xassert(false);
        }
        return;
    }

    void storage_remove(xbytes_t const & key) override {
        xassert(m_statectx != nullptr);
        auto storage_key = decode_key_type(key);

        auto unit_state = m_statectx->load_unit_state(storage_key.address);
        assert(unit_state);  // todo return result = {} empty
        auto state_observer = make_observer(unit_state->get_bstate().get());
        auto canvas = unit_state->get_canvas();
        state_accessor::xstate_accessor_t sa{state_observer, canvas};
        std::error_code ec;

        if (storage_key.key_type == storage_key_type::Nonce) {
            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_TX_INFO, state_accessor::properties::xproperty_category_t::system};

            // todo bytes(BE/LE) -> u256 -> u64 to set.
            // auto value_u256;
            uint64_t nonce_uint64 = 0;
            auto nonce = base::xstring_utl::tostring(nonce_uint64);
            auto nonce_bytes = top::to_bytes(nonce_uint64);
            sa.set_property_cell_value<evm_property_type_map>(property, data::XPROPERTY_TX_INFO_LATEST_SENDTX_NUM, nonce_bytes, ec);
            assert(!ec);
            top::error::throw_error(ec);

        } else if (storage_key.key_type == storage_key_type::Balance) {
            // todo bytes(BE/LE) -> u256 -> u64 to set.
            //? how unit_bstate to remove balance?

            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_BALANCE, state_accessor::properties::xproperty_category_t::system};
            // sa.set_property<evm_property_type_bytes>(property, value, ec);
            sa.set_property<evm_property_type_bytes>(property, {}, ec);
            assert(!ec);
            top::error::throw_error(ec);

        } else if (storage_key.key_type == storage_key_type::Code) {
            auto typeless_property =
                state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_CODE, state_accessor::properties::xproperty_category_t::system};
            auto property = state_accessor::properties::xproperty_identifier_t{typeless_property, evm_property_type_bytes};
            sa.clear_property(property, ec);
            assert(!ec);
            top::error::throw_error(ec);

        } else if (storage_key.key_type == storage_key_type::Storage) {
            auto typeless_property =
                state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_STORAGE, state_accessor::properties::xproperty_category_t::system};
            auto property = state_accessor::properties::xproperty_identifier_t{typeless_property, evm_property_type_map};
            sa.clear_property(property, ec);
            assert(!ec);
            top::error::throw_error(ec);

        } else if (storage_key.key_type == storage_key_type::Generation) {
            auto typeless_property =
                state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_GENERATION, state_accessor::properties::xproperty_category_t::system};
            auto property = state_accessor::properties::xproperty_identifier_t{typeless_property, evm_property_type_map};
            sa.clear_property(property, ec);
            assert(!ec);
            top::error::throw_error(ec);

        } else {
            xassert(false);
        }
    }

private:
    statectx::xstatectx_face_ptr_t m_statectx;
};
using xevm_storage = xtop_evm_storage;

NS_END3