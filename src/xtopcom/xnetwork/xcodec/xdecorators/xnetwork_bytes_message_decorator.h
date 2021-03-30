// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xnetwork/xnetwork_bytes_message.h"

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

struct xtop_network_bytes_message_decorator final
{
    xtop_network_bytes_message_decorator()                                                         = delete;
    xtop_network_bytes_message_decorator(xtop_network_bytes_message_decorator const &)             = delete;
    xtop_network_bytes_message_decorator & operator=(xtop_network_bytes_message_decorator const &) = delete;
    xtop_network_bytes_message_decorator(xtop_network_bytes_message_decorator &&)                  = delete;
    xtop_network_bytes_message_decorator & operator=(xtop_network_bytes_message_decorator &&)      = delete;
    ~xtop_network_bytes_message_decorator()                                                        = delete;

    static
    xbyte_buffer_t
    encode(xbyte_buffer_t const & in);

    static
    xbyte_buffer_t
    decode(xbyte_buffer_t const & in);
};

using xnetwork_bytes_message_decorator_t = xtop_network_bytes_message_decorator;

NS_END4
