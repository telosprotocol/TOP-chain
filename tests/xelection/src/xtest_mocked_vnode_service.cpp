// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xelection/xmocked_vnode_service.h"

#include <gtest/gtest.h>

NS_BEG3(top, tests, election)

TEST(mocked_vnode_service, create) {
    xobject_ptr_t<tests::election::xmocked_vnodesvr_t> mocked = make_object_ptr<tests::election::xmocked_vnodesvr_t>(common::xaccount_address_t{ "T0000xyz" }, std::string{"xxxx"});
}

NS_END3
