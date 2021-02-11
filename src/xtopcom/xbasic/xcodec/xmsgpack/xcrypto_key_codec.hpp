// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xcrypto_key.h"

#include <msgpack.hpp>

// MSGPACK_ADD_ENUM(top::xcrypto_key_type_t)

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
NS_BEG1(adaptor)

template <>
struct convert<top::xcrypto_key_t<top::pub>> final {
    msgpack::object const &
    operator()(msgpack::object const & o, top::xcrypto_key_t<top::pub> & v) const {
        if (o.is_nil()) {
            v = top::xcrypto_key_t<top::pub>{};
        } else {
            std::string t;
            msgpack::adaptor::convert<std::string>{}(o, t);
            v = top::xcrypto_key_t<top::pub>{ t };
        }

        return o;
    }
};

template <>
struct pack<top::xcrypto_key_t<top::pub>> {
    template <typename StreamT>
    msgpack::packer<StreamT> &
    operator()(msgpack::packer<StreamT> & o, top::xcrypto_key_t<top::pub> const & message) const {
        if (message.empty()) {
            o.pack_nil();
        } else {
            o.pack(message.to_string());
        }

        return o;
    }
};

template <>
struct object<top::xcrypto_key_t<top::pub>> {
    void
    operator()(msgpack::object & o, top::xcrypto_key_t<top::pub> const & message) const {
        if (message.empty()) {
            o.type = msgpack::type::NIL;
        } else {
            msgpack::adaptor::object<std::string>{}(o, message.to_string());
        }
    }
};

template <>
struct object_with_zone<top::xcrypto_key_t<top::pub>>
{
    void
    operator()(msgpack::object::with_zone & o, top::xcrypto_key_t<top::pub> const & message) const {
        if (message.empty()) {
            o.type = msgpack::type::NIL;
        } else {
            msgpack::adaptor::object_with_zone<std::string>{}(o, message.to_string());
        }
    }
};

NS_END1
}
NS_END1
