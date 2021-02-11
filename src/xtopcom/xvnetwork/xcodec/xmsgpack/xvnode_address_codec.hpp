// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xcodec/xmsgpack/xnode_address_codec.hpp"
#include "xvnetwork/xaddress.h"

//
//#include "xvnetwork/xaddress.h"
//#include "xvnetwork/xcodec/xmsgpack/xaccount_address_codec.hpp"
//#include "xvnetwork/xcodec/xmsgpack/xcluster_address_codec.hpp"
//
//#include <msgpack.hpp>
//
//NS_BEG1(msgpack)
//MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
//NS_BEG1(adaptor)
//
//XINLINE_CONSTEXPR std::size_t xvnode_address_field_count{ 3 };
//XINLINE_CONSTEXPR std::size_t xvnode_address_cluster_address_index{ 0 };
//XINLINE_CONSTEXPR std::size_t xvnode_address_account_address_index{ 1 };
//XINLINE_CONSTEXPR std::size_t xvnode_address_version_index{ 2 };
//
//template <>
//struct convert<top::vnetwork::xvnode_address_t> final
//{
//    msgpack::object const &
//    operator()(msgpack::object const & o, top::vnetwork::xvnode_address_t & v) const {
//        if (o.type != msgpack::type::ARRAY) {
//            throw msgpack::type_error{};
//        }
//
//        if (xvnode_address_field_count <= o.via.array.size) {
//            auto cluster_address = o.via.array.ptr[xvnode_address_cluster_address_index].as<top::vnetwork::xcluster_address_t>();
//            auto account_address = o.via.array.ptr[xvnode_address_account_address_index].as<top::vnetwork::xaccount_address_t>();
//            auto version = o.via.array.ptr[xvnode_address_version_index].as<top::vnetwork::xversion_t>();
//
//            if (!account_address.empty() && !version.empty()) {
//                v = top::vnetwork::xvnode_address_t{
//                    std::move(cluster_address),
//                    std::move(account_address),
//                    std::move(version)
//                };
//            } else if (account_address.empty() && version.empty()) {
//                v = top::vnetwork::xvnode_address_t{
//                    std::move(cluster_address)
//                };
//            } else if (version.empty()) {
//                v = top::vnetwork::xvnode_address_t{
//                    std::move(cluster_address),
//                    std::move(account_address)
//                };
//            } else {
//                v = top::vnetwork::xvnode_address_t{
//                    std::move(cluster_address),
//                    std::move(version)
//                };
//            }
//        } else {
//            throw msgpack::type_error{};
//        }
//
//        return o;
//    }
//};
//
//template <>
//struct pack<top::vnetwork::xvnode_address_t>
//{
//    template <typename Stream>
//    msgpack::packer<Stream> &
//    operator()(msgpack::packer<Stream> & o, top::vnetwork::xvnode_address_t const & message) const {
//        o.pack_array(xvnode_address_field_count);
//        o.pack(message.cluster_address());
//        o.pack(message.account_address());
//        o.pack(message.version());
//
//        return o;
//    }
//};
//
//template <>
//struct object_with_zone<top::vnetwork::xvnode_address_t>
//{
//    void
//    operator()(msgpack::object::with_zone & o, top::vnetwork::xvnode_address_t const & message) const {
//        o.type = type::ARRAY;
//        o.via.array.size = xvnode_address_field_count;
//        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));
//
//        o.via.array.ptr[xvnode_address_cluster_address_index] = msgpack::object{ message.cluster_address(), o.zone };
//        o.via.array.ptr[xvnode_address_account_address_index] = msgpack::object{ message.account_address(), o.zone };
//        o.via.array.ptr[xvnode_address_version_index]         = msgpack::object{ message.version(),         o.zone };
//    }
//};
//
//NS_END1
//}
//NS_END1
