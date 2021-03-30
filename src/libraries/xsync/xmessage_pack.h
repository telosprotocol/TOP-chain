// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xpacket.h"
#include "xvnetwork/xmessage.h"

NS_BEG2(top, sync)

using namespace top::base;
using namespace top::vnetwork;

#define DEFAULT_MIN_COMPRESS_THRESHOLD  2048

class xmessage_pack_t : public xpdu_t<xapphead_t> {
public:

    xmessage_pack_t();
    xmessage_pack_t(xbyte_buffer_t msg);
    
    static xmessage_t& pack_message(xmessage_t& unpack_msg,
            bool require_compress, xmessage_t& packed_msg);
    static void unpack_message(const xbyte_buffer_t& packed_msg,
            xbyte_buffer_t& msg);

protected:

    virtual int32_t do_read(xmemh_t & archive, const int32_t pdu_body_size);
    virtual int32_t do_write(xmemh_t & archive, int32_t & packet_processs_flags);
    virtual int32_t do_read(xstream_t & archive,const int32_t pdu_body_size);
    virtual int32_t do_write(xstream_t & archive,int32_t & packet_processs_flags);

public:
    xbyte_buffer_t message;
};

NS_END2