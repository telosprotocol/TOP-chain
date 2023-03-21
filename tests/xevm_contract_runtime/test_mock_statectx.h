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

    data::xunitstate_ptr_t load_unit_state(common::xaccount_address_t const& address) override {
        return m_unit_state;
    }

    data::xaccountstate_ptr_t           load_account_state(common::xaccount_address_t const& address) override {
        xassert(false);
        return nullptr;
    }

    bool do_rollback() override {
        return false;
    }

    size_t do_snapshot() override {
        return 0;
    }

    std::string get_table_address() const override {
        return common::eth_table_base_address.to_string();
    }

    bool is_state_dirty() const override {
        return true;
    }
    std::map<std::string, statectx::xunitstate_ctx_ptr_t> const& get_modified_unit_ctx() const override {
        return m_changed_ctxs;
    }
    data::xtablestate_ptr_t m_table_state{nullptr};
    data::xunitstate_ptr_t m_unit_state{nullptr};
    std::map<std::string, statectx::xunitstate_ctx_ptr_t> m_changed_ctxs;
};

}  // namespace tests
}  // namespace top