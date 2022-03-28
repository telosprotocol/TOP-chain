#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xcommon/xaccount_address_fwd.h"
#include "xevm_runner/evm_storage_base.h"
#include "xevm_statestore_helper/xstatestore_helper.h"
#include "xstate_accessor/xproperties/xproperty_identifier.h"

namespace top {
namespace contract_runtime {
namespace evm {

class xtop_evm_storage : public top::evm::xevm_storage_base_t {
public:
    explicit xtop_evm_storage(observer_ptr<evm_statestore::xevm_statestore_helper_t> const & statestore) : m_statestore{statestore} {
    }
    xtop_evm_storage(xtop_evm_storage const &) = delete;
    xtop_evm_storage & operator=(xtop_evm_storage const &) = delete;
    xtop_evm_storage(xtop_evm_storage &&) = default;
    xtop_evm_storage & operator=(xtop_evm_storage &&) = default;
    virtual ~xtop_evm_storage() = default;

    xbytes_t storage_get(xbytes_t const & key) override {
        xassert(m_statestore != nullptr);
        std::error_code ec;
        auto tumple = decode_key(key);
        xbytes_t value;
        // get state and value from statestore
        // auto value = m_state_accessor->get_property<state_accessor::properties::xproperty_type_t::bytes>(std::get<0>(tumple), std::get<1>(tumple), ec);
        assert(!ec);
        top::error::throw_error(ec);
        return value;
    }
    void storage_set(xbytes_t const & key, xbytes_t const & value) override {
        xassert(m_statestore != nullptr);
        std::error_code ec;
        auto tumple = decode_key(key);
        // set state and value through statestore
        // m_state_accessor->set_property<state_accessor::properties::xproperty_type_t::bytes>(std::get<0>(tumple), value, ec);
        top::error::throw_error(ec);
        return;
    }
    void storage_remove(xbytes_t const & key) override {
    }

private:
    observer_ptr<evm_statestore::xevm_statestore_helper_t> m_statestore;

    xbytes_t encode_key(state_accessor::properties::xtypeless_property_identifier_t const & prop, common::xaccount_address_t const & account, xbytes_t const & extra) {
        return {};
    }
    std::tuple<state_accessor::properties::xtypeless_property_identifier_t, common::xaccount_address_t, xbytes_t> decode_key(xbytes_t const & raw_bytes) {
        return {};
    }
};
using xevm_storage = xtop_evm_storage;

}  // namespace evm
}  // namespace contract_runtime
}  // namespace top