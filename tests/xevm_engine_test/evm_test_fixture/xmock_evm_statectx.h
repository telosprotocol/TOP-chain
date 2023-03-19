#pragma once
#include "xdata/xnative_contract_address.h"
#include "xdata/xunit_bstate.h"
#include "xstatectx/xstatectx_face.h"
#include "xdata/xsystem_contract/xdata_structures.h"

namespace top {

static std::string evm_to_top_address(std::string const & input) {
    if (input.substr(0, 2) == "0x") {
        return "T60004" + input.substr(2);
    }
    if (input.substr(0, 6) == "T60004") {
        return input;
    }
    return "T60004" + input;
}

namespace evm {
namespace tests {

class xmock_evm_statectx : public statectx::xstatectx_face_t {
public:
    xmock_evm_statectx() {
    }

    const data::xtablestate_ptr_t & get_table_state() const {
        return m_tablestate_ptr;
    }

    data::xunitstate_ptr_t load_unit_state(common::xaccount_address_t const& address) {
        data::xaccountstate_ptr_t accountstate = load_account_state(address);
        return accountstate->get_unitstate();
    }

    data::xaccountstate_ptr_t           load_account_state(common::xaccount_address_t const& address) override {
        if (m_mock_bstate.find(address.to_string()) == m_mock_bstate.end()) {
            top::base::xauto_ptr<top::base::xvbstate_t> bstate(new top::base::xvbstate_t(address.to_string(), 1, 1, "", "", 0, 0, 0));
            if (address == evm_eth_bridge_contract_address) {
                xobject_ptr_t<base::xvcanvas_t> canvas = make_object_ptr<base::xvcanvas_t>();
                bstate->new_string_map_var(data::system_contract::XPROPERTY_HEADERS, canvas.get());
                bstate->new_string_map_var(data::system_contract::XPROPERTY_HEADERS_SUMMARY, canvas.get());
                bstate->new_string_map_var(data::system_contract::XPROPERTY_ALL_HASHES, canvas.get());
                bstate->new_string_map_var(data::system_contract::XPROPERTY_EFFECTIVE_HASHES, canvas.get());
                bstate->new_string_var(data::system_contract::XPROPERTY_LAST_HASH, canvas.get());
                auto bytes = (evm_common::h256()).asBytes();
                bstate->load_string_var(data::system_contract::XPROPERTY_LAST_HASH)->reset({bytes.begin(), bytes.end()}, canvas.get());
            }
            auto unitstate_ptr = std::make_shared<data::xunit_bstate_t>(bstate.get(), bstate.get());
            base::xaccount_index_t aindex;
            auto accountstate_ptr = std::make_shared<data::xaccount_state_t>(unitstate_ptr, aindex);
            m_mock_bstate[address.to_string()] = accountstate_ptr;
        }
        return m_mock_bstate.at(address.to_string());
    }

    bool do_rollback() {
        return false;
    }

    size_t do_snapshot() {
        return 0;
    }

    std::string get_table_address() const {
        return table_address;
    }

    bool is_state_dirty() const {
        return true;
    }

    virtual std::map<std::string, statectx::xunitstate_ctx_ptr_t> const& get_modified_unit_ctx() const {
        return m_changed_ctxs;
    }

    // void add_balance()

    data::xtablestate_ptr_t m_tablestate_ptr;
    std::string table_address;

    std::map<std::string, data::xaccountstate_ptr_t> m_mock_bstate;
    std::map<std::string, statectx::xunitstate_ctx_ptr_t> m_changed_ctxs;
};
}  // namespace tests
}  // namespace evm
}  // namespace top
