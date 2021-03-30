// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xnetwork/xmessage_transmission_property.h"

#include <msgpack.hpp>

MSGPACK_ADD_ENUM(top::network::xdeliver_protocol_t)
MSGPACK_ADD_ENUM(top::network::xspread_mode_t)
MSGPACK_ADD_ENUM(top::network::xdeliver_priority_t)

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
NS_BEG1(adaptor)

XINLINE_CONSTEXPR std::size_t xdeliver_reliability_field_count{ 2 };
XINLINE_CONSTEXPR std::size_t xdeliver_reliability_protocol_index{ 0 };
XINLINE_CONSTEXPR std::size_t xdeliver_reliability_retry_count_index{ 1 };

template <>
struct convert<top::network::xdeliver_reliability_t> final
{
    ::msgpack::object const &
    operator()(::msgpack::object const & o, top::network::xdeliver_reliability_t & v) const {
        if (o.type != msgpack::type::ARRAY) {
            throw msgpack::type_error{};
        }

        if (xdeliver_reliability_field_count <= o.via.array.size) {
            v = top::network::xdeliver_reliability_t
            {
                o.via.array.ptr[xdeliver_reliability_protocol_index].as<top::network::xdeliver_protocol_t>(),
                o.via.array.ptr[xdeliver_reliability_retry_count_index].as<std::uint8_t>()
            };
        } else {
            throw msgpack::type_error{};
        }

        return o;
    }
};

template <>
struct pack<top::network::xdeliver_reliability_t> final
{
    template <typename Stream>
    ::msgpack::packer<Stream> &
    operator()(::msgpack::packer<Stream> & o, top::network::xdeliver_reliability_t const & message) const {
        o.pack_array(xdeliver_reliability_field_count);

        o.pack(message.protocol);
        o.pack(message.retry_count);

        return o;
    }
};

template <>
struct object_with_zone<top::network::xdeliver_reliability_t> final
{
    void
    operator()(msgpack::object::with_zone & o, top::network::xdeliver_reliability_t const & message) const {
        o.type = type::ARRAY;

        o.via.array.size = xdeliver_reliability_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));
        o.via.array.ptr[xdeliver_reliability_protocol_index] = msgpack::object{ message.protocol,    o.zone };
        o.via.array.ptr[xdeliver_reliability_retry_count_index] = msgpack::object{ message.retry_count, o.zone };
    }
};

XINLINE_CONSTEXPR std::size_t xdeliver_property_field_count{ 2 };

XINLINE_CONSTEXPR std::size_t xdeliver_property_priority_index{ 0 };
XINLINE_CONSTEXPR std::size_t xdeliver_property_reliability_index{ 1 };

template <>
struct convert<top::network::xdeliver_property_t> final
{
    ::msgpack::object const &
    operator()(::msgpack::object const & o, top::network::xdeliver_property_t & v) const {
        if (o.type != msgpack::type::ARRAY) {
            throw msgpack::type_error{};
        }

        if (xdeliver_property_field_count <= o.via.array.size) {
            v = top::network::xdeliver_property_t
            {
                o.via.array.ptr[xdeliver_property_priority_index].as<top::network::xdeliver_priority_t>(),
                o.via.array.ptr[xdeliver_property_reliability_index].as<std::shared_ptr<top::network::xdeliver_reliability_t>>()
            };
        } else {
            throw msgpack::type_error{};
        }

        return o;
    }
};

template <>
struct pack<top::network::xdeliver_property_t> final
{
    template <typename Stream>
    ::msgpack::packer<Stream> &
    operator()(::msgpack::packer<Stream> & o, top::network::xdeliver_property_t const & message) const {
        o.pack_array(xdeliver_property_field_count);

        o.pack(message.priority);
        o.pack(message.reliability);

        return o;
    }
};

template <>
struct object_with_zone<top::network::xdeliver_property_t> final
{
    void
    operator()(msgpack::object::with_zone & o, top::network::xdeliver_property_t const & message) const {
        o.type = type::ARRAY;

        o.via.array.size = xdeliver_property_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));
        o.via.array.ptr[xdeliver_property_priority_index] = msgpack::object{ message.priority, o.zone };
        o.via.array.ptr[xdeliver_property_reliability_index] = msgpack::object{ message.reliability, o.zone };
    }
};

