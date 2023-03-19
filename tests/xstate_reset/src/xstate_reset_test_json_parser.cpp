
#include "gtest/gtest.h"
#include "xbasic/xhex.h"
#include "xstate_reset/xstate_tablestate_reseter_sample.h"
#include "xstate_reset/xstate_tablestate_reseter_continuous_sample.h"
#include "xstate_reset/xstate_tablestate_reseter_account_state_sample.h"
#include "xstatectx/xstatectx_face.h"
#include "xstatectx/xunitstate_ctx.h"

NS_BEG3(top, state_reset, tests)

class xmock_statectx_t : public statectx::xstatectx_face_t {
    const data::xtablestate_ptr_t & get_table_state() const override {
        return m_tablestate_ptr;
    }
    data::xunitstate_ptr_t load_unit_state(common::xaccount_address_t const& address) override {
        if (m_mock_bstate.find(address.to_string()) == m_mock_bstate.end()) {
            top::base::xauto_ptr<top::base::xvbstate_t> bstate(new top::base::xvbstate_t(address.to_string(), 1, 1, "", "", 0, 0, 0));
            auto unitstate_ptr = std::make_shared<data::xunit_bstate_t>(bstate.get(), bstate.get());
            m_mock_bstate[address.to_string()] = unitstate_ptr;
        }
        return m_mock_bstate.at(address.to_string());
    }
    data::xaccountstate_ptr_t           load_account_state(common::xaccount_address_t const& address) override {
        xassert(false);
        return nullptr;
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

    std::map<std::string, statectx::xunitstate_ctx_ptr_t> const& get_modified_unit_ctx() const override {
        return m_changed_ctxs;
    }

    std::map<std::string, statectx::xunitstate_ctx_ptr_t> m_changed_ctxs;
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

TEST(_, 1) {
    top::base::xauto_ptr<top::base::xvbstate_t> bstate(new top::base::xvbstate_t("T00000LKKvgzCQ2EBXftSdkGiBbMB4i5YLqrd8gF", 1, 1, "", "", 0, 0, 0));
    auto unitstate_ptr = std::make_shared<data::xunit_bstate_t>(bstate.get(), bstate.get());
    unitstate_ptr->set_tep_balance(static_cast<common::xtoken_id_t>(1), evm_common::u256{100});
    unitstate_ptr->set_token_balance("$0", (base::vtoken_t)((uint64_t)1234567));
    auto r = unitstate_ptr->get_bstate()->export_state();
    std::cout << r.size() << ":" << to_hex(r) << std::endl;
    // 00000000000000010100285430303030304c4b4b76677a435132454258667453646b476942624d42346935594c71726438674600000200bfff000010000000022430088eda96019fff0000110000000224313f0101010164

    top::base::xauto_ptr<top::base::xvbstate_t> bstate2(new top::base::xvbstate_t("T00000LKKvgzCQ2EBXftSdkGiBbMB4i5YLqrd8gF", 1, 1, "", "", 0, 0, 0));
    auto unitstate_ptr2 = std::make_shared<data::xunit_bstate_t>(bstate2.get(), bstate2.get());
    unitstate_ptr2->reset_state(r);
    auto r2 = unitstate_ptr2->get_bstate()->export_state();
    std::cout << r2.size() << ":" << to_hex(r2) << std::endl;

    ASSERT_EQ(r, r2);
}

NS_END3