#include "xvm/xvm_node.h"
#include "xvm/xvm_trace.h"
#include <gtest/gtest.h>
#include "xbasic/xobject_ptr.h"
#include "xstore/xstore_face.h"

using namespace top;
using namespace top::xvm;
static shared_ptr<xaccount_context_t> g_vote_account_ctx_ptr;

class xvm_vote : public testing::Test
{
    public:
        void SetUp() override
        {
            //make_unit2("T-1", 10);
            //make_unit2("T-2", 10);
        }
        void TearDown() override
        {

        }
    public:
        xtransaction_ptr_t m_publish_trx;
        xtransaction_ptr_t m_exec_trx;

        xnode m_node;
};
        void submit_proposal(xvm_context& ctx);
        void tccVote(xvm_context& ctx);
        void add_limit_voter(xvm_context& ctx);
        void proxy_vote(xvm_context& ctx);
        void submit_option(xvm_context& ctx);

TEST_F(xvm_vote, submit_proposal)
{
    g_vote_account_ctx_ptr = make_shared<xaccount_context_t>("T-2", store::xstore_factory::create_store_with_memdb().get());
    g_vote_account_ctx_ptr->start();

    xaction_t source_action;
    xaction_t destination_action;
    source_action.set_account_addr("T-1");
    destination_action.set_account_addr("T-2");
    destination_action.set_action_name("submit_proposal");
    xtransaction_ptr_t publish_trx = make_object_ptr<xtransaction_t>();
    publish_trx->set_source_action(source_action);
    publish_trx->set_target_action(destination_action);
    xtransaction_trace_ptr trace = m_node.deal_transaction(*publish_trx, *g_vote_account_ctx_ptr);
    EXPECT_EQ(0, trace->m_error_no);
}


TEST_F(xvm_vote, tccVote)
{
    xaction_t source_action;
    xaction_t destination_action;
    //exec trx
    source_action.set_account_addr("T-1");
    destination_action.set_account_addr("T-2");
    destination_action.set_action_name("tccVote");

    xtransaction_ptr_t exec_trx = make_object_ptr<xtransaction_t>();
    exec_trx->set_source_action(source_action);
    exec_trx->set_target_action(destination_action);
    xtransaction_trace_ptr trace = m_node.deal_transaction(*exec_trx, *g_vote_account_ctx_ptr);
    EXPECT_EQ(0, trace->m_error_no);
}

TEST_F(xvm_vote, add_limit_voter)
{
    xaction_t source_action;
    xaction_t destination_action;
    //exec trx
    source_action.set_account_addr("T-1");
    destination_action.set_account_addr("T-2");
    destination_action.set_action_name("add_limit_voter");

    xtransaction_ptr_t exec_trx = make_object_ptr<xtransaction_t>();
    exec_trx->set_source_action(source_action);
    exec_trx->set_target_action(destination_action);
    xtransaction_trace_ptr trace = m_node.deal_transaction(*exec_trx, *g_vote_account_ctx_ptr);
    EXPECT_EQ(0, trace->m_error_no);
}

TEST_F(xvm_vote, proxy_vote)
{
    xaction_t source_action;
    xaction_t destination_action;
    //exec trx
    source_action.set_account_addr("T-1");
    destination_action.set_account_addr("T-2");
    destination_action.set_action_name("proxy_vote");

    xtransaction_ptr_t exec_trx = make_object_ptr<xtransaction_t>();
    exec_trx->set_source_action(source_action);
    exec_trx->set_target_action(destination_action);
    xtransaction_trace_ptr trace = m_node.deal_transaction(*exec_trx, *g_vote_account_ctx_ptr);
    EXPECT_EQ(0, trace->m_error_no);
}

TEST_F(xvm_vote, submit_option)
{
    xaction_t source_action;
    xaction_t destination_action;
    //exec trx
    source_action.set_account_addr("T-1");
    destination_action.set_account_addr("T-2");
    destination_action.set_action_name("submit_option");

    xtransaction_ptr_t exec_trx = make_object_ptr<xtransaction_t>();
    exec_trx->set_source_action(source_action);
    exec_trx->set_target_action(destination_action);
    xtransaction_trace_ptr trace = m_node.deal_transaction(*exec_trx, *g_vote_account_ctx_ptr);
    EXPECT_EQ(0, trace->m_error_no);
}
