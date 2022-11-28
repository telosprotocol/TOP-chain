
#include "gtest/gtest.h"
#include "xstate_reset/xstate_tablestate_reseter_sample.h"
#include "xstate_reset/xstate_tablestate_reseter_continuous_sample.h"
#include "xstatectx/xstatectx_face.h"
#include "xstatectx/xunitstate_ctx.h"

NS_BEG3(top, state_reset, tests)

class xmock_statectx_t : public statectx::xstatectx_face_t {
    const data::xtablestate_ptr_t & get_table_state() const override {
        return m_tablestate_ptr;
    }
    data::xunitstate_ptr_t load_unit_state(const base::xvaccount_t & addr) override {
        if (m_mock_bstate.find(addr.get_account()) == m_mock_bstate.end()) {
            top::base::xauto_ptr<top::base::xvbstate_t> bstate(new top::base::xvbstate_t(addr.get_account(), 1, 1, "", "", 0, 0, 0));
            auto unitstate_ptr = std::make_shared<data::xunit_bstate_t>(bstate.get(), false);
            m_mock_bstate[addr.get_account()] = unitstate_ptr;
        }
        return m_mock_bstate.at(addr.get_account());
    }
    data::xunitstate_ptr_t load_commit_unit_state(const base::xvaccount_t & addr) {
        return nullptr;
    }
    data::xunitstate_ptr_t load_commit_unit_state(const base::xvaccount_t & addr, uint64_t height) {
        return nullptr;
    }
    bool do_rollback() override {
        return true;
    }
    size_t do_snapshot() override {
        return 0;
    }
    void do_commit(base::xvblock_t * current_block) {
        return;
    }
    std::string get_table_address() const override {
        return "Ta0000@0";
    }
    bool is_state_dirty() const override {
        // return true;
        // if (get_table_state()->is_state_dirty()) {
        //     xdbg("xstatectx_t::is_state_dirty table dirty. %s", get_table_state()->dump().c_str());
        //     return true;
        // }
        for (auto & unitctx : m_mock_bstate) {
            if (unitctx.second->is_state_dirty()) {
                xdbg("xstatectx_t::is_state_dirty unit dirty. %s", unitctx.second->dump().c_str());
                return true;
            }
        }
        return false;
    }

    data::xtablestate_ptr_t m_tablestate_ptr;
    std::map<std::string, data::xunitstate_ptr_t> m_mock_bstate;
};

TEST(test_state_reset, json_parser) {
    // statectx::xstatectx_face_ptr_t mock_state = std::make_shared<xmock_statectx_t>();
    // xstate_tablestate_reseter_sample reseter{mock_state, "TEST_FORK"};
    // reseter.exec_reset_tablestate();
    // EXPECT_FALSE(mock_state->is_state_dirty());
}

TEST(test_state_reset_continuous, json_parser) {
    // statectx::xstatectx_face_ptr_t mock_state = std::make_shared<xmock_statectx_t>();
    // xstate_tablestate_reseter_continuous_sample reseter{mock_state, "TEST_FORK"};
    // reseter.exec_reset_tablestate(0);
    // reseter.exec_reset_tablestate(1);
    // reseter.exec_reset_tablestate(9);
    // EXPECT_FALSE(mock_state->is_state_dirty());
}

NS_END3