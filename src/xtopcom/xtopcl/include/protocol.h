// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#ifndef XCHAIN_SDK_PROTOCOL
#    define XCHAIN_SDK_PROTOCOL

#    include "json/json.h"
#    include "request_result_definition.h"
#    include "task/task_info.h"
#    include "xdata/xtransaction.h"

#    include <functional>
#    include <map>
#    include <memory>
#    include <string>

namespace xChainSDK {

class protocol {
public:
    protocol();
    virtual ~protocol();

    using CreateFunc = std::function<protocol *()>;
    using CreateFuncMap = std::map<std::string, CreateFunc>;
    using ParamMap = std::map<std::string, std::string>;

    virtual int encode_body_params(const ParamMap & params, xJson::Value & root);
    virtual int encode_body(const ParamMap & params, std::string & body);
    virtual int encode(ParamMap & params, std::string & body);
    virtual void decode(const std::string & params, task_info * info);
    static uint32_t decode_task_id(const std::string & result);
    static protocol * create(const std::string & method);
    static void regist_protocols();
    static void regist_create_function(const std::string & method, CreateFunc func);
    static CreateFuncMap _create_funcs;

    virtual int encode_transaction_params(xJson::Value & root, top::data::xtransaction_t * trans_action);
    void set_transaction(top::data::xtransaction_ptr_t & val);

protected:
    bool is_param_key_effect(const std::string & key);

    std::vector<std::string> * _param_names;

    ParamMap effective_params_;
    top::data::xtransaction_ptr_t trans_action_;
};

class CheckTxHashCmd : public protocol {
public:
    CheckTxHashCmd();
    virtual ~CheckTxHashCmd();
    virtual int encode_body_params(const std::map<std::string, std::string> & params, xJson::Value & root) override;
    virtual void decode(const std::string & params, task_info * info) override;
};

class AccountTxCmd : public protocol {
public:
    AccountTxCmd();
    virtual ~AccountTxCmd();
    virtual int encode_body_params(const std::map<std::string, std::string> & params, xJson::Value & root) override;
    virtual void decode(const std::string & params, task_info * info) override;
};

class VoteDetailsCmd : public protocol {
public:
    VoteDetailsCmd();
    virtual ~VoteDetailsCmd();
    virtual void decode(const std::string & params, task_info * info) override;
};

class CandidateCmd : public protocol {
public:
    CandidateCmd();
    virtual ~CandidateCmd();
    virtual void decode(const std::string & params, task_info * info) override;
};

class DividendCmd : public protocol {
public:
    DividendCmd();
    virtual ~DividendCmd();
    virtual void decode(const std::string & params, task_info * info) override;
};

class RequestTokenCmd : public protocol {
public:
    RequestTokenCmd();
    virtual ~RequestTokenCmd();
    virtual void decode(const std::string & params, task_info * info) override;
};

class CreateAccountCmd : public protocol {
public:
    CreateAccountCmd();
    virtual ~CreateAccountCmd();
    virtual void decode(const std::string & params, task_info * info) override;
    std::vector<std::string> _cur_param_names = {"account_addr"};
};

class AccountInfoCmd : public protocol {
public:
    AccountInfoCmd();
    virtual ~AccountInfoCmd();
    virtual void decode(const std::string & params, task_info * info) override;
    std::vector<std::string> _cur_param_names = {"account_addr"};
};

class AccountTransactionCmd : public protocol {
public:
    AccountTransactionCmd();
    virtual ~AccountTransactionCmd();
    virtual void decode(const std::string & params, task_info * info) override;
    std::vector<std::string> _cur_param_names = {"account_addr", "tx_hash"};
};

class TransferCmd : public protocol {
public:
    TransferCmd();
    virtual ~TransferCmd();
    virtual int encode_body_params(const std::map<std::string, std::string> & params, xJson::Value & root) override;
    virtual void decode(const std::string & params, task_info * info) override;
    std::vector<std::string> _cur_param_names = {"from", "to", "amount", "nonce", "last_hash"};
};

class PledgeVoteCmd : public protocol {
public:
    virtual void decode(const std::string & params, task_info * info) override;
};

class RedeemVoteCmd : public protocol {
public:
    virtual void decode(const std::string & params, task_info * info) override;
};

class KeyStoreCmd : public protocol {
public:
    KeyStoreCmd();
    virtual ~KeyStoreCmd();
    virtual int encode_body_params(const std::map<std::string, std::string> & params, xJson::Value & root) override;
    virtual void decode(const std::string & params, task_info * info) override;
    std::vector<std::string> _cur_param_names = {"from", "to", "type", "nonce", "last_hash", "key", "value"};
};

class GetPropertyCmd : public protocol {
public:
    GetPropertyCmd();
    virtual ~GetPropertyCmd();

