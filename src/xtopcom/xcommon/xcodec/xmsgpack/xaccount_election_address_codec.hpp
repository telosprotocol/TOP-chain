// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaddress.h"
#include "xcommon/xcodec/xmsgpack/xaccount_address_codec.hpp"
#include "xbasic/xcodec/xmsgpack/xid_codec.hpp"

#include <msgpack.hpp>

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
NS_BEG1(adaptor)

XINLINE_CONSTEXPR std::size_t xaccount_election_address_field_count{ 2 };
XINLINE_CONSTEXPR std::size_t xaccount_election_address_account_address_index{ 0 };
XINLINE_CONSTEXPR std::size_t xaccount_election_address_slot_id_index{ 1 };

template <>
struct convert<top::common::xaccount_election_address_t> final
{
    msgpack::object const &
    operator()(msgpack::object const & o, top::common::xaccount_election_address_t & v) const {
        if (o.type != msgpack::type::ARRAY) {
            throw msgpack::type_error{};
        }

        if (o.via.array.size == 0) {
            return o;
        }

        top::common::xaccount_address_t account_address;
        top::common::xslot_id_t slot_id;
        switch (o.via.array.size - 1) {
            default: {
                XATTRIBUTE_FALLTHROUGH;
            }

            case xaccount_election_address_slot_id_index: {
                slot_id = o.via.array.ptr[xaccount_election_address_slot_id_index].as<top::common::xslot_id_t>();
            }

            case xaccount_election_address_account_address_index: {
                account_address = o.via.array.ptr[xaccount_election_address_account_address_index].as<top::common::xaccount_address_t>();
            }
        }

        v = top::common::xaccount_election_address_t{ std::move(account_address), std::move(slot_id) };

        return o;
    }
};

template <>
struct pack<top::common::xaccount_election_address_t>
{
    template <typename Stream>
    msgpack::packer<Stream> &
    operator()(msgpack::packer<Stream> & o, top::common::xaccount_election_address_t const & message) const {
        o.pack_array(xaccount_election_address_field_count);
        o.pack(message.account_address());
        o.pack(message.slot_id());

        return o;
    }
};

template <>
struct object_with_zone<top::common::xaccount_election_address_t>
{
    void
    operator()(msgpack::object::with_zone & o, top::common::xaccount_election_address_t const & message) const {
        o.type = type::ARRAY;
        o.via.array.size = xaccount_election_address_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));

        o.via.array.ptr[xaccount_election_address_account_address_index] = msgpack::object{ message.account_address(), o.zone };
        o.via.array.ptr[xaccount_election_address_slot_id_index]         = msgpack::object{ message.slot_id(),         o.zone };
    }
};

NS_END1
}
NS_END1
