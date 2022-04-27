// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xevm_common/common.h"
#include "xevm_common/common_data.h"

#include <msgpack.hpp>

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
    NS_BEG1(adaptor)

    template <>
    struct convert<top::evm_common::u256> final {
        msgpack::object const & operator()(msgpack::object const & o, top::evm_common::u256 & result) const {
            top::xbytes_t bytes;
            msgpack::adaptor::convert<top::xbytes_t>()(o, bytes);
            result = top::from_bytes<top::evm_common::u256>(bytes);

            return o;
        }
    };

    template <>
    struct pack<top::evm_common::u256> {
        template <typename StreamT>
        msgpack::packer<StreamT> & operator()(msgpack::packer<StreamT> & o, top::evm_common::u256 const & result) const {
            top::xbytes_t const bytes = top::to_bytes(result);
            assert(top::from_bytes<top::evm_common::u256>(bytes) == result);

            o.pack(bytes);
            return o;
        }
    };

    NS_END1
}
NS_END1
