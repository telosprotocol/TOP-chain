// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdemo_param.h"

NS_BEG3(top, xvm, xcontract)

hello_param::hello_param(common::xnetwork_id_t const & network_id)
    : xbase_t{ network_id } {
}

void hello_param::hi() {
    printf("no param");
}
void hello_param::hi2(int32_t i) {
    printf("hi2,%d",i);
}
void hello_param::hi3(int32_t i, std::string& str) {
    printf("hi3,%d,%s",i, str.c_str());
}
void hello_param::hi4(int32_t i, std::string& str, std::map<std::string,std::string> _map) {
    printf("hi4,%d,%s,%zu",i, str.c_str(),_map.size());
}
NS_END3
