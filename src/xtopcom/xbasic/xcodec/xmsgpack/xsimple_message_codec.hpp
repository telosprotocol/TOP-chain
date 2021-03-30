// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xsimple_message.hpp"

#include <msgpack.hpp>

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
NS_BEG1(adaptor)

template <typename MessageT>
struct convert<top::xsimple_message_t<MessageT>> final
{
    msgpack::object const &
    operator()(msgpack::object const & o,
               top::xsimple_message_t<MessageT> & v) const {
        if (o.type != msgpack::type::ARRAY || o.via.array.size != 2) {
            throw msgpack::type_error{};
        }

        v = top::xsimple_message_t<MessageT>
        {
            o.via.array.ptr[0].as<top::xbyte_buffer_t>(),
            o.via.array.ptr[1].as<MessageT>()
        };

        return o;
    }
};

template <typename MessageT>
struct pack<top::xsimple_message_t<MessageT>>
{
    template <typename Stream>
    msgpack::packer<Stream> &
    operator()(msgpack::packer<Stream> & o,
               top::xsimple_message_t<MessageT> const & message) const {
        o.pack_array(2);
        o.pack(message.payload());
        o.pack(message.type());

        return o;
    }
};

template <typename MessageT>
struct object_with_zone<top::xsimple_message_t<MessageT>>
{
    void
    operator()(msgpack::object::with_zone & o,
               top::xsimple_message_t<MessageT> const & message) const {
        o.type = type::ARRAY;
        o.via.array.size = 2;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));
        o.via.array.ptr[0] = msgpack::object{ message.payload(), o.zone };
        o.via.array.ptr[1] = msgpack::object{ message.type(), o.zone };
    }
};

NS_END1
}
NS_END1
