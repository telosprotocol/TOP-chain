// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xvm/xvote/xproposal.h"
#include "xvm/xvote/xvote_engine.h"

NS_BEG2(top, xvm)
class xvm_context;
class xvote_plugin
{
    public:
        void submit_proposal(xvm_context& ctx);
        void vote_proposal(xvm_context& ctx);
        void add_limit_voter(xvm_context& ctx);
        void proxy_vote(xvm_context& ctx);
        void submit_option(xvm_context& ctx);
    private:
        xvote_engine m_vote_mgr;
};
NS_END2