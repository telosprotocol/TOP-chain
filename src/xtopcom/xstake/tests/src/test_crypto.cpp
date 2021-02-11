// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xip.h"
#include "xdata/xelect_transaction.hpp"
#include "xstore/xstore_face.h"
#include <gtest/gtest.h>
#include <memory>
#include <cmath>

#define private public
#include "xcrypto/xckey.h"
#include "xrpc/xuint_format.h"

using namespace top;
using namespace base;
using namespace xrpc;

TEST(test_crypto, all) {
    utl::xecprikey_t pri_key_obj(hex_to_uint("0xe9ffcb2d9d4f5b0cc558941f0eae9ad125d7e7993f1589b7be7af8920cfa3ab6").data());
    utl::xecpubkey_t pub_key_obj = pri_key_obj.get_public_key();
    /*
    std::cout << pub_key_obj.to_address(3, 0) << std::endl;
    std::cout << pub_key_obj.to_address(2, 0) << std::endl;
    std::cout << pub_key_obj.to_address(1, 0) << std::endl;
    std::cout << pub_key_obj.to_address(0, 0) << std::endl;*/

    std::cout << pub_key_obj.to_address("T000001C89ydMtmXVTcDyp6qjRF11jgLxKtYVQfT", 3, 0) << std::endl;
    std::cout << pub_key_obj.to_address("T000001C89ydMtmXVTcDyp6qjRF11jgLxKtYVQfT", 2, 0) << std::endl;
    std::cout << pub_key_obj.to_address("T000001C89ydMtmXVTcDyp6qjRF11jgLxKtYVQfT", 1, 0) << std::endl;
    std::cout << pub_key_obj.to_address("T000001C89ydMtmXVTcDyp6qjRF11jgLxKtYVQfT", 0, 0) << std::endl;
}
