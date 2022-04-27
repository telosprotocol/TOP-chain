#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xcommon/xaccount_address_fwd.h"
#include "xdata/xproperty.h"
#include "xevm_common/common_data.h"
#include "xevm_common/xborsh.hpp"
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

        try {
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
                assert(!ec);
                top::error::throw_error(ec);

                evm_common::u256 value_u256{value_uint64};
                evm_common::xBorshEncoder encoder;
                encoder.EncodeInteger(value_u256);
                xbytes_t result = encoder.GetBuffer();
                assert(result.size() == 32);
                return result;

            } else if (storage_key.key_type == storage_key_type::Balance) {
                return unit_state->tep_token_balance_bytes(data::XPROPERTY_TEP1_BALANCE_KEY, data::XPROPERTY_ASSET_ETH);
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
                auto property =
                    state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_GENERATION, state_accessor::properties::xproperty_category_t::system};
                auto value = sa.get_property_cell_value<evm_property_type_map>(property, storage_key.extra_key, ec);
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
    void storage_set(xbytes_t const & key, xbytes_t const & value) override {
        xassert(m_statectx != nullptr);
        auto storage_key = decode_key_type(key);

        try {
            auto unit_state = m_statectx->load_unit_state(storage_key.address);
            assert(unit_state);  // todo return result = {} empty
            auto state_observer = make_observer(unit_state->get_bstate().get());
            auto canvas = unit_state->get_canvas();
            state_accessor::xstate_accessor_t sa{state_observer, canvas};
            std::error_code ec;

            if (storage_key.key_type == storage_key_type::Nonce) {
                auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_TX_INFO, state_accessor::properties::xproperty_category_t::system};

                assert(value.size() == 32);
                evm_common::u256 value_u256;
                evm_common::xBorshDecoder decoder;
                decoder.getInteger(value, value_u256);

                auto nonce_uint64 = value_u256.convert_to<uint64_t>();  // if overflow?
                auto nonce_bytes = top::to_bytes(nonce_uint64);
                xdbg("storage_set set nonce account:%s, nonce:%llu", storage_key.address.c_str(), nonce_uint64);
                sa.set_property_cell_value<evm_property_type_map>(property, data::XPROPERTY_TX_INFO_LATEST_SENDTX_NUM, nonce_bytes, ec);
                assert(!ec);
                top::error::throw_error(ec);

            } else if (storage_key.key_type == storage_key_type::Balance) {
                assert(value.size() == 32);
                unit_state->set_tep_balance_bytes(data::XPROPERTY_TEP1_BALANCE_KEY, data::XPROPERTY_ASSET_ETH, value);

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
                auto property =
                    state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_GENERATION, state_accessor::properties::xproperty_category_t::system};
                sa.set_property_cell_value<evm_property_type_map>(property, storage_key.extra_key, value, ec);
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

    void storage_remove(xbytes_t const & key) override {
        xassert(m_statectx != nullptr);
        auto storage_key = decode_key_type(key);

        try {
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

                // auto property = state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_EVM_BALANCE, state_accessor::properties::xproperty_category_t::system};
                // // sa.set_property<evm_property_type_bytes>(property, value, ec);
                // sa.set_property<evm_property_type_bytes>(property, {}, ec);
                // assert(!ec);
                // top::error::throw_error(ec);

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

private:
    statectx::xstatectx_face_ptr_t m_statectx;
};
using xevm_storage = xtop_evm_storage;

NS_END3