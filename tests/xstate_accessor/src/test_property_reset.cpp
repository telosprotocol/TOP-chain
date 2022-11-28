#include "tests/mock/xvchain_creator.hpp"
#include "xstate_accessor/xstate_accessor.h"

#include <gtest/gtest.h>

NS_BEG3(top, state_accessor, tests)

TEST(state_accessor, reset_token) {
    mock::xvchain_creator creator{true};
    common::xaccount_address_t const test_account = common::xaccount_address_t::build_from("T80000f84105cfdb5d35865019e1c963ad4fadc3fd7339");
    auto const account_accessor = state_accessor::xstate_accessor_t::build_from(test_account);
    std::error_code ec;

    evm_common::u256 const balance{123456};

    properties::xproperty_identifier_t const token_property_id{data::XPROPERTY_BALANCE_AVAILABLE, properties::xproperty_type_t::token, properties::xproperty_category_t::system};
    auto const new_balance = account_accessor->balance(token_property_id, balance, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(balance, new_balance);
    auto const queried_balance = account_accessor->balance(token_property_id, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(balance, queried_balance);
}

NS_END3
