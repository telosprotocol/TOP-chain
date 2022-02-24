// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xutility/xhash.h"
#include "xvm/xvm_context.h"
#include "xvm/xvm_transaction_context.h"
#include "xvm/xvote/xvote_plugin.h"

NS_BEG2(top, xvm)

#define UPDATE_DB_STRING_STRING(key, value) do { \
        base::xstream_t value##buffer(xcontext_t::instance());\
        value.serialize_write(value##buffer);\
        string value##str((char*)value##buffer.data(), value##buffer.size());\
        if ( ctx.m_account_context.string_set(key, value##str) ) {\
            ctx.m_trx_context.m_trace_ptr->m_errno = vm_vote_update_db_error;\
            break;\
        }\
} while(0)

void xvote_plugin::submit_proposal(xvm_context& ctx) {
    // unpack data
    do {
        // mock proposal_info

        uint256_t arg_digest1 = xsha3_256_t::digest("hash_array.data(), hash_array.size()");
        uint256_t arg_digest2 = xsha3_256_t::digest("hash_array.data(), hash_array.size()");
        std::array<uint8_t, 64> arg_array{0x1,0x2};
        uint256_t arg_digest3 = xsha3_256_t::digest(arg_array.data(), arg_array.size());
        uint256_t arg_digest4 = xsha3_256_t::digest(arg_array.data(), arg_array.size());



        xproposal_info_t proposal_info;
        proposal_info.m_proposal_owern = "T-2";
        proposal_info.m_proposal_content = "vote for me";

        xproposal_option_info_t proposal_option_info;
        proposal_option_info.m_option_content   = "yes";
        proposal_option_info.m_option_owern     = "T-2";
        proposal_option_info.m_is_active        = 1;
        proposal_info.m_option_map.insert(make_pair(proposal_option_info.m_option_content, proposal_option_info));
        proposal_option_info.m_option_content   = "no";
        proposal_option_info.m_option_owern     = "T-2";
        proposal_option_info.m_is_active        = 1;
        proposal_info.m_option_map.insert(make_pair(proposal_option_info.m_option_content, proposal_option_info));

        // end
        string proposal_db_info;
        uint256_t proposal_digest = proposal_info.get_digest();
        string proposal_digest_str((char *)proposal_digest.data(), proposal_digest.size());
        // check the proposal is already exist
        if ( !ctx.m_account_context.string_get(proposal_digest_str, proposal_db_info) ) {
            ctx.m_trx_context.m_trace_ptr->m_errno = vm_vote_proposal_exist_error;
            break;
        }
        if ( !m_vote_mgr.validate_proposal(proposal_info) ) {
            ctx.m_trx_context.m_trace_ptr->m_errno = vm_vote_proposal_ruler_not_valid;
            break;
        }
        //
        base::xstream_t stream_buffer(xcontext_t::instance());
        proposal_info.serialize_write(stream_buffer);
        string proposal_info_buffer((char*)stream_buffer.data(), stream_buffer.size());
        ctx.m_account_context.string_set(proposal_digest_str, proposal_info_buffer);
    } while (0);
}

void xvote_plugin::vote_proposal(xvm_context& ctx) {
    do {
        // unpack data and mock
        xproposal_vote_option_t  proposal_vote_option;
        proposal_vote_option.m_proposal_owern = "T-2";
        proposal_vote_option.m_proposal_content = "vote for me";

        xvote_staked_t vote_staked;
        vote_staked.m_voter_addr = "T-1";
        vote_staked.m_option_content = "yes";
        vote_staked.m_vote_staked = 10;
        proposal_vote_option.m_option_vote_list.emplace_back(vote_staked);

        vote_staked.m_voter_addr = "T-1";
        vote_staked.m_option_content = "no";
        vote_staked.m_vote_staked = 10;
        proposal_vote_option.m_option_vote_list.emplace_back(vote_staked);
        //end

        //first check the proposal is exist
        string db_value_str;
        uint256_t proposal_digest = proposal_vote_option.get_digest();
        string proposal_digest_str((char*)proposal_digest.data(), proposal_digest.size());

        if ( ctx.m_account_context.string_get(proposal_digest_str, db_value_str) ) {
            ctx.m_trx_context.m_trace_ptr->m_errno = vm_vote_proposal_exist_error;
            break;
        }

        // get proposal_info
        base::xstream_t stream_buffer(xcontext_t::instance(), (uint8_t*)db_value_str.data(), db_value_str.size());
        xproposal_info_t proposal_info;
        proposal_info.serialize_read(stream_buffer);

        // set voter_info,first check the voter is already voted
        xproposal_vote_proxy_rpc_t proposal_vote_proxy_rpc;
        proposal_vote_proxy_rpc.m_proposal_owern    = proposal_vote_option.m_proposal_owern;
        proposal_vote_proxy_rpc.m_proposal_content  = proposal_vote_option.m_proposal_content;
        proposal_vote_proxy_rpc.m_voter_addr        = proposal_vote_option.m_option_vote_list.front().m_voter_addr;


        string proposal_vote_digest_str = proposal_vote_proxy_rpc.get_proposal_voter_digest();
        if ( !ctx.m_account_context.string_get(proposal_vote_digest_str, db_value_str) ) {
            ctx.m_trx_context.m_trace_ptr->m_errno = vm_vote_voter_voted_or_proxy;
            break;
        }
        xproposal_vote_info_t proposal_vote_info;
        //proposal_vote_info.m_proposal_hash = proposal_digest;
        proposal_vote_info.m_voter_addr = proposal_vote_option.m_option_vote_list.front().m_voter_addr;
        proposal_vote_info.m_staked = proposal_vote_option.m_option_vote_list.front().m_vote_staked;
        proposal_vote_info.m_voted = static_cast<uint8_t>(enum_proposal_voted_type::enum_proposal_voted);

        // base::xstream_t stream_buffer_w(xcontext_t::instance());
        // proposal_vote_info.serialize_write(stream_buffer_w);
        // string proposal_vote_str(stream_buffer_w.data(), stream_buffer_w.size());
        // if ( ctx.m_account_context.string_set(proposal_vote_digest_str, proposal_vote_str) ) {
        //     ctx.m_trx_context.m_trace_ptr->m_errno = vm_vote_update_db_error;
        //     break;
        // }
        UPDATE_DB_STRING_STRING(proposal_vote_digest_str, proposal_vote_info);

        //update xproposal_info_t
        for(auto& iter : proposal_vote_option.m_option_vote_list) {
            auto iter2 = proposal_info.m_option_map.find(iter.m_option_content);
            if( iter2 != proposal_info.m_option_map.end() && iter2->second.m_is_active == static_cast<uint8_t>(enum_proposal_option_open_type::enum_proposal_option_open) ) {
                iter2->second.m_total_votes += iter.m_vote_staked;
            }
            else
            {
                xinfo_vm("proposal option not find or unopen");
            }
        }

        UPDATE_DB_STRING_STRING(proposal_digest_str, proposal_info);
    } while (0);

}

void xvote_plugin::add_limit_voter(xvm_context& ctx) {
    do {
        // unpack data and mock
        xproposal_vote_limit_t proposal_vote_limit;
        proposal_vote_limit.m_proposal_owern = "T-2";
        proposal_vote_limit.m_proposal_content = "vote for me";
        proposal_vote_limit.m_voter_list.emplace_back(move("T-1"));
        proposal_vote_limit.m_voter_list.emplace_back(move("T-2"));

        //first check the proposal property
        string db_value_str;
        string proposal_digest_str = proposal_vote_limit.get_digest();

        if ( ctx.m_account_context.string_get(proposal_digest_str, db_value_str) ) {
            ctx.m_trx_context.m_trace_ptr->m_errno = vm_vote_proposal_exist_error;
            break;
        }

        // get proposal_info
        base::xstream_t stream_buffer(xcontext_t::instance(), (uint8_t*)db_value_str.data(), db_value_str.size());
        xproposal_info_t proposal_info;
        proposal_info.serialize_read(stream_buffer);
        // if(proposal_info.m_limit_vote != static_cast<uint8_t>(enum_proposal_limit_voter_type::enum_proposal_limit_voter)) {
        //     ctx.m_trx_context.m_trace_ptr->m_errno = vm_vote_proposal_property_error;
        //     break;
        // }
        // todo check the limit voter is exist
        // add limit voter first calculate the hash key [proposal_hash] + ["limit_voter"] + [voter_addr]
        unordered_map<string, string> voter_limit_map = proposal_vote_limit.get_limit_voter_hash();
        for(auto& iter: voter_limit_map) {
            xproposal_limit_voter_info_t proposal_limit_voter_info;
            proposal_limit_voter_info.m_voter_addr = iter.second;
            UPDATE_DB_STRING_STRING(iter.first, proposal_limit_voter_info);
        }
    } while(0);
}

void xvote_plugin::proxy_vote(xvm_context& ctx) {
    do {
        xproposal_vote_proxy_rpc_t proposal_vote_proxy_rpc;
        proposal_vote_proxy_rpc.m_proposal_owern = "T-2";
        proposal_vote_proxy_rpc.m_proposal_content = "vote for me";
        proposal_vote_proxy_rpc.m_voter_addr = "T-1";
        proposal_vote_proxy_rpc.m_proxy_addr = "T-2";
        proposal_vote_proxy_rpc.m_staked = 10;

        //
        //first check the proposal property
        string db_value_str;
        string proposal_digest_str = proposal_vote_proxy_rpc.get_proposal_digest();

        if ( ctx.m_account_context.string_get(proposal_digest_str, db_value_str) ) {
            ctx.m_trx_context.m_trace_ptr->m_errno = vm_vote_proposal_exist_error;
            break;
        }

        // get proposal_info
        base::xstream_t stream_buffer(xcontext_t::instance(), (uint8_t*)db_value_str.data(), db_value_str.size());
        xproposal_info_t proposal_info;
        proposal_info.serialize_read(stream_buffer);

        // check property
        // if(proposal_info.m_is_proxy != static_cast<uint8_t>(enum_proposal_proxy_type::enum_proposal_support_proxy)) {
        //     ctx.m_trx_context.m_trace_ptr->m_errno = vm_vote_proposal_property_error;
        //     break;
        // }
        // check the vote already vote or proxy
        string proposal_voter_digest_str = proposal_vote_proxy_rpc.get_proposal_voter_digest();
        // check the proposal is already exist
        if ( !ctx.m_account_context.string_get(proposal_voter_digest_str, db_value_str) ) {
            ctx.m_trx_context.m_trace_ptr->m_errno = vm_vote_voter_voted_or_proxy;
            break;
        }

        // add vote proxy
        xproposal_vote_info_t proposal_vote_info;
        proposal_vote_info.m_voter_addr = proposal_vote_proxy_rpc.m_voter_addr;
        proposal_vote_info.m_proxy_addr = proposal_vote_proxy_rpc.m_proxy_addr;
        proposal_vote_info.m_staked     = proposal_vote_proxy_rpc.m_staked;
        UPDATE_DB_STRING_STRING(proposal_voter_digest_str, proposal_vote_info);

    } while(0);
}

void xvote_plugin::submit_option(xvm_context& ctx) {
    do {
        xproposal_submit_option_rpc_t proposal_submit_option;
        proposal_submit_option.m_proposal_owern = "T-2";
        proposal_submit_option.m_proposal_content = "vote for me";
        proposal_submit_option.m_submit_addr = "T-1";
        proposal_submit_option.m_option = "abstain";

        //first check the proposal property
        string db_value_str;
        string proposal_digest_str = proposal_submit_option.get_proposal_digest();

        if ( ctx.m_account_context.string_get(proposal_digest_str, db_value_str) ) {
            ctx.m_trx_context.m_trace_ptr->m_errno = vm_vote_proposal_exist_error;
            break;
        }

        // get proposal_info
        base::xstream_t stream_buffer(xcontext_t::instance(), (uint8_t*)db_value_str.data(), db_value_str.size());
        xproposal_info_t proposal_info;
        proposal_info.serialize_read(stream_buffer);
        // check property
        // if(proposal_info.m_proposal_open != static_cast<uint8_t>(enum_proposal_option_open_type::enum_proposal_option_open)) {
        //     ctx.m_trx_context.m_trace_ptr->m_errno = vm_vote_proposal_property_error;
        //     break;
        // }
        if(proposal_info.m_option_map.find(proposal_submit_option.m_option) != proposal_info.m_option_map.end())
        {
            ctx.m_trx_context.m_trace_ptr->m_errno = vm_vote_option_exist_error;
            break;
        }

        xproposal_option_info_t proposal_option_info;
        proposal_option_info.m_option_content = proposal_submit_option.m_option;
        proposal_option_info.m_option_owern = proposal_submit_option.m_submit_addr;
        proposal_option_info.m_is_active = static_cast<uint8_t>(enum_proposal_option_active_type::enum_proposal_option_active);
        proposal_info.m_option_map.insert(make_pair(proposal_submit_option.m_option, proposal_option_info));

        UPDATE_DB_STRING_STRING(proposal_digest_str, proposal_info);

    } while(0);
}


NS_END2
