#include "gtest/gtest.h"
#include "xvledger/xvblock.h"
#include "xBFT/xconsaccount.h"
#include "xbase/xthread.h"
#include "xcrypto/xckey.h"
#include "xcertauth/xcertauth_face.h"
#include "xBFT/test/common/xunitblock.hpp"

using namespace top;
using namespace top::base;
using namespace top::xconsensus;

class test_xbft_msg : public testing::Test {
 protected:
    void SetUp() override {

     }

    void TearDown() override {
    }
 public:

};


TEST(test_xbft_msg, test_1) {
    // base::xbftpdu_t         _packet;
    // _packet.set_block_chainid(0);
    // _packet.set_block_account("T-a1-3T7BKn5pP8Zi3K5z2Z5BQuSXTf5u37Se79x");
    // _packet.set_block_height(1);
    // _packet.set_block_clock(1);

    // _packet.set_block_viewid(1);
    // _packet.set_block_viewtoken(1111);
    std::vector<int64_t>                m_temps;
    for (int64_t i = 0; i < 10000; i++) {
        m_temps.push_back(i);
    }
    base::xautostream_t<1024> _stream(base::xcontext_t::instance());
    _stream << m_temps;
    std::string bin_data((const char*)_stream.data(), _stream.size());
    // _packet.reset_message(4, 4,bin_data,100,10,10);

    // base::xstream_t stream(base::xcontext_t::instance());
    // int32_t ret = _packet.serialize_to(stream);
    // if (ret <= 0) {
    //     xerror("xnetwork_proxy::send_out serialize fail");
    // }
    // std::cout << "stream_size:" << stream.size() << " bin_size:" << bin_data.size() << " msg_body_size:"  << _packet.get_msg_body().size() << std::endl;

    std::string msg_stream;
    xcommit_msg_t in_msg(enum_xconsensus_code_successful);
    in_msg.set_proof_certificate(std::string(), 1);
    in_msg.set_commit_output(bin_data);
    in_msg.serialize_to_string(msg_stream);
    std::cout << "stream_size:" << msg_stream.size() << " bin_size:" << bin_data.size() << std::endl;

    xcommit_msg_t out_msg;
    xassert(out_msg.serialize_from_string(msg_stream) > 0);
}

TEST(test_xbft_msg, test_2) {
    std::vector<int64_t>                m_temps;
    for (int64_t i = 0; i < 1000; i++) {
        m_temps.push_back(i);
    }
    base::xautostream_t<1024> _stream(base::xcontext_t::instance());
    _stream << m_temps;
    std::string bin_data((const char*)_stream.data(), _stream.size());

    std::string commit_msg_stream;
    xcommit_msg_t in_msg(enum_xconsensus_code_successful);
    in_msg.set_proof_certificate(std::string(), 1);
    in_msg.set_commit_output(bin_data);
    in_msg.serialize_to_string(commit_msg_stream);
    std::cout << "commit_msg_stream_size:" << commit_msg_stream.size() << " bin_size:" << bin_data.size() << std::endl;

    base::xbftpdu_t         send_pdu;
    send_pdu.set_block_chainid(0);
    send_pdu.set_block_account("T-a1-3T7BKn5pP8Zi3K5z2Z5BQuSXTf5u37Se79x");
    send_pdu.set_block_height(1);
    send_pdu.set_block_clock(1);
    send_pdu.set_block_viewid(1);
    send_pdu.set_block_viewtoken(1111);
    send_pdu.reset_message(4, 4,commit_msg_stream,100,10,10);

    base::xstream_t _stream2(base::xcontext_t::instance());
    int32_t ret = send_pdu.serialize_to(_stream2);
    xassert(ret > 0);

    std::string pdu_bin_data((const char*)_stream2.data(), _stream2.size());

    base::xstream_t _stream3(top::base::xcontext_t::instance(), (uint8_t *)(pdu_bin_data.data()), (uint32_t)pdu_bin_data.size());
    base::xcspdu_t * recv_pdu = new base::xcspdu_t(base::xcspdu_t::enum_xpdu_type_consensus_xbft);
    ret = recv_pdu->serialize_from(_stream3);
    xassert(ret > 0);

    xcommit_msg_t out_msg;
    xassert(out_msg.serialize_from_string(recv_pdu->get_msg_body()) > 0);
    xassert(recv_pdu->get_msg_body() == send_pdu.get_msg_body());
}

TEST(test_xbft_msg, test_3) {
    std::vector<int64_t>                m_temps;
    for (int64_t i = 0; i < 10000; i++) {
        m_temps.push_back(i);
    }
    base::xautostream_t<1024> _stream(base::xcontext_t::instance());
    _stream << m_temps;
    std::string bin_data((const char*)_stream.data(), _stream.size());

    std::string commit_msg_stream;
    xcommit_msg_t in_msg(enum_xconsensus_code_successful);
    in_msg.set_proof_certificate(std::string(), 1);
    in_msg.set_commit_output(bin_data);
    in_msg.serialize_to_string(commit_msg_stream);

    base::xbftpdu_t         send_pdu;
    send_pdu.set_block_chainid(0);
    send_pdu.set_block_account("T-a1-3T7BKn5pP8Zi3K5z2Z5BQuSXTf5u37Se79x");
    send_pdu.set_block_height(1);
    send_pdu.set_block_clock(1);
    send_pdu.set_block_viewid(1);
    send_pdu.set_block_viewtoken(1111);
    send_pdu.reset_message(4, 4,commit_msg_stream,100,10,10);

    base::xstream_t _stream2(base::xcontext_t::instance());
    int32_t ret = send_pdu.serialize_to(_stream2);
    xassert(ret > 0);

    std::cout << " commit_msg_stream_size:" << commit_msg_stream.size() << std::endl;
    std::cout << " bin_size:" << bin_data.size() << std::endl;
    std::cout << " _stream2_size:" << _stream2.size() << std::endl;

    std::string pdu_bin_data((const char*)_stream2.data(), _stream2.size());

    base::xstream_t _stream3(top::base::xcontext_t::instance(), (uint8_t *)(pdu_bin_data.data()), (uint32_t)pdu_bin_data.size());
    xassert(_stream2.size() == _stream3.size());
    base::xcspdu_t * recv_pdu = new base::xcspdu_t(base::xcspdu_t::enum_xpdu_type_consensus_xbft);
    ret = recv_pdu->serialize_from(_stream3);
    xassert(ret > 0);
    std::cout << "recv_pdu_size:" << recv_pdu->get_msg_body().size() << " send_pdu_size:" << send_pdu.get_msg_body().size() << std::endl;
    xassert(recv_pdu->get_msg_body() == send_pdu.get_msg_body());

    xcommit_msg_t out_msg;
    xassert(out_msg.serialize_from_string(recv_pdu->get_msg_body()) > 0);
}
