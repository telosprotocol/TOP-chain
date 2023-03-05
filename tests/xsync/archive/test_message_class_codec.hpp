// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xcodec/xmsgpack/xversion_codec.hpp"
#include <msgpack.hpp>
#include <cstdint>
#include "test_message_class.h"

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS)
{
    NS_BEG1(adaptor)


    template <>
    struct convert<message_class_test> final {
        msgpack::object const& operator()(msgpack::object const& o, message_class_test& message_class_test_obj) const
        {
            if (o.type != msgpack::type::ARRAY) {
                throw msgpack::type_error {};
            }

            if (o.via.array.size == 0) {
                return o;
            }

            message_class_test_obj.set_max_size(o.via.array.ptr[0].as<uint64_t>());      
            message_class_test_obj.set_string_a(o.via.array.ptr[1].as<std::string>());
            message_class_test_obj.set_string_b(o.via.array.ptr[2].as<std::string>());
            return o;
        }
    };

    template <>
    struct pack<message_class_test> {
        template <typename StreamT>
        msgpack::packer<StreamT>& operator()(msgpack::packer<StreamT>& o, message_class_test const& message_class_test_obj) const
        {
            // version + variable numver
            o.pack_array(3);
            o.pack(message_class_test_obj.get_max_size());
            o.pack(message_class_test_obj.get_string_a());
            o.pack(message_class_test_obj.get_string_b());

            return o;
        }
    };

    template <>
    struct object_with_zone<message_class_test> {
        void operator()(msgpack::object::with_zone& o, message_class_test const& message_class_test_obj) const
        {
            o.type = msgpack::type::ARRAY;
            o.via.array.size = 3;
            o.via.array.ptr = static_cast<msgpack::object*>(o.zone.allocate_align(sizeof(::msgpack::object) * o.via.array.size));
            o.via.array.ptr[0] = msgpack::object { message_class_test_obj.get_max_size(), o.zone };
            o.via.array.ptr[1] = msgpack::object { message_class_test_obj.get_string_a(), o.zone };
            o.via.array.ptr[2] = msgpack::object { message_class_test_obj.get_string_b(), o.zone };

        }
    };

    NS_END1
}
NS_END1
