// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xnetwork/xcodec/xmessage_flags.h"
#include "xnetwork/xsocket_message.h"

NS_BEG4(top, network, codec, decorators)

/**
 * message structure
 * +----------------+-----------------+--------------+
 * | message header | message payload | message tail |
 * +----------------+-----------------+--------------+
 *
 *
 * message header structure
 * 0----------------------2-----------------------3------------4---------------12----------13----------------15-------------16---------------------------24-----------------------------32
 * +----------------------+-----------------------+------------+----------------+-----------+-----------------+--------------+----------------------------+------------------------------+
 * | message total length | message header length |  version   | identification |   flags   | fragment offset | message type | sender identification hash | receiver identification hash |
 * +----------------------+-----------------------+------------+----------------+-----------+-----------------+--------------+----------------------------+------------------------------+
 * |<------ 2 bytes ----->|<------ 1 byte ------->|<- 1 byte ->|<-- 8 bytes --->|<-1 byte ->|<--- 2 bytes --->|<-- 1 byte -->|<-------- 8 bytes --------->|<--------- 8 bytes ---------->|
 *
 *
 * message tail structure
 * 4 bytes  CRC32
 */

struct xtop_network_bytes_message_header_decorator final
{
    xtop_network_bytes_message_header_decorator()                                                                = delete;
    xtop_network_bytes_message_header_decorator(xtop_network_bytes_message_header_decorator const &)             = delete;
    xtop_network_bytes_message_header_decorator & operator=(xtop_network_bytes_message_header_decorator const &) = delete;
    xtop_network_bytes_message_header_decorator(xtop_network_bytes_message_header_decorator &&)                  = delete;
    xtop_network_bytes_message_header_decorator & operator=(xtop_network_bytes_message_header_decorator &&)      = delete;
    ~xtop_network_bytes_message_header_decorator()                                                               = delete;

    static
    xbyte_buffer_t
    encode(xbyte_buffer_t const & in,
           std::uint8_t const version,
           std::uint64_t const id,
           xmessage_flags_t const flags,
           std::uint16_t const offset,
           xsocket_message_type_t const message_type,
           std::uint64_t const sender_hash,
           std::uint64_t const receiver_hash);

    static
    xbyte_buffer_t
    decode(xbyte_buffer_t const & in,
           std::uint8_t & version,
           std::uint64_t & id,
           xmessage_flags_t & flags,
           std::uint16_t & offset,
           xsocket_message_type_t & message_type,
           std::uint64_t & sender_hash,
           std::uint64_t & receiver_hash);
};

using xnetwork_bytes_message_header_decorator_t = xtop_network_bytes_message_header_decorator;

NS_END4
