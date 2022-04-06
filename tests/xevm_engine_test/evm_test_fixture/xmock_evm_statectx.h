#include "xstatectx/xstatectx_face.h"

namespace top {
namespace evm {
namespace tests {

class xmock_evm_statectx : public statectx::xstatectx_face_t {
public:
    xmock_evm_statectx(uint64_t clock, const std::string & random_seed, uint64_t tgas_lock) : m_param(clock, random_seed, tgas_lock) {};

    const data::xtablestate_ptr_t & get_table_state() const {
        return m_tablestate_ptr;
    }

    data::xunitstate_ptr_t load_unit_state(const base::xvaccount_t & addr) {
        return nullptr;
    }

    const statectx::xstatectx_para_t & get_ctx_para() const {
        return m_param;
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
    statectx::xstatectx_para_t m_param;
    std::string table_address;
};
}  // namespace tests
}  // namespace evm
}  // namespace top