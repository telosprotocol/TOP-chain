#include "xdata/xunit_bstate.h"
#include "xstatectx/xstatectx_face.h"

namespace top {
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

    data::xtablestate_ptr_t m_tablestate_ptr;
    std::string table_address;

    std::map<std::string, data::xunitstate_ptr_t> m_mock_bstate;
};
}  // namespace tests
}  // namespace evm
}  // namespace top
