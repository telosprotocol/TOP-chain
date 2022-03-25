#pragma once

#include "xevm_runner/evm_storage_base.h"
#include "xstate_accessor/xstate_accessor.h"

namespace top {
namespace contract_runtime {
namespace evm {

class xtop_evm_storage : public top::evm::xevm_storage_base_t {
public:
    explicit xtop_evm_storage(observer_ptr<state_accessor::xstate_accessor_t> const & sa) : m_state_accessor{sa} {
    }
    xtop_evm_storage(xtop_evm_storage const &) = delete;
    xtop_evm_storage & operator=(xtop_evm_storage const &) = delete;
    xtop_evm_storage(xtop_evm_storage &&) = default;
    xtop_evm_storage & operator=(xtop_evm_storage &&) = default;
    virtual ~xtop_evm_storage() = default;

    xbytes_t storage_get(xbytes_t const & key) override {
        xassert(m_state_accessor != nullptr);
        std::error_code ec;
        auto tumple = decode_key(key);
        auto value = m_state_accessor->get_property<state_accessor::properties::xproperty_type_t::bytes>(std::get<0>(tumple), std::get<1>(tumple), ec);
        assert(!ec);
        top::error::throw_error(ec);
        return value;
    }
    void storage_set(xbytes_t const & key, xbytes_t const & value) override {
        xassert(m_state_accessor != nullptr);
        std::error_code ec;
        auto tumple = decode_key(key);
        m_state_accessor->set_property<state_accessor::properties::xproperty_type_t::bytes>(std::get<0>(tumple), value, ec);
        top::error::throw_error(ec);
        return;
    }
    void storage_remove(xbytes_t const & key) override {
    }

private:
    observer_ptr<state_accessor::xstate_accessor_t> m_state_accessor;

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