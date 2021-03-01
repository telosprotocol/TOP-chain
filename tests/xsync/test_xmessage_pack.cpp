#include <gtest/gtest.h>
#include <algorithm>
#include "xvnetwork/xvnetwork_message.h"
#include "xcommon/xsharding_info.h"
#include "xsync/xmessage_pack.h"

using namespace top;
using namespace top::sync;
using namespace top::vnetwork;
using namespace top::common;

bool check_pack_and_unpack(xmessage_t& unpack_msg) {
    xmessage_t packed_msg;
    xmessage_pack_t::pack_message(unpack_msg, 
            unpack_msg.payload().size() >= 2048, packed_msg);
    xbyte_buffer_t msg;
    xmessage_pack_t::unpack_message(packed_msg.payload(), msg);

    if (unpack_msg.id() != packed_msg.id()) return false;

    return msg == unpack_msg.payload();
}

TEST(xmessage_pack, pack_and_unpack_message) {
    // no compress
    xbyte_buffer_t msg;
    for (int i = 0; i < 2000; i++) msg.push_back((uint8_t) i);

    xmessage_t m = xmessage_t(msg, (xmessage_id_t) 10);
    ASSERT_TRUE(check_pack_and_unpack(m));

    // test compress
    msg.clear();
    for (int i = 0; i < 4068; i++) msg.push_back((uint8_t) i);

    xmessage_t m1 = xmessage_t(msg, (xmessage_id_t) 10);
    ASSERT_TRUE(check_pack_and_unpack(m1));

}