    virtual int encode_body_params(const std::map<std::string, std::string> & params, xJson::Value & root) override;
    virtual void decode(const std::string & params, task_info * info) override;
    std::vector<std::string> _cur_param_names = {"account_addr", "type", "data1", "data2"};
};

class CallContractCmd : public protocol {
public:
    virtual void decode(const std::string & params, task_info * info) override;
};

class PublishContractCmd : public protocol {
public:
    PublishContractCmd();
    virtual ~PublishContractCmd();
    virtual void decode(const std::string & params, task_info * info) override;
};

class CreateChildAccountCmd : public protocol {
public:
    CreateChildAccountCmd();
    virtual ~CreateChildAccountCmd();
    virtual void decode(const std::string & params, task_info * info) override;
};

class LockTokenCmd : public protocol {
public:
    LockTokenCmd();
    virtual ~LockTokenCmd();
    virtual void decode(const std::string & params, task_info * info) override;
};

class UnlockTokenCmd : public protocol {
public:
    UnlockTokenCmd();
    virtual ~UnlockTokenCmd();
    virtual void decode(const std::string & params, task_info * info) override;
};

class GetBlockCmd : public protocol {
public:
    GetBlockCmd();
    virtual int encode_body_params(const std::map<std::string, std::string> & params, xJson::Value & root) override;
    virtual void decode(const std::string & params, task_info * info) override;
    std::vector<std::string> _cur_param_names = {ParamAccount, ParamGetBlockType, ParamBlockHeight};
};

class NodeRegisterCmd : public protocol {
public:
    virtual void decode(const std::string & params, task_info * info) override;
};

class NodeDeRegisterCmd : public protocol {
public:
    virtual void decode(const std::string & params, task_info * info) override;
};

class NodeRedeemCmd : public protocol {
public:
    virtual void decode(const std::string & params, task_info * info) override;
};

class SetVoteCmd : public protocol {
public:
    virtual void decode(const std::string & params, task_info * info) override;
};

class ClaimNodeRewardCmd : public protocol {
public:
    virtual void decode(const std::string & params, task_info * info) override;
};

class ClaimRewardCmd : public protocol {
public:
    virtual void decode(const std::string & params, task_info * info) override;
};
class AddProposalCmd : public protocol {
public:
    virtual void decode(const std::string & params, task_info * info) override;
};

class WithdrawProposalCmd : public protocol {
public:
    virtual void decode(const std::string & params, task_info * info) override;
};

class VoteProposalCmd : public protocol {
public:
    virtual void decode(const std::string & params, task_info * info) override;
};

class ChainInfoCmd : public protocol {
public:
    ChainInfoCmd();
    // virtual int encode_body_params(const std::map<std::string, std::string>& params, xJson::Value& root) override;
    virtual ~ChainInfoCmd();
    virtual void decode(const std::string & params, task_info * info) override;
    std::vector<std::string> _cur_param_names = {"account_addr"};
};

class NodeInfoCmd : public protocol {
public:
    NodeInfoCmd();
    virtual ~NodeInfoCmd();
    virtual void decode(const std::string & params, task_info * info) override;
    std::vector<std::string> _cur_param_names = {"account_addr", "node_account_addr"};
};

class ElectInfoCmd : public protocol {
public:
    ElectInfoCmd();
    virtual ~ElectInfoCmd();
    virtual void decode(const std::string & params, task_info * info) override;
    std::vector<std::string> _cur_param_names = {"account_addr", "node_account_addr"};
};

class NodeRewardCmd : public protocol {
public:
    NodeRewardCmd();
    virtual ~NodeRewardCmd();
    virtual void decode(const std::string & params, task_info * info) override;
    std::vector<std::string> _cur_param_names = {"account_addr", "node_account_addr"};
};

class VoteDistCmd : public protocol {
public:
    VoteDistCmd();
    virtual ~VoteDistCmd();
    virtual void decode(const std::string & params, task_info * info) override;
    std::vector<std::string> _cur_param_names = {"account_addr", "node_account_addr"};
};

class VoterRewardCmd : public protocol {
public:
    VoterRewardCmd();
    virtual ~VoterRewardCmd();
    virtual void decode(const std::string & params, task_info * info) override;
    std::vector<std::string> _cur_param_names = {"account_addr", "node_account_addr"};
};

class GetProposalCmd : public protocol {
public:
    GetProposalCmd();
    virtual ~GetProposalCmd();
    virtual void decode(const std::string & params, task_info * info) override;
    std::vector<std::string> _cur_param_names = {"account_addr", "proposal_id"};
};

class StandbysInfoCmd : public protocol {
public:
    StandbysInfoCmd();
    virtual ~StandbysInfoCmd();
    virtual void decode(const std::string & params, task_info * info) override;
    std::vector<std::string> _cur_param_names = {"account_addr", "node_account_addr"};
};
}  // namespace xChainSDK

#endif  // !XCHAIN_SDK_PROTOCOL
