// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xmessage_id.h"

#include <gtest/gtest.h>


TEST(xcommon, define_msg_id) {
    XDEFINE_MSG_CATEGORY(xmessage_category_test, 0x0001);
    XDEFINE_MSG_ID(xmessage_category_test, xmessage_id_test1, 0x0001);
    XDEFINE_MSG_ID(xmessage_category_test, xmessage_id_test2, 0x0002);
    XDEFINE_MSG_ID(xmessage_category_test, xmessage_id_test3, 0x0003);

    EXPECT_NE(xmessage_id_test1, xmessage_id_test2);
    EXPECT_LE(xmessage_id_test2, xmessage_id_test3);

    XDEFINE_MSG_CATEGORY(xmessage_category_test2, 0x0002);
    XDEFINE_MSG_ID(xmessage_category_test2, xmessage_id_test4, 0x0001);

    EXPECT_NE(xmessage_id_test1, xmessage_id_test4);

}
