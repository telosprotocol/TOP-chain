#if 0
#include "gtest/gtest.h"
#include "xunit_service/xnetwork_proxy.h"
#include "xunit_service/xcons_unorder_cache.h"

namespace top {
using namespace xunit_service;

class xunorder_test : public testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}

public:
};

TEST_F(xunorder_test, unoder_filter) {
    base::xauto_ptr<base::xcspdu_t> pdu(new base::xcspdu_t(base::xcspdu_t::enum_xpdu_type_consensus_xbft));
    uint64_t account_viewid = 10;
    xvip2_t from_addr;
    xvip2_t to_addr;
    std::string msg_content;
    uint16_t msg_nonce = 10;
    int8_t ttl = 12;
    uint8_t msg_type;

    xcons_unorder_cache unorder_cache;

    {
        msg_type = xconsensus::enum_consensus_msg_type_proposal;
        pdu->set_block_viewid(account_viewid);
        pdu->reset_message(msg_type, ttl, msg_content, msg_nonce, from_addr.low_addr, to_addr.low_addr);
        ASSERT_TRUE(unorder_cache.filter_event(account_viewid, from_addr, to_addr, *pdu.get()));
    }
    {
        msg_type = xconsensus::enum_consensus_msg_type_proposal;
        pdu->set_block_viewid(account_viewid - 1);
        pdu->reset_message(msg_type, ttl, msg_content, msg_nonce, from_addr.low_addr, to_addr.low_addr);
        ASSERT_FALSE(unorder_cache.filter_event(account_viewid, from_addr, to_addr, *pdu.get()));
        ASSERT_EQ(unorder_cache.get_unoder_cache_size(), 0);
    }
    {
        msg_type = xconsensus::enum_consensus_msg_type_proposal;
        pdu->set_block_viewid(account_viewid + 1);
        pdu->reset_message(msg_type, ttl, msg_content, msg_nonce, from_addr.low_addr, to_addr.low_addr);
        ASSERT_FALSE(unorder_cache.filter_event(account_viewid, from_addr, to_addr, *pdu.get()));
        ASSERT_EQ(unorder_cache.get_unoder_cache_size(), 1);
        unorder_cache.on_view_fire(account_viewid + 2);
        ASSERT_EQ(unorder_cache.get_unoder_cache_size(), 0);
    }
    {
        msg_type = xconsensus::enum_consensus_msg_type_proposal;
        pdu->set_block_viewid(account_viewid + 1);
        pdu->reset_message(msg_type, ttl, msg_content, msg_nonce, from_addr.low_addr, to_addr.low_addr);
        ASSERT_FALSE(unorder_cache.filter_event(account_viewid, from_addr, to_addr, *pdu.get()));
        ASSERT_EQ(unorder_cache.get_unoder_cache_size(), 1);
        xconsensus::xcspdu_fire* proposal_event = unorder_cache.get_proposal_event(account_viewid + 1);
        ASSERT_NE(proposal_event, nullptr);
        ASSERT_EQ(unorder_cache.get_unoder_cache_size(), 0);
    }
    {
        msg_type = xconsensus::enum_consensus_msg_type_proposal;
        pdu->set_block_viewid(account_viewid + 2);
        pdu->reset_message(msg_type, ttl, msg_content, msg_nonce, from_addr.low_addr, to_addr.low_addr);
        ASSERT_FALSE(unorder_cache.filter_event(account_viewid, from_addr, to_addr, *pdu.get()));
        ASSERT_EQ(unorder_cache.get_unoder_cache_size(), 0);
    }

    {
        msg_type = xconsensus::enum_consensus_msg_type_commit;
        pdu->set_block_viewid(account_viewid + 10);
        pdu->reset_message(msg_type, ttl, msg_content, msg_nonce, from_addr.low_addr, to_addr.low_addr);
        ASSERT_TRUE(unorder_cache.filter_event(account_viewid, from_addr, to_addr, *pdu.get()));
        ASSERT_EQ(unorder_cache.get_unoder_cache_size(), 0);
    }
}
}  // namespace top
#endif
