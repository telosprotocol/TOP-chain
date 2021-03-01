// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdemo.h"

NS_BEG3(top, xvm, xcontract)

hello::hello(common::xnetwork_id_t const & network_id)
    : xbase_t{ network_id }{

}


void hello::hi() {
    printf( "Hello, ");
}
void hello::hi2() {
    printf( "Hello2, ");
}
NS_END3
