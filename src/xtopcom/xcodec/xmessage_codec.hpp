// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"

NS_BEG2(top, codec)

template <typename DecoratorT>
struct xtop_message_codec final
{
    xtop_message_codec()                                       = delete;
    xtop_message_codec(xtop_message_codec const &)             = delete;
    xtop_message_codec & operator=(xtop_message_codec const &) = delete;
    xtop_message_codec(xtop_message_codec &&)                  = delete;
    xtop_message_codec & operator=(xtop_message_codec &&)      = delete;
    ~xtop_message_codec()                                      = delete;

    template <typename ... ArgsT>
    static
    xbyte_buffer_t
    encode(typename DecoratorT::message_type const & data, ArgsT && ... args) {
        return DecoratorT::encode(data, std::forward<ArgsT>(args)...);
    }

    template <typename ... ArgsT>
    static
    auto
    decode(xbyte_buffer_t const & in, ArgsT && ... args) -> decltype(DecoratorT::decode(in, std::forward<ArgsT>(args)...)) {
        return DecoratorT::decode(in, std::forward<ArgsT>(args)...);
    }
};

template <typename DecoratorT>
using xmessage_codec_t = xtop_message_codec<DecoratorT>;

NS_END2
