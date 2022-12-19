// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"
#    pragma clang diagnostic ignored "-Wsign-compare"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#    pragma GCC diagnostic ignored "-Wsign-compare"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

#include "xbase/xcontext.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include "xsync/xmessage_pack.h"

NS_BEG2(top, sync)

xmessage_pack_t::xmessage_pack_t() :
xpdu_t<base::xapphead_t>(
    base::xcontext_t::instance(),
(enum_xprotocol_type) (enum_xprotocol_type_chain_min + 1), // TODO : global define protocol type
1) {

}

xmessage_pack_t::xmessage_pack_t(xbyte_buffer_t msg) :
xpdu_t<base::xapphead_t>(
    base::xcontext_t::instance(),
(enum_xprotocol_type) (enum_xprotocol_type_chain_min + 1), // TODO : global define protocol type
1),
message{std::move(msg)}
{
}

vnetwork::xmessage_t& xmessage_pack_t::pack_message(vnetwork::xmessage_t& unpack_msg,
                                                    bool require_compress,
                                                    vnetwork::xmessage_t& packed_msg) {
    xmessage_pack_t mp(unpack_msg.payload());

    base::xpacket_t pack;
    if (require_compress) {
        pack.set_process_flag(base::enum_xpacket_process_flag_compress);
    }
    mp.serialize_to(pack);

    packed_msg = vnetwork::xmessage_t({pack.get_body().data(),
        pack.get_body().data() + pack.get_body().size()},
    unpack_msg.id());

    return packed_msg;
}

void xmessage_pack_t::unpack_message(const xbyte_buffer_t& packed_msg,
        xbyte_buffer_t& msg) {
    base::xstream_t pack(base::xcontext_t::instance(),
                         (uint8_t*) packed_msg.data(),
                         packed_msg.size());

    xmessage_pack_t mp;
    mp.serialize_from(pack);

    msg = std::move(mp.message);
}

int32_t xmessage_pack_t::do_read(base::xmemh_t & archive, const int32_t pdu_body_size) {
    const int32_t begin_size = archive.size();
    archive >> message;
    return (begin_size - archive.size());
}

int32_t xmessage_pack_t::do_write(base::xmemh_t & archive, int32_t & packet_processs_flags) {
    const int32_t begin_size = archive.size();
    archive << message;
    return (archive.size() - begin_size);
}

int32_t xmessage_pack_t::do_read(base::xstream_t & archive, const int32_t pdu_body_size) {
    const int32_t begin_size = archive.size();
    archive >> message;
    return (begin_size - archive.size());
}

int32_t xmessage_pack_t::do_write(base::xstream_t & archive, int32_t & packet_processs_flags) {
    const int32_t begin_size = archive.size();
    archive << message;
    return (archive.size() - begin_size);
}

NS_END2
