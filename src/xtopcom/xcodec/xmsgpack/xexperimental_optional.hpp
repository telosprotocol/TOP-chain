// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <msgpack.hpp>

#include <experimental/optional>


NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(v1)
{
NS_BEG1(adaptor)

#if !defined (MSGPACK_USE_CPP03)

template <typename T>
struct as<std::experimental::optional<T>, typename std::enable_if<msgpack::has_as<T>::value>::type>
{
    std::experimental::optional<T>
    operator()(msgpack::object const& o) const {
        if (o.is_nil()) {
            return std::experimental::nullopt;
        }
        return o.as<T>();
    }
};

#endif // !defined (MSGPACK_USE_CPP03)

template <typename T>
struct convert<std::experimental::optional<T> >
{
    msgpack::object const &
    operator()(msgpack::object const& o, std::experimental::optional<T> & v) const {
        if (o.is_nil()) {
            v = std::experimental::nullopt;
        } else {
            T t;
            msgpack::adaptor::convert<T>()(o, t);
            v = t;
        }

        return o;
    }
};

template <typename T>
struct pack<std::experimental::optional<T>>
{
    template <typename Stream>
    msgpack::packer<Stream> &
    operator()(msgpack::packer<Stream>& o, std::experimental::optional<T> const & v) const {
        if (v) {
            o.pack(*v);
        } else {
            o.pack_nil();
        }

        return o;
    }
};

template <typename T>
struct object<std::experimental::optional<T>>
{
    void
    operator()(msgpack::object & o, std::experimental::optional<T> const & v) const {
        if (v) {
            msgpack::adaptor::object<T>()(o, *v);
        } else {
            o.type = msgpack::type::NIL;
        }
    }
};

template <typename T>
struct object_with_zone<std::experimental::optional<T>>
{
    void
    operator()(msgpack::object::with_zone& o, std::experimental::optional<T> const & v) const {
        if (v) {
            msgpack::adaptor::object_with_zone<T>()(o, *v);
        } else {
            o.type = msgpack::type::NIL;
        }
    }
};
NS_END1
}
NS_END1

