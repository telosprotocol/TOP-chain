#pragma once
#include "xdata/xnative_contract_address.h"
#include "xdata/xunit_bstate.h"
#include "xstatectx/xstatectx_face.h"

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

    data::xunitstate_ptr_t load_unit_state(const base::xvaccount_t & addr) {
        if (m_mock_bstate.find(addr.get_account()) == m_mock_bstate.end()) {
            top::base::xauto_ptr<top::base::xvbstate_t> bstate(new top::base::xvbstate_t(addr.get_account(), 1, 1, "", "", 0, 0, 0));
            if (addr.get_account() == evm_eth_bridge_contract_address.to_string()) {
                xobject_ptr_t<base::xvcanvas_t> canvas = make_object_ptr<base::xvcanvas_t>();
                bstate->new_string_map_var(data::system_contract::XPROPERTY_HEADERS, canvas.get());
                bstate->new_string_map_var(data::system_contract::XPROPERTY_HEADERS_SUMMARY, canvas.get());
                bstate->new_string_map_var(data::system_contract::XPROPERTY_ALL_HASHES, canvas.get());
                bstate->new_string_map_var(data::system_contract::XPROPERTY_EFFECTIVE_HASHES, canvas.get());
                bstate->new_string_var(data::system_contract::XPROPERTY_LAST_HASH, canvas.get());
                auto bytes = (evm_common::h256()).asBytes();
                bstate->load_string_var(data::system_contract::XPROPERTY_LAST_HASH)->reset({bytes.begin(), bytes.end()}, canvas.get());
            }
            auto unitstate_ptr = std::make_shared<data::xunit_bstate_t>(bstate.get(), false);
            m_mock_bstate[addr.get_account()] = unitstate_ptr;
        }
        return m_mock_bstate.at(addr.get_account());
    }

    bool do_rollback() {
        return false;
    }

    size_t do_snapshot() {
        return 0;
    }

    const std::string & get_table_address() const {
        return table_address;
    }

    bool is_state_dirty() const {
        return true;
    }

    // void add_balance()

    data::xtablestate_ptr_t m_tablestate_ptr;
    std::string table_address;

    std::map<std::string, data::xunitstate_ptr_t> m_mock_bstate;
};
}  // namespace tests
}  // namespace evm
}  // namespace top
