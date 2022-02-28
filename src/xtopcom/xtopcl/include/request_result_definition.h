// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#ifndef XCHAIN_REQUEST_RESULT_DIFINITION_H
#    define XCHAIN_REQUEST_RESULT_DIFINITION_H

#    include "topchain_type.h"
#    include "xbase/xmem.h"

#    include <memory>
#    include <string>

namespace xChainSDK {
const char ERROR_NO[]{"errno"};
const char ERROR_NO2[]{"error"};
const char ERROR_MSG[]{"errmsg"};
const char SEQUENCE_ID[]{"sequence_id"};
const char DATA[]{"data"};

enum class ErrorCode : int {
    Success = 0,
    JsonParseError,
    SignError = 4,
};
const char CMD_TOKEN[]{"requestToken"};
const char CMD_CREATE_ACCOUNT[]{"create_account"};
const char CMD_ACCOUNT_INFO[]{"getAccount"};
const char CMD_STANDBYS_INFO[]{"getStandbys"};
const char CMD_TRANSFER[]{"transfer"};
const char CMD_PLEDGETGAS[]{"pledgetgas"};
const char CMD_REDEEMTGAS[]{"redeemtgas"};
const char CMD_PLEDGEDISK[]{"pledgedisk"};
const char CMD_REDEEMDISK[]{"redeemdisk"};
const char CMD_ACCOUNT_TRANSACTION[]{"getTransaction"};
const char CMD_KEY_STORE[]{"keystore"};
const char CMD_GET_PROPERTY[]{"get_property"};
const char CMD_PUBLISH_CONTRACT[]{"publish_contract"};
const char CMD_CALL_CONTRACT[]{"call_contract"};
const char CMD_CREATE_CHILD_ACCOUNT[]{"create_child_account"};
const char CMD_LOCK_TOKEN[]{"lock_token"};
const char CMD_UNLOCK_TOKEN[]{"unlock_token"};
const char CMD_GET_BLOCK[]{"getBlock"};
const char CMD_GET_BLOCKS_BY_HEIGHT[]{"getBlocksByHeight"};
const char CMD_NODE_REGISTER[]{"registerNode"};
const char CMD_NODE_DEREGISTER[]{"unregisterNode"};
const char CMD_REDEEM[]{"redeemNodeDeposit"};
const char CMD_PLEDGE_VOTE[]{"pledge_vote"};
const char CMD_REDEEM_VOTE[]{"redeem_vote"};
const char CMD_SET_VOTE[]{"voteNode"};
const char CMD_CLAIM_NODE_REWARD[]{"claimNodeReward"};
const char CMD_CLAIM_REWARD[]{"claimVoterDividend"};
const char CMD_ISSURANCE[]{"issurance"};
const char CMD_ADD_PROPOSAL[]{"submitProposal"};
const char CMD_WITHDRAW_PROPOSAL[]{"withdrawProposal"};
const char CMD_VOTE_PROPOSAL[]{"tccVote"};
const char CMD_ADD_PROPOSAL2[]{"propose"};
const char CMD_CANCEL_PROPOSAL[]{"cancel"};
const char CMD_APPROVE_PROPOSAL[]{"approve"};
const char CMD_CHAIN_INFO[]{"getChainInfo"};
const char CMD_NODE_INFO[]{"queryNodeInfo"};
const char CMD_ELECT_INFO[]{"getElectInfo"};
const char CMD_NODE_REWARD[]{"queryNodeReward"};
const char CMD_VOTE_DIST[]{"listVoteUsed"};
const char CMD_VOTER_REWARD[]{"queryVoterDividend"};
const char CMD_GET_PROPOSAL[]{"queryProposal"};
const char CMD_GET_ONCHAINPARAMS[]{"getCGP"};

using VoteDetailsResultPtr = std::shared_ptr<VoteDetailsResult>;
using CandidateDetailsResultPtr = std::shared_ptr<CandidateDetailsResult>;
using DividendDetailsResultPtr = std::shared_ptr<DividendDetailsResult>;

using RequestTokenResultPtr = std::shared_ptr<RequestTokenResult>;
using CreateAccountResultPtr = std::shared_ptr<CreateAccountResult>;
using AccountInfoResultPtr = std::shared_ptr<AccountInfoResult>;
using AccountTransactionResultPtr = std::shared_ptr<AccountTransactionResult>;
using TransferResultPtr = std::shared_ptr<TransferResult>;
using GetPropertyResultPtr = std::shared_ptr<GetPropertyResult>;

struct xproperty_vote {
    int32_t serialize_write(top::base::xstream_t & stream) {
        const int32_t begin_pos = stream.size();
        stream << m_lock_hash;
        stream << m_amount;
        stream << m_available;
        stream << m_expiration;
        const int32_t end_pos = stream.size();
        return (end_pos - begin_pos);
    }

    int32_t serialize_read(top::base::xstream_t & stream) {
        const int32_t begin_pos = stream.size();
        stream >> m_lock_hash;
        stream >> m_amount;
        stream >> m_available;
        stream >> m_expiration;
        const int32_t end_pos = stream.size();
        return (begin_pos - end_pos);
    }

    std::string m_lock_hash;
    uint64_t m_amount;
    uint64_t m_available;
    uint64_t m_expiration;
};

struct xproperty_vote_out {
    int32_t serialize_write(top::base::xstream_t & stream) {
        const int32_t begin_pos = stream.size();
        stream << m_address;
        stream << m_lock_hash;
        stream << m_amount;
        stream << m_expiration;
        const int32_t end_pos = stream.size();
        return (end_pos - begin_pos);
    }

    int32_t serialize_read(top::base::xstream_t & stream) {
        const int32_t begin_pos = stream.size();
        stream >> m_address;
        stream >> m_lock_hash;
        stream >> m_amount;
        stream >> m_expiration;
        const int32_t end_pos = stream.size();
        return (begin_pos - end_pos);
    }

    std::string m_address;
    std::string m_lock_hash;
    uint64_t m_amount;
    uint64_t m_expiration;
};

struct xvote_info {
    int32_t serialize_write(top::base::xstream_t & stream) {
        const int32_t begin_pos = stream.size();
        stream << m_addr_from;
        stream << m_addr_to;
        stream << m_lock_hash;
        stream << m_amount;
        stream << m_expiration;
        const int32_t end_pos = stream.size();
        return (end_pos - begin_pos);
    }

    int32_t serialize_read(top::base::xstream_t & stream) {
        const int32_t begin_pos = stream.size();
        stream >> m_addr_from;
        stream >> m_addr_to;
        stream >> m_lock_hash;
        stream >> m_amount;
        stream >> m_expiration;
        const int32_t end_pos = stream.size();
        return (begin_pos - end_pos);
    }

    std::string m_addr_from{""};
    std::string m_addr_to{""};
    std::string m_lock_hash{""};
    uint64_t m_amount{0};
    uint64_t m_expiration{0};
};
}  // namespace xChainSDK

#endif  // !XCHAIN_REQUEST_RESULT_DIFINITION_H
