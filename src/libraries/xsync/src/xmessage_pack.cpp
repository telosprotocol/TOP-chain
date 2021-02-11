// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xcontext.h"
#include "xsync/xmessage_pack.h"

NS_BEG2(top, sync)

xmessage_pack_t::xmessage_pack_t() :
xpdu_t<xapphead_t>(
xcontext_t::instance(),
(enum_xprotocol_type) (enum_xprotocol_type_chain_min + 1), // TODO : global define protocol type
1) {

}

xmessage_pack_t::xmessage_pack_t(xbyte_buffer_t msg) :
xpdu_t<xapphead_t>(
xcontext_t::instance(),
(enum_xprotocol_type) (enum_xprotocol_type_chain_min + 1), // TODO : global define protocol type
1),
message{std::move(msg)}
{
}

xmessage_t& xmessage_pack_t::pack_message(xmessage_t& unpack_msg,
        bool require_compress, xmessage_t& packed_msg) {
    xmessage_pack_t mp(unpack_msg.payload());

    xpacket_t pack;
    if (require_compress) {
        pack.set_process_flag(enum_xpacket_process_flag_compress);
    }
    mp.serialize_to(pack);

    packed_msg = vnetwork::xmessage_t({pack.get_body().data(),
        pack.get_body().data() + pack.get_body().size()},
    unpack_msg.id());

    return packed_msg;
}

void xmessage_pack_t::unpack_message(const xbyte_buffer_t& packed_msg,
        xbyte_buffer_t& msg) {

    xstream_t pack(xcontext_t::instance(),
            (uint8_t*) packed_msg.data(),
            packed_msg.size());

    xmessage_pack_t mp;
    mp.serialize_from(pack);

    msg = std::move(mp.message);
}

int32_t xmessage_pack_t::do_read(xmemh_t & archive, const int32_t pdu_body_size) {
    const int32_t begin_size = archive.size();
    archive >> message;
    return (begin_size - archive.size());
}

int32_t xmessage_pack_t::do_write(xmemh_t & archive, int32_t & packet_processs_flags) {
    const int32_t begin_size = archive.size();
    archive << message;
    return (archive.size() - begin_size);
}

int32_t xmessage_pack_t::do_read(xstream_t & archive, const int32_t pdu_body_size) {
    const int32_t begin_size = archive.size();
    archive >> message;
    return (begin_size - archive.size());
}

int32_t xmessage_pack_t::do_write(xstream_t & archive, int32_t & packet_processs_flags) {
    const int32_t begin_size = archive.size();
    archive << message;
    return (archive.size() - begin_size);
}

NS_END2