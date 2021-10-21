// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xcodec/xmsgpack_codec.hpp"

#include "xsystem_contracts/xbasic_system_contract.h"

#include <string>

NS_BEG3(top, contract_common, serialization)

extern std::map<common::xaccount_address_t, std::string> const sys_addr_to_metrics_enum_get_property_time;
extern std::map<common::xaccount_address_t, std::string> const sys_addr_to_metrics_enum_set_property_time;
extern std::map<common::xaccount_address_t, std::string> const sys_addr_to_metrics_enum_get_property_size;
extern std::map<common::xaccount_address_t, std::string> const sys_addr_to_metrics_enum_set_property_size;

template <typename T>
struct xtop_msgpack final {
    static xbytes_t serialize_to_bytes(T const & object) {
        return codec::msgpack_encode(object);
    }

    static T deserialize_from_bytes(xbytes_t const & value) {
        return codec::msgpack_decode<T>(value);
    }
};

template <typename T>
using xmsgpack_t = xtop_msgpack<T>;

NS_END3
