// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xpacket.h"
#include "xvnetwork/xmessage.h"

NS_BEG2(top, sync)

// using namespace top::base;
// using namespace top::vnetwork;

#define DEFAULT_MIN_COMPRESS_THRESHOLD  2048

class xmessage_pack_t : public base::xpdu_t<base::xapphead_t> {
public:

    xmessage_pack_t();
    xmessage_pack_t(xbyte_buffer_t msg);
    
    static vnetwork::xmessage_t& pack_message(vnetwork::xmessage_t& unpack_msg,
                                              bool require_compress,
                                              vnetwork::xmessage_t& packed_msg,
                                              bool new_packet_fork = true);
    static void unpack_message(const xbyte_buffer_t& packed_msg,
                               xbyte_buffer_t& msg);
    static void unpack_message(const xbyte_buffer_t& packed_msg, 
                               vnetwork::xmessage_t::message_type &msg_type,
                               xbyte_buffer_t& msg);
   
    static int32_t serialize_from_big_pack(const xbyte_buffer_t& packed_msg, 
                                          base::xstream_t& stream_dst);

    static int32_t serialize_to_big_pack(vnetwork::xmessage_t& unpack_msg, base::xstream_t& stream_dst, bool require_compress = false);
protected:

    virtual int32_t do_read(base::xmemh_t & archive, const int32_t pdu_body_size);
    virtual int32_t do_write(base::xmemh_t & archive, int32_t & packet_processs_flags);
    virtual int32_t do_read(base::xstream_t & archive,const int32_t pdu_body_size);
    virtual int32_t do_write(base::xstream_t & archive,int32_t & packet_processs_flags);

public:
    xbyte_buffer_t message;
};

NS_END2