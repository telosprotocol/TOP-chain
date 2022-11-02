// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xnode_id.h"

#include <msgpack.hpp>

#include <string>

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
NS_BEG1(adaptor)

#if !defined (MSGPACK_USE_CPP03)

template <>
struct as<top::common::xnode_id_t>
{
    top::common::xnode_id_t
    operator()(msgpack::object const & o) const {
        if (o.is_nil()) {
            return top::common::xnode_id_t{};
        }

        return top::common::xnode_id_t{ o.as<std::string>() };
    }
};

#endif // !defined (MSGPACK_USE_CPP03)

template <>
struct convert<top::common::xnode_id_t> final
{
    msgpack::object const &
    operator()(msgpack::object const & o, top::common::xnode_id_t & node_id) const {
        if (o.is_nil()) {
            node_id = top::common::xnode_id_t{};
        } else {
            std::string t;
            msgpack::adaptor::convert<std::string>()(o, t);
            node_id = top::common::xnode_id_t{ t };
        }

        return o;
    }
};

template <>
struct pack<top::common::xnode_id_t>
{
    template <typename Stream>
    msgpack::packer<Stream> &
    operator()(msgpack::packer<Stream> & o, top::common::xnode_id_t const & node_id) const {
        if (node_id.empty()) {
            o.pack_nil();
        } else {
            o.pack(node_id.to_string());
        }

        return o;
    }
};

// template <>
// struct object<top::common::xnode_id_t>
// {
//     void
//     operator()(msgpack::object & o, top::common::xnode_id_t const & node_id) const {
//         if (node_id.empty()) {
//             o.type = msgpack::type::NIL;
//         } else {
//             msgpack::adaptor::object<std::string>()(o, node_id.value());  // TODO(bluecl): fatal error: implicit instantiation of undefined template
//         }
//     }
// };

template <>
struct object_with_zone<top::common::xnode_id_t>
{
    void
    operator()(msgpack::object::with_zone & o, top::common::xnode_id_t const & node_id) const {
        if (node_id.empty()) {
            o.type = msgpack::type::NIL;
        } else {
            msgpack::adaptor::object_with_zone<std::string>()(o, node_id.to_string());
        }
    }
};

NS_END1
}
NS_END1