XINLINE_CONSTEXPR std::size_t xgossip_spread_property_field_count{ 2 };

XINLINE_CONSTEXPR std::size_t xgossip_spread_property_spread_mode_index{ 0 };
XINLINE_CONSTEXPR std::size_t xgossip_spread_property_ttl_factor_index{ 1 };

template <>
struct convert<top::network::xspread_property_t> final
{
    ::msgpack::object const &
    operator()(::msgpack::object const & o, top::network::xspread_property_t & v) const {
        if (o.type != msgpack::type::ARRAY) {
            throw msgpack::type_error{};
        }

        if (xgossip_spread_property_field_count <= o.via.array.size) {
            v = top::network::xspread_property_t
            {
                o.via.array.ptr[xgossip_spread_property_spread_mode_index].as<top::network::xspread_mode_t>(),
                o.via.array.ptr[xgossip_spread_property_ttl_factor_index].as<std::uint8_t>()
            };
        } else {
            throw msgpack::type_error{};
        }

        return o;
    }
};

template <>
struct pack<top::network::xspread_property_t> final
{
    template <typename Stream>
    ::msgpack::packer<Stream> &
    operator()(::msgpack::packer<Stream> & o, top::network::xspread_property_t const & message) const {
        o.pack_array(xgossip_spread_property_field_count);

        o.pack(message.spread_mode);
        o.pack(message.ttl_factor);

        return o;
    }
};

template <>
struct object_with_zone<top::network::xspread_property_t> final
{
    void
    operator()(msgpack::object::with_zone & o, top::network::xspread_property_t const & message) const {
        o.type = type::ARRAY;

        o.via.array.size = xgossip_spread_property_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));
        o.via.array.ptr[xgossip_spread_property_spread_mode_index] = msgpack::object{ message.spread_mode, o.zone };
        o.via.array.ptr[xgossip_spread_property_ttl_factor_index] = msgpack::object{ message.ttl_factor, o.zone };
    }
};

XINLINE_CONSTEXPR std::size_t xtransmission_property_field_count{ 2 };

XINLINE_CONSTEXPR std::size_t xtransmission_property_deliver_index{ 0 };
XINLINE_CONSTEXPR std::size_t xtransmission_property_spread_index{ 1 };

template <>
struct convert<top::network::xtransmission_property_t> final
{
    ::msgpack::object const &
    operator()(::msgpack::object const & o, top::network::xtransmission_property_t & v) const {
        if (o.type != msgpack::type::ARRAY) {
            throw msgpack::type_error{};
        }

        if (xtransmission_property_field_count <= o.via.array.size) {
            v = top::network::xtransmission_property_t
            {
                o.via.array.ptr[xtransmission_property_deliver_index].as<top::network::xdeliver_property_t>(),
                o.via.array.ptr[xtransmission_property_spread_index].as<top::network::xspread_property_t>()
            };
        } else {
            throw msgpack::type_error{};
        }

        return o;
    }
};

template <>
struct pack<top::network::xtransmission_property_t> final
{
    template <typename Stream>
    ::msgpack::packer<Stream> &
    operator()(::msgpack::packer<Stream> & o, top::network::xtransmission_property_t const & message) const {
        o.pack_array(xtransmission_property_field_count);

        o.pack(message.deliver_property);
        o.pack(message.spread_property);

        return o;
    }
};

template <>
struct object_with_zone<top::network::xtransmission_property_t> final
{
    void
    operator()(msgpack::object::with_zone & o, top::network::xtransmission_property_t const & message) const {
        o.type = type::ARRAY;

        o.via.array.size = xtransmission_property_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));
        o.via.array.ptr[xtransmission_property_deliver_index] = msgpack::object{ message.deliver_property, o.zone };
        o.via.array.ptr[xtransmission_property_spread_index] = msgpack::object{ message.spread_property, o.zone };
    }
};

NS_END1
}
NS_END1
