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

enum class enum_compress_type : uint8_t {
    no_compress = 0,
    xstream_compress = 1,
};

struct message_compress_header {
    message_compress_header() = default;
    message_compress_header(bool compress_flag) {
        if (compress_flag) {
            compress_type = 1;
        }
    }
    uint8_t protocol_version{1};  
    uint8_t compress_type{0};    //  1:xstream compress

    int32_t serialize_from(base::xstream_t& stream_dst){
        int32_t _size = stream_dst.size();
        stream_dst >> protocol_version;
        stream_dst >> compress_type;

        return (_size - stream_dst.size()); 
    }

    int32_t serialize_to(base::xstream_t& stream_dst){
        int32_t _size = stream_dst.size();
        stream_dst << protocol_version;
        stream_dst << compress_type;
        return (stream_dst.size() - _size); 
    }
};

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
                                          vnetwork::xmessage_t& packed_msg,
                                          bool new_packet_fork) {
    if (new_packet_fork) {
        uint32_t msgid_int = static_cast<std::uint32_t>(unpack_msg.id());
        common::xmessage_id_t msg_id = static_cast<common::xmessage_id_t>(msgid_int + sync_big_pack_increase_index);

        base::xstream_t stream_dst(base::xcontext_t::instance());
        auto size = serialize_to_big_pack(unpack_msg, stream_dst, require_compress);
        if (size <= 0) {
            // error
            xerror("serialize_to_big_pack is error ");
            return unpack_msg;
        }
        packed_msg = vnetwork::xmessage_t({ stream_dst.data(), stream_dst.data() + stream_dst.size() }, msg_id);
    } else {
        xmessage_pack_t mp(unpack_msg.payload());

        base::xpacket_t pack;
        if (require_compress) {
            pack.set_process_flag(base::enum_xpacket_process_flag_compress);
        }
        mp.serialize_to(pack);

        packed_msg = vnetwork::xmessage_t({ pack.get_body().data(),
                                           pack.get_body().data() + pack.get_body().size() },
                                           unpack_msg.id());
    }

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

void xmessage_pack_t::unpack_message(const xbyte_buffer_t& packed_msg, vnetwork::xmessage_t::message_type &msg_type,
    xbyte_buffer_t& msg) {
    if ((static_cast<std::uint32_t>(msg_type) & 0x0000FFFF) < sync_big_pack_increase_index) {
        unpack_message(packed_msg, msg);
    } else {
        base::xstream_t stream_dst(base::xcontext_t::instance());
        int size = serialize_from_big_pack(packed_msg, stream_dst);
        if (size <= 0) {
            xerror("serialize_from_big_pack error.");
            msg_type = static_cast<common::xmessage_id_t>(0);
            return;
        }
        uint32_t msgid_int = static_cast<std::uint32_t>(msg_type);
        msg_type = static_cast<common::xmessage_id_t>(msgid_int - sync_big_pack_increase_index);
        msg = { stream_dst.data(), stream_dst.data() + stream_dst.size() };
    }
}

int32_t xmessage_pack_t::do_read(base::xmemh_t& archive, const int32_t pdu_body_size) {
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

int32_t xmessage_pack_t::serialize_from_big_pack(const xbyte_buffer_t& packed_msg, base::xstream_t& stream_dst) {
    message_compress_header compress_h; 
    base::xstream_t stream_src(base::xcontext_t::instance(), (uint8_t*)packed_msg.data(), packed_msg.size());
    compress_h.serialize_from(stream_src);
    if(compress_h.protocol_version == 1) {
        if(compress_h.compress_type == static_cast<std::uint8_t>(enum_compress_type::xstream_compress)) {
            base::xstream_t::decompress_from_stream(stream_src, stream_src.size(), stream_dst);
        } else if(compress_h.compress_type == static_cast<std::uint8_t>(enum_compress_type::no_compress)){
          stream_src >> stream_dst;
        } else {
            xerror("not suporrt compress_type%d protocol_version%d now.",compress_h.compress_type, compress_h.protocol_version);
        }
    } else {
        xerror("not suporrt compress_type%d protocol_version%d now.",compress_h.compress_type, compress_h.protocol_version);
    }
    xdbg("serialize_from_big_pack xbyte_buffer_t size %ld, unserialize size %d, compress_type %d ", packed_msg.size(), stream_dst.size(), compress_h.compress_type);
    return stream_dst.size();
}

int32_t xmessage_pack_t::serialize_to_big_pack(vnetwork::xmessage_t& unpack_msg, base::xstream_t& stream_dst, bool require_compress) {

    message_compress_header compress_header(require_compress);
    compress_header.serialize_to(stream_dst);
    if(require_compress == true) {
        base::xstream_t stream_src(base::xcontext_t::instance(), (uint8_t*)unpack_msg.payload().data(), unpack_msg.payload().size());
        base::xstream_t::compress_to_stream(stream_src, stream_src.size(), stream_dst);
    } else {
        stream_dst << unpack_msg.payload();
    }
    xdbg("serialize_to_big_pack message size %ld, serialize size %d, require_compress %d ", unpack_msg.payload().size(), stream_dst.size(), require_compress);
    return stream_dst.size();
}
NS_END2
