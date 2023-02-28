// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtvm_runtime/xtvm_storage.h"

#include "xstate_accessor/xproperties/xproperty_identifier.h"
#include "xstate_accessor/xstate_accessor.h"

#include <cassert>

NS_BEG2(top, tvm)

const state_accessor::properties::xproperty_type_t tvm_property_type_bytes = state_accessor::properties::xproperty_type_t::bytes;
const state_accessor::properties::xproperty_type_t tvm_property_type_map = state_accessor::properties::xproperty_type_t::map;

// todo ! should `data::XPROPERTY_EVM_CODE` && `data::XPROPERTY_EVM_STORAGE` use new property name.
xbytes_t xtop_vm_storage::storage_get(xbytes_t const & key) {
    assert(m_statectx);

    auto storage_key = decode_storage_key(key);
    auto address = common::xaccount_address_t{storage_key.t8_address()};

    try {
        auto account_state = m_statectx->load_account_state(address);
        if (nullptr == account_state) {
            xassert(false);
            return {};
        }
        auto unit_state = account_state->get_unitstate();
        auto state_observer = make_observer(unit_state->get_bstate().get());
        auto canvas = unit_state->get_canvas();
        state_accessor::xstate_accessor_t sa{state_observer, canvas};
        std::error_code ec;

        switch (storage_key.key_type()) {
        case storage_key_prefix::Nonce: {
            uint64_t nonce_u64 = account_state->get_tx_nonce();
            xdbg("tvm_storage get address %s nonce %llu", address.to_string().c_str(), nonce_u64);
            xbytes_t nonce_bytes(8);
            evm_common::toBigEndian(nonce_u64, nonce_bytes);
            return nonce_bytes;
        }
        case storage_key_prefix::Balance: {
            uint64_t balance_u64 = account_state->get_unitstate()->balance();
            xdbg("tvm_storage get address %s balance %llu", address.to_string().c_str(), balance_u64);
            xbytes_t balance_bytes(8);
            evm_common::toBigEndian(balance_u64, balance_bytes);
            return balance_bytes;
        }
        case storage_key_prefix::Code: {
            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_CODE, state_accessor::properties::xproperty_category_t::system};
            auto value = sa.get_property<tvm_property_type_bytes>(property, ec);
            assert(!ec);
            top::error::throw_error(ec);
            return value;
        }
        case storage_key_prefix::Storage: {
            if (!storage_key.has_storage_key()) {
                // remove all storage, but read first
                // just return;
                return xbytes_t{};
            }
            assert(storage_key.has_storage_key());
            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_STORAGE, state_accessor::properties::xproperty_category_t::system};
            auto value = sa.get_property_cell_value<tvm_property_type_map>(property, storage_key.storage_key(), ec);
            assert(!ec);
            xdbg("tvm_storage get address %s storage_key %s", address.to_string().c_str(), top::to_hex(storage_key.storage_key()).c_str());
            top::error::throw_error(ec);
            return value;
        }
        default: {
            xassert(false);  // NOLINT(clang-diagnostic-disabled-macro-expansion)
            __builtin_unreachable();
        }
        }
    } catch (top::error::xtop_error_t const & eh) {
        xwarn("tvm_storage get catch category %s, errc %" PRIi32 " msg %s", eh.category().name(), eh.code().value(), eh.what());
    } catch (std::exception const & eh) {
        xwarn("tvm_storage get catch std::exception %s", eh.what());
    } catch (enum_xerror_code ec) {
        xwarn("tvm_storage get catch xbase error %d", ec);
    } catch (...) {
        xwarn("tvm_storage get catch unknonw error");
    }
    return xbytes_t{};
}
void xtop_vm_storage::storage_set(xbytes_t const & key, xbytes_t const & value) {
    assert(m_statectx);

    auto storage_key = decode_storage_key(key);
    auto address = common::xaccount_address_t{storage_key.t8_address()};

    try {
        auto account_state = m_statectx->load_account_state(address);
        if (nullptr == account_state) {
            xassert(false);
            return;
        }
        auto unit_state = account_state->get_unitstate();
        auto state_observer = make_observer(unit_state->get_bstate().get());
        auto canvas = unit_state->get_canvas();
        state_accessor::xstate_accessor_t sa{state_observer, canvas};
        std::error_code ec;

        switch (storage_key.key_type()) {
        case storage_key_prefix::Nonce: {
            uint64_t nonce_u64 = evm_common::fromBigEndian<uint64_t, xbytes_t>(value);
            xdbg("tvm_storage set address %s nonce %llu", address.to_string().c_str(), nonce_u64);
            account_state->set_tx_nonce(nonce_u64);
            return;
        }
        case storage_key_prefix::Balance: {
            uint64_t balance_u64 = evm_common::fromBigEndian<uint64_t, xbytes_t>(value);
            xdbg("tvm_storage set address %s balance %llu", address.to_string().c_str(), balance_u64);
            account_state->get_unitstate()->set_token_balance(data::XPROPERTY_BALANCE_AVAILABLE, base::vtoken_t(balance_u64));
            return;
        }
        case storage_key_prefix::Code: {
            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_CODE, state_accessor::properties::xproperty_category_t::system};
            sa.set_property<tvm_property_type_bytes>(property, value, ec);
            assert(!ec);
            top::error::throw_error(ec);
            return;
        }
        case storage_key_prefix::Storage: {
            assert(storage_key.has_storage_key());
            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_STORAGE, state_accessor::properties::xproperty_category_t::system};
            sa.set_property_cell_value<tvm_property_type_map>(property, storage_key.storage_key(), value, ec);
            assert(!ec);
            xdbg("tvm_storage set address %s storage_key %s", address.to_string().c_str(), top::to_hex(storage_key.storage_key()).c_str());
            top::error::throw_error(ec);
            return;
        }
        default: {
            xassert(false);  // NOLINT(clang-diagnostic-disabled-macro-expansion)
            __builtin_unreachable();
        }
        }
    } catch (top::error::xtop_error_t const & eh) {
        xwarn("tvm_storage set catch category %s, errc %" PRIi32 " msg %s", eh.category().name(), eh.code().value(), eh.what());
    } catch (std::exception const & eh) {
        xwarn("tvm_storage set catch std::exception %s", eh.what());
    } catch (enum_xerror_code ec) {
        xwarn("tvm_storage set catch xbase error %d", ec);
    } catch (...) {
        xwarn("tvm_storage set catch unknonw error");
    }
}
void xtop_vm_storage::storage_remove(xbytes_t const & key) {
    assert(m_statectx);

    auto storage_key = decode_storage_key(key);
    auto address = common::xaccount_address_t{storage_key.t8_address()};

    try {
        auto account_state = m_statectx->load_account_state(address);
        if (nullptr == account_state) {
            xassert(false);
            return;
        }
        auto unit_state = account_state->get_unitstate();
        auto state_observer = make_observer(unit_state->get_bstate().get());
        auto canvas = unit_state->get_canvas();
        state_accessor::xstate_accessor_t sa{state_observer, canvas};
        std::error_code ec;

        switch (storage_key.key_type()) {
        case storage_key_prefix::Nonce: {
            uint64_t nonce_u64 = 0;
            xdbg("tvm_storage remove address %s nonce to %llu", address.to_string().c_str(), nonce_u64);
            account_state->set_tx_nonce(nonce_u64);
            return;
        }
        case storage_key_prefix::Balance: {
            uint64_t balance_u64 = 0;
            xdbg("tvm_storage remove address %s balance to %llu", address.to_string().c_str(), balance_u64);
            account_state->get_unitstate()->set_token_balance(data::XPROPERTY_BALANCE_AVAILABLE, base::vtoken_t(balance_u64));
            return;
        }
        case storage_key_prefix::Code: {
            auto typeless_property =
                state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_CODE, state_accessor::properties::xproperty_category_t::system};
            auto property = state_accessor::properties::xproperty_identifier_t{typeless_property, tvm_property_type_bytes};
            sa.clear_property(property, ec);
            assert(!ec);
            top::error::throw_error(ec);
            return;
        }
        case storage_key_prefix::Storage: {
            if (storage_key.has_storage_key()) {
                auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_STORAGE, state_accessor::properties::xproperty_category_t::system};
                sa.remove_property_cell<tvm_property_type_map>(property, storage_key.storage_key(), ec);
                if (ec == static_cast<std::error_code>(state_accessor::error::xerrc_t::property_key_not_exist) ||
                    ec == static_cast<std::error_code>(state_accessor::error::xerrc_t::property_not_exist)) {
                    ec.clear();  // Is possible that key not exist when deploying contract, since evm will always try to remove zero value.
                }
                xdbg("tvm_storage remove address %s storage:%s", address.to_string().c_str(), top::to_hex(storage_key.storage_key()).c_str());
            } else {
                auto typeless_property =
                    state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_STORAGE, state_accessor::properties::xproperty_category_t::system};
                auto property = state_accessor::properties::xproperty_identifier_t{typeless_property, tvm_property_type_map};
                sa.clear_property(property, ec);
                if (ec == static_cast<std::error_code>(state_accessor::error::xerrc_t::property_not_exist)) {
                    ec.clear();
                }
                xdbg("tvm_storage remove address %s all storage", address.to_string().c_str());
            }
            assert(!ec);
            top::error::throw_error(ec);
            return;
        }
        default: {
            xassert(false);  // NOLINT(clang-diagnostic-disabled-macro-expansion)
            __builtin_unreachable();
        }
        }
    } catch (top::error::xtop_error_t const & eh) {
        xwarn("tvm_storage remove catch category %s, errc %" PRIi32 " msg %s", eh.category().name(), eh.code().value(), eh.what());
    } catch (std::exception const & eh) {
        xwarn("tvm_storage remove catch std::exception %s", eh.what());
    } catch (enum_xerror_code ec) {
        xwarn("tvm_storage remove catch xbase error %d", ec);
    } catch (...) {
        xwarn("tvm_storage remove catch unknonw error");
    }
}
NS_END2