// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/xevm_storage.h"

#include "xbasic/xhex.h"
#include "xcommon/xaccount_address_fwd.h"
#include "xdata/xproperty.h"
#include "xevm_common/common_data.h"
#include "xstate_accessor/xproperties/xproperty_identifier.h"
#include "xstate_accessor/xstate_accessor.h"

NS_BEG3(top, contract_runtime, evm)

const state_accessor::properties::xproperty_type_t evm_property_type_bytes = state_accessor::properties::xproperty_type_t::bytes;
const state_accessor::properties::xproperty_type_t evm_property_type_map = state_accessor::properties::xproperty_type_t::map;

xbytes_t xtop_evm_storage::storage_get(xbytes_t const & key) {
    xassert(m_statectx != nullptr);
    auto storage_key = decode_key_type(key);

    try {
        auto unit_state = m_statectx->load_unit_state(storage_key.address);
        assert(unit_state);
        if (!unit_state) {
            return {};
        }
        auto state_observer = make_observer(unit_state->get_bstate().get());
        auto canvas = unit_state->get_canvas();
        state_accessor::xstate_accessor_t sa{state_observer, canvas};
        std::error_code ec;

        if (storage_key.key_type == storage_key_type::Nonce) {
            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_TX_INFO, state_accessor::properties::xproperty_category_t::system};
            auto value = sa.get_property_cell_value<evm_property_type_map>(property, data::XPROPERTY_TX_INFO_LATEST_SENDTX_NUM, ec);  // uint64_t in string
            auto value_uint64 = base::xstring_utl::touint64(top::to_string(value));
            assert(!ec);
            top::error::throw_error(ec);

            xdbg("storage_get get nonce account:%s, nonce:%llu", storage_key.address.c_str(), value_uint64);
            xbytes_t result(8);
            evm_common::toBigEndian(value_uint64, result);
            return result;

        } else if (storage_key.key_type == storage_key_type::Balance) {
            return unit_state->tep_token_balance_bytes(common::xtoken_id_t::eth);
        } else if (storage_key.key_type == storage_key_type::Code) {
            // todo add contract_manager lru cache.
            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_CODE, state_accessor::properties::xproperty_category_t::system};
            auto value = sa.get_property<evm_property_type_bytes>(property, ec);
            assert(!ec);
            top::error::throw_error(ec);
            xdbg("storage_get get code account:%s, size:%zu", storage_key.address.c_str(), value.size());
            return value;

        } else if (storage_key.key_type == storage_key_type::Storage) {
            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_STORAGE, state_accessor::properties::xproperty_category_t::system};
            auto value = sa.get_property_cell_value<evm_property_type_map>(property, storage_key.extra_key, ec);
            assert(!ec);
            xdbg("storage_get storage:%s,%s,value:%s", storage_key.address.c_str(), storage_key.extra_key.c_str(), top::to_hex(value).c_str());
            top::error::throw_error(ec);
            return value;

        } else if (storage_key.key_type == storage_key_type::Generation) {
            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_GENERATION, state_accessor::properties::xproperty_category_t::system};
            auto value = sa.get_property<evm_property_type_bytes>(property, ec);
            assert(!ec);
            top::error::throw_error(ec);
            return value;

        } else {
            xassert(false);
        }

    } catch (top::error::xtop_error_t const & eh) {
        xwarn("[EVM][storage_get] catch category %s, errc %" PRIi32 " msg %s", eh.category().name(), eh.code().value(), eh.what());
    } catch (std::exception const & eh) {
        xwarn("[EVM][storage_get] catch std::exception %s", eh.what());
    } catch (enum_xerror_code ec) {
        xwarn("[EVM][storage_get] catch xbase error %d", ec);
    } catch (...) {
        xwarn("[EVM][storage_get] catch unknonw error");
    }
    return {};
}
void xtop_evm_storage::storage_set(xbytes_t const & key, xbytes_t const & value) {
    xassert(m_statectx != nullptr);
    auto storage_key = decode_key_type(key);

    try {
        auto unit_state = m_statectx->load_unit_state(storage_key.address);
        assert(unit_state);
        if (!unit_state) {
            return;
        }
        auto state_observer = make_observer(unit_state->get_bstate().get());
        auto canvas = unit_state->get_canvas();
        state_accessor::xstate_accessor_t sa{state_observer, canvas};
        std::error_code ec;

        if (storage_key.key_type == storage_key_type::Nonce) {
            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_TX_INFO, state_accessor::properties::xproperty_category_t::system};

            assert(value.size() == 8);
            uint64_t nonce_u64 = 0;
            nonce_u64 = evm_common::fromBigEndian<uint64_t>(value);

            auto nonce_bytes = top::to_bytes(nonce_u64);
            xdbg("storage_set set nonce account:%s, nonce:%llu", storage_key.address.c_str(), nonce_u64);
            sa.set_property_cell_value<evm_property_type_map>(property, data::XPROPERTY_TX_INFO_LATEST_SENDTX_NUM, nonce_bytes, ec);
            assert(!ec);
            top::error::throw_error(ec);

        } else if (storage_key.key_type == storage_key_type::Balance) {
#ifdef DEBUG
            evm_common::u256 balance = evm_common::fromBigEndian<evm_common::u256>(value);
            xdbg("storage_set set balance account:%s, balance:%s", storage_key.address.c_str(), balance.str().c_str());
#endif
            unit_state->set_tep_balance_bytes(common::xtoken_id_t::eth, value);

        } else if (storage_key.key_type == storage_key_type::Code) {
            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_CODE, state_accessor::properties::xproperty_category_t::system};
            sa.set_property<evm_property_type_bytes>(property, value, ec);
            assert(!ec);
            top::error::throw_error(ec);

        } else if (storage_key.key_type == storage_key_type::Storage) {
            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_STORAGE, state_accessor::properties::xproperty_category_t::system};
            sa.set_property_cell_value<evm_property_type_map>(property, storage_key.extra_key, value, ec);
            xdbg("storage_set storage:%s,%s,value:%s", storage_key.address.c_str(), storage_key.extra_key.c_str(), top::to_hex(value).c_str());
            assert(!ec);
            top::error::throw_error(ec);

        } else if (storage_key.key_type == storage_key_type::Generation) {
            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_GENERATION, state_accessor::properties::xproperty_category_t::system};
            sa.set_property<evm_property_type_bytes>(property, value, ec);
            assert(!ec);
            top::error::throw_error(ec);

        } else {
            xassert(false);
        }

    } catch (top::error::xtop_error_t const & eh) {
        xwarn("[EVM][storage_set] catch category %s, errc %" PRIi32 " msg %s", eh.category().name(), eh.code().value(), eh.what());
    } catch (std::exception const & eh) {
        xwarn("[EVM][storage_set] catch std::exception %s", eh.what());
    } catch (enum_xerror_code ec) {
        xwarn("[EVM][storage_set] catch xbase error %d", ec);
    } catch (...) {
        xwarn("[EVM][storage_set] catch unknonw error");
    }
    return;
}

