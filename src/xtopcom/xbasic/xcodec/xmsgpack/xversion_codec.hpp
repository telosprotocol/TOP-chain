// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xversion.hpp"

#include <msgpack.hpp>

#include <type_traits>

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
NS_BEG1(adaptor)

template <typename TagT, typename ValueT>
struct as<top::xepoch_t<TagT, ValueT>, typename std::enable_if<msgpack::has_as<ValueT>::value>::type>
{
    top::xepoch_t<TagT, ValueT>
    operator()(msgpack::object const & o) const {
        if (o.is_nil()) {
            return top::xepoch_t<TagT, ValueT>{};
        }

        return top::xepoch_t<TagT, ValueT>{ o.as<ValueT>() };
    }
};

template <typename TagT, typename ValueT>
struct convert<top::xepoch_t<TagT, ValueT>> final
{
    msgpack::object const &
    operator()(msgpack::object const & o, top::xepoch_t<TagT, ValueT> & v) const {
        if (o.is_nil()) {
            v = top::xepoch_t<TagT, ValueT>{};
        } else {
            ValueT t;
            msgpack::adaptor::convert<ValueT>{}(o, t);
            v = top::xepoch_t<TagT, ValueT>{ t };
        }

        return o;
    }
};

template <typename TagT, typename ValueT>
struct pack<top::xepoch_t<TagT, ValueT>>
{
    template <typename Stream>
    msgpack::packer<Stream> &
    operator()(msgpack::packer<Stream> & o, top::xepoch_t<TagT, ValueT> const & message) const {
        if (message.has_value()) {
            o.pack(message.value());
        } else {
            o.pack_nil();
        }

        return o;
    }
};

template <typename TagT, typename ValueT>
struct object<top::xepoch_t<TagT, ValueT>>
{
    void
    operator()(msgpack::object & o, top::xepoch_t<TagT, ValueT> const & message) const {
        if (message.has_value()) {
            msgpack::adaptor::object<ValueT>{}(o, message.value());
        } else {
            o.type = msgpack::type::NIL;
        }
    }
};

template <typename TagT, typename ValueT>
struct object_with_zone<top::xepoch_t<TagT, ValueT>>
{
    void
    operator()(msgpack::object::with_zone & o, top::xepoch_t<TagT, ValueT> const & message) const {
        if (message.has_value()) {
            msgpack::adaptor::object_with_zone<ValueT>()(o, message.value());
        } else {
            o.type = msgpack::type::NIL;
        }
    }
};

NS_END1
}
NS_END1
