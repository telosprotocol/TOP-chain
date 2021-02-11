// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcodec/xbuffer_codec.hpp"
#include "xcodec/xdecorators/xprepend_size_decorator.hpp"
#include "xnetwork/xcodec/xdecorators/xnetwork_bytes_message_header_decorator.h"


#include <cassert>
#include <cstring>

#ifdef __LINUX__
#include <endian.h>  // TODO(bluecl): linux only?
#elif defined(__APPLE__)
#include <machine/endian.h>
#endif

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
 *                                                |<----------------------------------------------------- header payload --------------------------------------------------------------->|
 *
 * message tail structure
 * 4 bytes  CRC32
 */

static constexpr std::size_t xsocket_message_header_payload_length{ 29 };
static constexpr std::size_t xsocket_message_header_length_adjustment{ 2 };

xbyte_buffer_t
xtop_network_bytes_message_header_decorator::encode(xbyte_buffer_t const & in,
                                                    std::uint8_t const version,
                                                    std::uint64_t const id,
                                                    xmessage_flags_t const flags,
                                                    std::uint16_t const offset,
                                                    xsocket_message_type_t const message_type,
                                                    std::uint64_t const sender_hash,
                                                    std::uint64_t const receiver_hash) {
    using namespace top::codec;
    using namespace top::codec::decorators;

    XSTATIC_ASSERT(sizeof(xmessage_flags_t) == sizeof(std::uint8_t));
    XSTATIC_ASSERT(sizeof(xsocket_message_type_t) == sizeof(std::uint8_t));

    xbyte_buffer_t header_payload(xsocket_message_header_payload_length);
    auto * data_pos = header_payload.data();

    std::memcpy(data_pos, &version, 1);
    data_pos += 1;

    auto const leid = htole32(id);
    std::memcpy(data_pos, &leid, 4);
    data_pos += 4;

    std::memcpy(data_pos, &flags, 1);
    data_pos += 1;

    auto const leoffset = htole16(offset);
    std::memcpy(data_pos, &leoffset, 2);
    data_pos += 2;

    std::memcpy(data_pos, &message_type, 1);
    data_pos += 1;

    auto const lesender = htole64(sender_hash);
    auto const lereceiver = htole64(receiver_hash);

    std::memcpy(data_pos, &lesender, 8);
    data_pos += 8;

    std::memcpy(data_pos, &lereceiver, 8);
    data_pos += 8;

    assert(std::end(header_payload) == std::next(std::begin(header_payload), data_pos - header_payload.data()));

    auto header =
        xbuffer_codec_t<xprepend_size_decorator_t<std::uint8_t>>::encode(header_payload,
                                                                         xsocket_message_header_length_adjustment);
    header.insert(header.end(), std::begin(in), std::end(in));

    return header;
}

xbyte_buffer_t
xtop_network_bytes_message_header_decorator::decode(xbyte_buffer_t const & in,
                                                    std::uint8_t & version,
                                                    std::uint64_t & id,
                                                    xmessage_flags_t & flags,
                                                    std::uint16_t & offset,
                                                    xsocket_message_type_t & message_type,
                                                    std::uint64_t & sender_hash,
                                                    std::uint64_t & receiver_hash) {
    using namespace top::codec;
    using namespace top::codec::decorators;

    XSTATIC_ASSERT(sizeof(xmessage_flags_t) == sizeof(std::uint8_t));
    XSTATIC_ASSERT(sizeof(xsocket_message_type_t) == sizeof(std::uint8_t));

    std::size_t decoded_size;
    auto header_payload =
        xbuffer_codec_t<xprepend_size_decorator_t<std::uint8_t>>::decode(in,
                                                                         xsocket_message_header_length_adjustment,
                                                                         &decoded_size);

    if (header_payload.size() < xsocket_message_header_payload_length) {
        throw xcodec_error_t{ xcodec_errc_t::decode_wrong_header, __LINE__, __FILE__ };
    }

    auto * header_pos = header_payload.data();

    std::memcpy(&version, header_pos, 1);
    header_pos += 1;

    std::uint64_t leid;
    std::memcpy(&leid, header_pos, 8);
    header_pos += 8;

    id = le64toh(leid);

    std::memcpy(&flags, header_pos, 1);
    header_pos += 1;

    std::uint16_t leoffset;
    std::memcpy(&leoffset, header_pos, 2);
    header_pos += 2;

    offset = le16toh(leoffset);

    std::memcpy(&message_type, header_pos, 1);
    header_pos += 1;

    std::uint64_t lesender;
    std::memcpy(&lesender, header_pos, 8);
    header_pos += 8;

    sender_hash = le64toh(lesender);

    std::uint64_t lereceiver;
    std::memcpy(&lereceiver, header_pos, 8);
    header_pos += 8;

    receiver_hash = le64toh(lereceiver);

    return { std::next(std::begin(in), decoded_size), std::end(in) };
}

NS_END4