void xtop_evm_storage::storage_remove(xbytes_t const & key) {
    xassert(m_statectx != nullptr);
    auto storage_key = decode_key_type(key);

    try {
        auto unit_state = m_statectx->load_unit_state(storage_key.address);
        assert(unit_state);
        if (!unit_state) {
            return;
        }
        auto state_observer = make_observer(unit_state->get_bstate().get());
        auto canvas = unit_state->get_canvas();
        state_accessor::xstate_accessor_t sa{state_observer, canvas};
        std::error_code ec;

        if (storage_key.key_type == storage_key_type::Nonce) {
            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_TX_INFO, state_accessor::properties::xproperty_category_t::system};

            uint64_t nonce_uint64 = 0;
            auto nonce = base::xstring_utl::tostring(nonce_uint64);
            auto nonce_bytes = top::to_bytes(nonce_uint64);
            sa.set_property_cell_value<evm_property_type_map>(property, data::XPROPERTY_TX_INFO_LATEST_SENDTX_NUM, nonce_bytes, ec);
            assert(!ec);
            top::error::throw_error(ec);

        } else if (storage_key.key_type == storage_key_type::Balance) {
            evm_common::u256 value{0};
            unit_state->set_tep_balance(common::xtoken_id_t::eth, value);

        } else if (storage_key.key_type == storage_key_type::Code) {
            auto typeless_property =
                state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_CODE, state_accessor::properties::xproperty_category_t::system};
            auto property = state_accessor::properties::xproperty_identifier_t{typeless_property, evm_property_type_bytes};
            sa.clear_property(property, ec);
            assert(!ec);
            top::error::throw_error(ec);

        } else if (storage_key.key_type == storage_key_type::Storage) {
            auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_STORAGE, state_accessor::properties::xproperty_category_t::system};
            sa.remove_property_cell<evm_property_type_map>(property, storage_key.extra_key, ec);
            xdbg("storage_remove storage:%s,%s", storage_key.address.c_str(), storage_key.extra_key.c_str());
            // sa.clear_property(property, ec);
            assert(!ec);
            top::error::throw_error(ec);

        } else if (storage_key.key_type == storage_key_type::Generation) {
            auto typeless_property =
                state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_GENERATION, state_accessor::properties::xproperty_category_t::system};
            auto property = state_accessor::properties::xproperty_identifier_t{typeless_property, evm_property_type_bytes};
            sa.clear_property(property, ec);
            assert(!ec);
            top::error::throw_error(ec);

        } else {
            xassert(false);
        }

    } catch (top::error::xtop_error_t const & eh) {
        xwarn("[EVM][storage_remove] catch category %s, errc %" PRIi32 " msg %s", eh.category().name(), eh.code().value(), eh.what());
    } catch (std::exception const & eh) {
        xwarn("[EVM][storage_remove] catch std::exception %s", eh.what());
    } catch (enum_xerror_code ec) {
        xwarn("[EVM][storage_remove] catch xbase error %d", ec);
    } catch (...) {
        xwarn("[EVM][storage_remove] catch unknonw error");
    }
}

NS_END3