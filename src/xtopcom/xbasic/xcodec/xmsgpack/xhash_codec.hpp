// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xhash.hpp"

#include <msgpack.hpp>

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
NS_BEG1(adaptor)

#if !defined (MSGPACK_USE_CPP03)

template <std::size_t Bytes>
struct as<top::xhash_t<Bytes>, typename std::enable_if<msgpack::has_as<std::array<top::xbyte_t, Bytes>>::value>::type>
{
    top::xhash_t<Bytes>
    operator()(msgpack::object const & o) const {
        return top::xhash_t<Bytes>{ o.as<std::array<top::xbyte_t, Bytes>>() };
    }
};

#endif // !defined (MSGPACK_USE_CPP03)

template <std::size_t Bytes>
struct convert<top::xhash_t<Bytes>> final
{
    msgpack::object const &
    operator()(msgpack::object const & o, top::xhash_t<Bytes> & v) const {
        std::array<top::xbyte_t, Bytes> array;
        msgpack::adaptor::convert<std::array<top::xbyte_t, Bytes>>()(o, array);

        v = top::xhash_t<Bytes>{ array };

        return o;
    }
};

template <std::size_t Bytes>
struct pack<top::xhash_t<Bytes>>
{
    template <typename Stream>
    msgpack::packer<Stream> &
    operator()(msgpack::packer<Stream> & o, top::xhash_t<Bytes> const & message) const {
        o.pack(message.as_array());
        return o;
    }
};

template <std::size_t Bytes>
struct object<top::xhash_t<Bytes>>
{
    void
    operator()(msgpack::object & o, top::xhash_t<Bytes> const & v) const {
        msgpack::adaptor::object<std::array<top::xbyte_t, Bytes>>()(o, v.as_array());
    }
};

template <std::size_t Bytes>
struct object_with_zone<top::xhash_t<Bytes>>
{
    void
    operator()(msgpack::object::with_zone & o, top::xhash_t<Bytes> const & v) const {
        msgpack::adaptor::object_with_zone<std::array<top::xbyte_t, Bytes>>()(o, v.as_array());
    }
};

NS_END1
}
NS_END1
