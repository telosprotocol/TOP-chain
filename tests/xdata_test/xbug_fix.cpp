// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xerror/xerror.h"
#include "xdata/xelection/xelection_group_result.h"
#include "xdata/xerror/xerror.h"

#include <gtest/gtest.h>

NS_BEG3(top, tests, data)

TEST(bug_fix, top_4103) {
    top::data::election::xelection_group_result_t egr;

    std::error_code ec;
    auto st = egr.start_time(ec);
    ASSERT_TRUE(ec);
    ASSERT_TRUE(ec.category() == top::data::error::data_category());
    ASSERT_EQ(ec.value(), static_cast<int>(top::data::error::xerrc_t::election_data_start_time_invalid));
    ec.clear();

    ASSERT_EQ(top::common::xjudgement_day, st);

    ASSERT_THROW({ egr.start_time(); }, top::error::xtop_error_t);

    common::xlogic_time_t new_st1{0};
    egr.start_time(new_st1, ec);
    ASSERT_TRUE(!ec);
    ASSERT_NO_THROW(egr.start_time());
    ASSERT_EQ(new_st1, egr.start_time());

    ASSERT_THROW(egr.start_time(new_st1), top::error::xtop_error_t);
    egr.start_time(new_st1, ec);
    ASSERT_TRUE(ec);
    ASSERT_TRUE(ec.category() == top::data::error::data_category());
    ASSERT_EQ(ec.value(), static_cast<int>(top::data::error::xerrc_t::election_data_start_time_invalid));
    ec.clear();

    ASSERT_NO_THROW(egr.start_time());
    auto actual_st = egr.start_time(ec);
    ASSERT_EQ(new_st1, actual_st);
    ASSERT_TRUE(!ec);

    ASSERT_THROW(egr.start_time(top::common::xjudgement_day), top::error::xtop_error_t);
    egr.start_time(top::common::xjudgement_day, ec);
    ASSERT_TRUE(ec);
    ASSERT_TRUE(ec.category() == top::data::error::data_category());
    ASSERT_EQ(ec.value(), static_cast<int>(top::data::error::xerrc_t::election_data_start_time_invalid));
    ec.clear();

    actual_st = egr.start_time(ec);
    ASSERT_EQ(new_st1, actual_st);
    ASSERT_TRUE(!ec);
}

NS_END3
