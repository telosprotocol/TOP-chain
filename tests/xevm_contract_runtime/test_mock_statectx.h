#include "xdata/xnative_contract_address.h"
#include "xstatectx/xstatectx_face.h"

namespace top {
namespace tests {

class xmock_statectx_t : public statectx::xstatectx_face_t {
public:
    xmock_statectx_t(data::xunitstate_ptr_t state) : m_unit_state(state) {
    }

    const data::xtablestate_ptr_t & get_table_state() const override {
        return m_table_state;
    }

    data::xunitstate_ptr_t load_unit_state(const base::xvaccount_t & addr) override {
        return m_unit_state;
    }

    bool do_rollback() override {
        return false;
    }

    size_t do_snapshot() override {
        return 0;
    }

    std::string get_table_address() const override {
        return eth_table_address.value();
    }

    bool is_state_dirty() const override {
        return true;
    }

    data::xtablestate_ptr_t m_table_state{nullptr};
    data::xunitstate_ptr_t m_unit_state{nullptr};
};

}  // namespace tests
}  // namespace top