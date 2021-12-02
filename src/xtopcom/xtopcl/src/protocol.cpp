// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "protocol.h"

#include "base/utility.h"
#include "global_definition.h"
#include "task/task_dispatcher.h"
#include "user_info.h"
#include "xrpc/xuint_format.h"

#include <assert.h>

#include <set>
#include <vector>

extern xChainSDK::user_info g_userinfo;

namespace xChainSDK {
using namespace top::xrpc;
using std::cout;
using std::endl;
using std::string;

constexpr char account[] = "account";
constexpr char tx_hash[] = "tx_hash";
constexpr char from_account_addr[] = "from_account_addr";
constexpr char to_account_addr[] = "to_account_addr";
constexpr char details[] = "details";
constexpr char amount[] = "amount";
constexpr char turns[] = "turns";
constexpr char from[] = "from";
constexpr char to[] = "to";
constexpr char tickets[] = "tickets";
constexpr char name[] = "name";
constexpr char self_rate[] = "self_rate";
constexpr char total_share[] = "total_share";

constexpr char target_account_addr[] = "target_account_addr";
constexpr char identity_token[] = "identity_token";
constexpr char signature[] = "signature";
constexpr char sign_method[] = "signature_method";
constexpr char sign_version[] = "signature_ver_code";
constexpr char secret_key[] = "secret_key";
constexpr char keyid[] = "keyid";

// these params come out of body
const std::set<std::string> content_name = {
    "target_account_addr",
    "key_id",
    "nounce",
    "time_stamp",
    "signature",
    "identity_token",
    "expire",
    "method",
    "action",  // ???
    "version",
    "seed",
    "hash",
    "sequence_id",
    "fragment_id",
    "cookie",
    "body",
};

protocol::CreateFuncMap protocol::_create_funcs;

protocol::protocol() : _param_names(nullptr) {
}

protocol::~protocol() {
}

int protocol::encode_body_params(const ParamMap & params, xJson::Value & root) {
    if (nullptr == _param_names)
        return 0;

    auto it = _param_names->begin();
    for (; it != _param_names->end(); ++it) {
        const std::string & name = *it;
        auto it_v = params.find(*it);
        if (it_v != params.end()) {
            root["params"][name] = it_v->second;
        } else {
#ifdef TRANS_EMPTY_PARAMS
            root["params"][name] = "";
#endif  // TRANS_EMPTY_PARAMS
        }
    }
    return 0;
}

int protocol::encode_body(const ParamMap & params, std::string & body) {
    xJson::Value root;

    if (trans_action_ != nullptr) {
        encode_transaction_params(root, trans_action_.get());
    } else {
        encode_body_params(params, root);
    }
    xJson::FastWriter fast_writer;
    body += fast_writer.write(root);
    return body.length();
}

int protocol::encode(ParamMap & params, std::string & body) {
    // encode body params, different for every specific interface
    std::string body_params{""};
    encode_body(params, body_params);
    params.insert(ParamMap::value_type("body", body_params));

    // public parameters shared by all rpc interfaces
    for (auto it : params) {
        if (is_param_key_effect(it.first)) {
            body += (body.length() == 0 ? "" : "&");
            body += utility::urlencode(it.first);
            body += "=";
            body += utility::urlencode(it.second);  // it.second; //
        }
    }

    return body.length();
}

void protocol::decode(const std::string & params, task_info * info) {
    xJson::Reader reader;
    xJson::Value root;

    auto result = std::make_shared<ResultBase>();

    try {
        if (reader.parse(params, root)) {
            if (root[SEQUENCE_ID].isString())
                result->sequence_id = atoi(root[SEQUENCE_ID].asString().c_str());
            if (root[ERROR_NO].isInt())
                result->error = root[ERROR_NO].asInt();
            if (root[ERROR_MSG].isString())
                result->errmsg = root[ERROR_MSG].asString();
        }
    } catch (...) {
        result->error = static_cast<int>(ErrorCode::JsonParseError);
        result->errmsg = "Json Parse Error.";
    }

    auto info_callback = dynamic_cast<task_info_callback<ResultBase> *>(info);
    assert(info_callback != nullptr);

    if (info_callback->callback_ != nullptr) {
        info_callback->callback_(result.get());
    }
}

uint32_t protocol::decode_task_id(const std::string & result) {
    xJson::Reader reader;
    xJson::Value root;

    uint32_t task_id = 0;
    try {
        if (reader.parse(result, root)) {
            if (root[SEQUENCE_ID].isString()) {
                std::string str = root[SEQUENCE_ID].asString();
                if (!str.empty())
                    task_id = atoi(str.c_str());
            }
        }
    } catch (...) {
    }
    return task_id;
}

protocol * protocol::create(const std::string & method) {
    if (protocol::_create_funcs.size() == 0) {
#ifdef DEBUG
        std::cout << "[debug]register protocols" << std::endl;
#endif
        regist_protocols();
    }
    auto it = _create_funcs.find(method);
    if (it != _create_funcs.end()) {
        return it->second();
    } else {
        it = _create_funcs.find("__default");
        if (it != _create_funcs.end()) {
            return it->second();
        }
    }
    return nullptr;
}

void protocol::regist_protocols() {
    regist_create_function("__default", []() { return new protocol(); });

    regist_create_function(CMD_TOKEN, []() { return new RequestTokenCmd(); });

    regist_create_function(CMD_CREATE_ACCOUNT, []() { return new CreateAccountCmd(); });

    regist_create_function(CMD_ACCOUNT_INFO, []() { return new AccountInfoCmd(); });

    regist_create_function(CMD_ACCOUNT_TRANSACTION, []() { return new AccountTransactionCmd(); });

    regist_create_function(CMD_TRANSFER, []() { return new TransferCmd(); });
    regist_create_function(CMD_PLEDGETGAS, []() { return new TransferCmd(); });
    regist_create_function(CMD_REDEEMTGAS, []() { return new TransferCmd(); });
    regist_create_function(CMD_PLEDGEDISK, []() { return new TransferCmd(); });
    regist_create_function(CMD_REDEEMDISK, []() { return new TransferCmd(); });
    regist_create_function(CMD_PLEDGE_VOTE, []() { return new PledgeVoteCmd(); });
    regist_create_function(CMD_REDEEM_VOTE, []() { return new RedeemVoteCmd(); });

    regist_create_function(CMD_KEY_STORE, []() { return new KeyStoreCmd(); });

    regist_create_function(CMD_GET_PROPERTY, []() { return new GetPropertyCmd(); });

    regist_create_function(CMD_PUBLISH_CONTRACT, []() { return new PublishContractCmd(); });

    regist_create_function(CMD_CALL_CONTRACT, []() { return new CallContractCmd(); });

    regist_create_function(CMD_CREATE_CHILD_ACCOUNT, []() { return new CreateChildAccountCmd(); });

    regist_create_function(CMD_LOCK_TOKEN, []() { return new LockTokenCmd(); });

    regist_create_function(CMD_UNLOCK_TOKEN, []() { return new UnlockTokenCmd(); });

    regist_create_function(CMD_GET_BLOCK, []() { return new GetBlockCmd(); });

    regist_create_function(CMD_NODE_REGISTER, []() { return new NodeRegisterCmd(); });

    regist_create_function(CMD_NODE_DEREGISTER, []() { return new NodeDeRegisterCmd(); });

    regist_create_function(CMD_REDEEM, []() { return new NodeRedeemCmd(); });

    regist_create_function(CMD_SET_VOTE, []() { return new SetVoteCmd(); });

    regist_create_function(CMD_CLAIM_NODE_REWARD, []() { return new ClaimNodeRewardCmd(); });

    regist_create_function(CMD_CLAIM_REWARD, []() { return new ClaimRewardCmd(); });
    regist_create_function(CMD_ADD_PROPOSAL, []() { return new AddProposalCmd(); });
    regist_create_function(CMD_WITHDRAW_PROPOSAL, []() { return new WithdrawProposalCmd(); });
    regist_create_function(CMD_VOTE_PROPOSAL, []() { return new VoteProposalCmd(); });
    regist_create_function(CMD_ADD_PROPOSAL2, []() { return new AddProposalCmd(); });
    regist_create_function(CMD_CANCEL_PROPOSAL, []() { return new WithdrawProposalCmd(); });
    regist_create_function(CMD_APPROVE_PROPOSAL, []() { return new VoteProposalCmd(); });

    regist_create_function(CMD_CHAIN_INFO, []() { return new ChainInfoCmd(); });
    regist_create_function(CMD_NODE_INFO, []() { return new NodeInfoCmd(); });
    regist_create_function(CMD_ELECT_INFO, []() { return new ElectInfoCmd(); });
    regist_create_function(CMD_STANDBYS_INFO, []() { return new StandbysInfoCmd(); });
    regist_create_function(CMD_NODE_REWARD, []() { return new NodeRewardCmd(); });
    regist_create_function(CMD_VOTE_DIST, []() { return new VoteDistCmd(); });
    regist_create_function(CMD_VOTER_REWARD, []() { return new VoterRewardCmd(); });
    regist_create_function(CMD_GET_PROPOSAL, []() { return new GetProposalCmd(); });
    regist_create_function(CMD_GET_ONCHAINPARAMS, []() { return new GetProposalCmd(); });
}

void protocol::regist_create_function(const std::string & method, CreateFunc func) {
    _create_funcs.insert(CreateFuncMap::value_type(method, func));
}

int protocol::encode_transaction_params(xJson::Value & root, top::data::xtransaction_t * tx_ptr) {
    if (tx_ptr == nullptr)
        return -1;

    xJson::Value & result_json = root["params"];
    if (tx_ptr->get_tx_version() == top::data::xtransaction_version_2) {
        tx_ptr->parse_to_json(result_json);
    } else {
        tx_ptr->parse_to_json(result_json, top::data::RPC_VERSION_V1);
    }
    return 0;
}

void protocol::set_transaction(top::data::xtransaction_ptr_t & val) {
    trans_action_ = val;
}

bool protocol::is_param_key_effect(const std::string & key) {
    return content_name.find(key) != content_name.end();
}

CheckTxHashCmd::CheckTxHashCmd() {
}

CheckTxHashCmd::~CheckTxHashCmd() {
    std::cout << "Destroy CheckTxHashCmd" << std::endl;
}

int CheckTxHashCmd::encode_body_params(const std::map<std::string, std::string> & params, xJson::Value & root) {
    if (nullptr == _param_names)
        return 0;

    auto it = _param_names->begin();
    for (; it != _param_names->end(); ++it) {
        const std::string & name = *it;
        auto it_v = params.find(*it);
        if (it_v != params.end()) {
            root["params"][name] = it_v->second;
        } else {
#ifdef TRANS_EMPTY_PARAMS
            root["params"][name] = "";
#endif  // TRANS_EMPTY_PARAMS
        }
    }
    return 0;
}

void CheckTxHashCmd::decode(const std::string & params, task_info * info) {
}

AccountTxCmd::AccountTxCmd() {
}

AccountTxCmd::~AccountTxCmd() {
    // std::cout << "Destroy CheckTxHashCmd" << std::endl;
}

int AccountTxCmd::encode_body_params(const std::map<std::string, std::string> & params, xJson::Value & root) {
    if (nullptr == _param_names)
        return 0;

    auto it = _param_names->begin();
    for (; it != _param_names->end(); ++it) {
        const std::string & name = *it;
        auto it_v = params.find(*it);
        if (it_v != params.end()) {
            if ("amount" == name) {
                root["params"][name] = atoi(it_v->second.c_str());
            } else {
                root["params"][name] = it_v->second;
            }
        } else {
#ifdef TRANS_EMPTY_PARAMS
            root["params"][name] = "";
#endif  // TRANS_EMPTY_PARAMS
        }
    }
    return 0;
}

void AccountTxCmd::decode(const std::string & params, task_info * info) {
}

VoteDetailsCmd::VoteDetailsCmd() {
}

VoteDetailsCmd::~VoteDetailsCmd() {
    // std::cout << "Destroy VoteDetailsCmd" << std::endl;
}

void VoteDetailsCmd::decode(const std::string & params, task_info * info) {
    xJson::Reader reader;
    xJson::Value root;

    auto result = std::make_shared<VoteDetailsResult>();

    try {
        if (reader.parse(params, root)) {
            if (root[SEQUENCE_ID].isString())
                result->sequence_id = atoi(root[SEQUENCE_ID].asString().c_str());
            if (root[ERROR_NO].isInt())
                result->error = root[ERROR_NO].asInt();
            if (root[ERROR_MSG].isString())
                result->errmsg = root[ERROR_MSG].asString();

            xJson::Value date = root[DATA];
            if (!date.isNull()) {
                if (date[details].isArray()) {
                    auto & a = date[details];
                    uint32_t size = a.size();
                    for (uint32_t i = 0; i < size; ++i) {
                        result->_votes.push_back({a[i][turns].asUInt(), a[i][from].asString(), a[i][to].asString(), a[i][tickets].asUInt()});
                    }
                }
            }
        }
    } catch (...) {
        result->error = static_cast<int>(ErrorCode::JsonParseError);
        result->errmsg = "Json Parse Error.";
    }
    auto info_callback = dynamic_cast<task_info_callback<VoteDetailsResult> *>(info);
    if (info_callback->callback_ != nullptr) {
        info_callback->callback_(result.get());
    } else {
        task_dispatcher::get_instance()->handle_vote_details_result(result);
    }
}

CandidateCmd::CandidateCmd() {
}

CandidateCmd::~CandidateCmd() {
    // std::cout << "Destroy CandidateCmd" << std::endl;
}

void CandidateCmd::decode(const std::string & params, task_info * info) {
    xJson::Reader reader;
    xJson::Value root;

    auto result = std::make_shared<CandidateDetailsResult>();

    try {
        if (reader.parse(params, root)) {
            if (root[SEQUENCE_ID].isString())
                result->sequence_id = atoi(root[SEQUENCE_ID].asString().c_str());
            if (root[ERROR_NO].isInt())
                result->error = root[ERROR_NO].asInt();
            if (root[ERROR_MSG].isString())
                result->errmsg = root[ERROR_MSG].asString();

            xJson::Value date = root[DATA];
            if (!date.isNull()) {
                if (date[details].isArray()) {
                    auto & a = date[details];
                    uint32_t size = a.size();
                    for (uint32_t i = 0; i < size; ++i) {
                        result->_candidates.push_back({a[i][turns].asUInt(), a[i][name].asString(), a[i][tickets].asUInt(), a[i][self_rate].asFloat()});
                    }
                }
            }
        }
    } catch (...) {
        result->error = static_cast<int>(ErrorCode::JsonParseError);
        result->errmsg = "Json Parse Error.";
    }
    auto info_callback = dynamic_cast<task_info_callback<CandidateDetailsResult> *>(info);
    if (info_callback->callback_ != nullptr) {
        info_callback->callback_(result.get());
    } else {
        task_dispatcher::get_instance()->handle_candidate_details_result(result);
    }
}

DividendCmd::DividendCmd() {
}

DividendCmd::~DividendCmd() {
    // std::cout << "Destroy DividendCmd" << std::endl;
}

void DividendCmd::decode(const std::string & params, task_info * info) {
    xJson::Reader reader;
    xJson::Value root;

    auto result = std::make_shared<DividendDetailsResult>();

    try {
        if (reader.parse(params, root)) {
            if (root[SEQUENCE_ID].isString())
                result->sequence_id = atoi(root[SEQUENCE_ID].asString().c_str());
            if (root[ERROR_NO].isInt())
                result->error = root[ERROR_NO].asInt();
            if (root[ERROR_MSG].isString())
                result->errmsg = root[ERROR_MSG].asString();

            xJson::Value date = root[DATA];
            if (!date.isNull()) {
                if (date[details].isArray()) {
                    auto & a = date[details];
                    uint32_t size = a.size();
                    for (uint32_t i = 0; i < size; ++i) {
                        result->_dividends.push_back({a[i][name].asString(), a[i][total_share].asUInt()});
                    }
                }
            }
        }
    } catch (...) {
        result->error = static_cast<int>(ErrorCode::JsonParseError);
        result->errmsg = "Json Parse Error.";
    }
    auto info_callback = dynamic_cast<task_info_callback<DividendDetailsResult> *>(info);
    if (info_callback->callback_ != nullptr) {
        info_callback->callback_(result.get());
    } else {
        task_dispatcher::get_instance()->handle_dividend_details_result(result);
    }
}

RequestTokenCmd::RequestTokenCmd() {
}

RequestTokenCmd::~RequestTokenCmd() {
    // std::cout << "Destroy RequestTokenCmd" << std::endl;
}

void RequestTokenCmd::decode(const std::string & params, task_info * info) {
    xJson::Reader reader;
    xJson::Value root;

    auto result = std::make_shared<RequestTokenResult>();

    try {
        if (reader.parse(params, root)) {
            if (root[SEQUENCE_ID].isString())
                result->sequence_id = atoi(root[SEQUENCE_ID].asString().c_str());
            if (root[ERROR_NO].isInt())
                result->error = root[ERROR_NO].asInt();
            if (root[ERROR_MSG].isString())
                result->errmsg = root[ERROR_MSG].asString();

            xJson::Value date = root[DATA];
            if (!date.isNull()) {
                if (date[identity_token].isString())
                    result->_token = date[identity_token].asString();
                if (date[secret_key].isString())
                    result->_secret_key = date[secret_key].asString();
                if (date[sign_method].isString())
                    result->_sign_method = date[sign_method].asString();
                if (date[sign_version].isString())
                    result->_sign_version = date[sign_version].asString();
            }
        }
    } catch (...) {
        result->error = static_cast<int>(ErrorCode::JsonParseError);
        result->errmsg = "Json Parse Error.";
    }
    auto info_callback = dynamic_cast<task_info_callback<RequestTokenResult> *>(info);
    if (info_callback->callback_ != nullptr) {
        info_callback->callback_(result.get());
    } else {
        task_dispatcher::get_instance()->handle_request_token_result(result);
    }
}

CreateAccountCmd::CreateAccountCmd() {
    _param_names = &_cur_param_names;
}

CreateAccountCmd::~CreateAccountCmd() {
    // std::cout << "Destroy CreateAccountCmd" << std::endl;
}

void CreateAccountCmd::decode(const std::string & params, task_info * info) {
    xJson::Reader reader;
    xJson::Value root;

    auto result = std::make_shared<CreateAccountResult>();

    try {
        if (reader.parse(params, root)) {
            if (root[SEQUENCE_ID].isString())
                result->sequence_id = atoi(root[SEQUENCE_ID].asString().c_str());
            if (root[ERROR_NO].isInt())
                result->error = root[ERROR_NO].asInt();
            if (root[ERROR_MSG].isString())
                result->errmsg = root[ERROR_MSG].asString();

            xJson::Value date = root["data"];
            if (!date.isNull()) {
            }
        }
    } catch (...) {
        result->error = static_cast<int>(ErrorCode::JsonParseError);
        result->errmsg = "Json Parse Error.";
    }
    auto info_callback = dynamic_cast<task_info_callback<ResultBase> *>(info);
    if (info_callback->callback_ != nullptr) {
        info_callback->callback_(result.get());
    } else {
        task_dispatcher::get_instance()->handle_create_account_result(result);
    }
}

AccountInfoCmd::AccountInfoCmd() {
    _param_names = &_cur_param_names;
}

AccountInfoCmd::~AccountInfoCmd() {
    // std::cout << "Destroy AccountInfoCmd" << std::endl;
}

void AccountInfoCmd::decode(const std::string & params, task_info * info) {
    xJson::Reader reader;
    xJson::Value root;

    auto result = std::make_shared<AccountInfoResult>();
    try {
        if (reader.parse(params, root)) {
            if (root[SEQUENCE_ID].isString())
                result->sequence_id = atoi(root[SEQUENCE_ID].asString().c_str());
            if (root[ERROR_NO].isInt())
                result->error = root[ERROR_NO].asInt();
            if (root[ERROR_MSG].isString())
                result->errmsg = root[ERROR_MSG].asString();

            xJson::Value data = root["data"];
            if (!data.isNull()) {
                if (data["account_addr"].isString())
                    result->_account = data["account_addr"].asString();
                if (data["latest_tx_hash"].isString())
                    result->_last_hash = data["latest_tx_hash"].asString();
                if (data["balance"].isUInt64())
                    result->_balance = data["balance"].asUInt64();
                if (data["nonce"].isUInt())
                    result->_nonce = data["nonce"].asUInt();
                if (data["latest_tx_hash_xxhash64"].isString()) {
                    auto xxhash_str = data["latest_tx_hash_xxhash64"].asString();
                    result->_last_hash_xxhash64 = top::data::hex_to_uint64(xxhash_str);
                }
            }
        }
    } catch (...) {
        result->error = static_cast<int>(ErrorCode::JsonParseError);
        result->errmsg = "Json Parse Error.";
    }

    auto info_callback = dynamic_cast<task_info_callback<AccountInfoResult> *>(info);
    if (info_callback->callback_ != nullptr) {
        info_callback->callback_(result.get());
    } else {
        task_dispatcher::get_instance()->handle_account_info_result(result);
    }
}

AccountTransactionCmd::AccountTransactionCmd() {
    _param_names = &_cur_param_names;
}

AccountTransactionCmd::~AccountTransactionCmd() {
    // std::cout << "Destroy AccountTransactionCmd" << std::endl;
}
void AccountTransactionCmd::decode(const std::string & params, task_info * info) {
    xJson::Reader reader;
    xJson::Value root;

    auto result = std::make_shared<AccountTransactionResult>();
    try {
        if (reader.parse(params, root)) {
            if (root[SEQUENCE_ID].isString())
                result->sequence_id = atoi(root[SEQUENCE_ID].asString().c_str());
            if (root[ERROR_NO].isInt())
                result->error = root[ERROR_NO].asInt();
            if (root[ERROR_MSG].isString())
                result->errmsg = root[ERROR_MSG].asString();

            xJson::Value date = root["data"];
            if (!date.isNull()) {
                result->_authorization = date["authorization"].asString();
                result->_expire_duration = date["expire_duration"].asUInt();
                result->_fire_timestamp = date["fire_timestamp"].asUInt();
                result->_from_network_id = date["from_ledger_id"].asUInt();
                result->_last_trans_hash = date["last_trans_hash"].asUInt();
                result->_last_trans_nonce = date["last_trans_nonce"].asUInt();
                result->_to_network_id = date["to_ledger_id"].asUInt();
                result->_trans_random_nounce = date["tx_random_nonce"].asUInt();
                result->_premium_price = date["premium_price"].asUInt();
                result->_transaction_hash = date["transaction_hash"].asString();
                result->_ext = date["ext"].asString();
                result->_transaction_len = date["transaction_len"].asUInt();
                result->_transaction_type = date["transaction_type"].asUInt();

                if (!date["sender_action"].isNull()) {
                    result->_source_action._account_addr = date["sender_action"]["account_addr"].asString();
                    result->_source_action._action_authorization = date["sender_action"]["action_authorization"].asString();
                    result->_source_action._action_hash = date["sender_action"]["action_hash"].asUInt();
                    result->_source_action._action_name = date["sender_action"]["action_name"].asString();
                    result->_source_action._action_param = date["sender_action"]["action_param"].asString();
                    result->_source_action._action_ext = date["sender_action"]["action_ext"].asString();
                    result->_source_action._action_size = date["sender_action"]["action_size"].asUInt();
                    result->_source_action._action_type = date["sender_action"]["action_type"].asUInt();
                }

                if (!date["receiver_action"].isNull()) {
                    result->_target_action._account_addr = date["receiver_action"]["account_addr"].asString();
                    result->_target_action._action_authorization = date["receiver_action"]["action_authorization"].asString();
                    result->_target_action._action_hash = date["receiver_action"]["action_hash"].asUInt();
                    result->_target_action._action_name = date["receiver_action"]["action_name"].asString();
                    result->_target_action._action_param = date["receiver_action"]["action_param"].asString();
                    result->_target_action._action_ext = date["receiver_action"]["action_ext"].asString();
                    result->_target_action._action_size = date["receiver_action"]["action_size"].asUInt();
                    result->_target_action._action_type = date["receiver_action"]["action_type"].asUInt();
                }
            }
        }
    } catch (...) {
        result->error = static_cast<int>(ErrorCode::JsonParseError);
        result->errmsg = "Json Parse Error.";
    }

    auto info_callback = dynamic_cast<task_info_callback<AccountTransactionResult> *>(info);
    if (info_callback->callback_ != nullptr) {
        info_callback->callback_(result.get());
    } else {
        task_dispatcher::get_instance()->handle_account_transaction_result(result);
    }
}

TransferCmd::TransferCmd() {
    _param_names = &_cur_param_names;
}

TransferCmd::~TransferCmd() {
    //        std::cout << "Destroy TransferCmd" << std::endl;
}

int TransferCmd::encode_body_params(const std::map<std::string, std::string> & params, xJson::Value & root) {
    if (nullptr == _param_names)
        return 0;

    auto it = _param_names->begin();
    for (; it != _param_names->end(); ++it) {
        const std::string & name = *it;
        auto it_v = params.find(*it);
        if (it_v != params.end()) {
            if ("amount" == name || "nonce" == name) {
                root["params"][name] = atoi(it_v->second.c_str());
            } else {
                root["params"][name] = it_v->second;
            }
        } else {
#ifdef TRANS_EMPTY_PARAMS
            root["params"][name] = "";
#endif  // TRANS_EMPTY_PARAMS
        }
    }
    return 0;
}

void TransferCmd::decode(const std::string & params, task_info * info) {
    xJson::Reader reader;
    xJson::Value root;

    auto result = std::make_shared<TransferResult>();

    try {
        if (reader.parse(params, root)) {
            if (root[SEQUENCE_ID].isString())
                result->sequence_id = atoi(root[SEQUENCE_ID].asString().c_str());
            if (root[ERROR_NO].isInt())
                result->error = root[ERROR_NO].asInt();
            if (root[ERROR_MSG].isString())
                result->errmsg = root[ERROR_MSG].asString();

            xJson::Value date = root["data"];
            if (!date.isNull()) {
            }
        }
    } catch (...) {
        result->error = static_cast<int>(ErrorCode::JsonParseError);
        result->errmsg = "Json Parse Error.";
    }

    auto info_callback = dynamic_cast<task_info_callback<TransferResult> *>(info);
    if (info_callback->callback_ != nullptr) {
        info_callback->callback_(result.get());
    } else {
        task_dispatcher::get_instance()->handle_transfer_result(result);
    }
}

void PledgeVoteCmd::decode(const std::string & params, task_info * info) {
}

void RedeemVoteCmd::decode(const std::string & params, task_info * info) {
}

KeyStoreCmd::KeyStoreCmd() {
    _param_names = &_cur_param_names;
}

KeyStoreCmd::~KeyStoreCmd() {
    // std::cout << "Destroy KeyStoreCmd" << std::endl;
}

int KeyStoreCmd::encode_body_params(const std::map<std::string, std::string> & params, xJson::Value & root) {
    if (nullptr == _param_names)
        return 0;

    auto it = _param_names->begin();
    for (; it != _param_names->end(); ++it) {
        const std::string & name = *it;
        auto it_v = params.find(*it);
        if (it_v != params.end()) {
            if ("amount" == name || "nonce" == name || "type" == name) {
                root["params"][name] = atoi(it_v->second.c_str());
            } else {
                root["params"][name] = it_v->second;
            }
        } else {
#ifdef TRANS_EMPTY_PARAMS
            root["params"][name] = "";
#endif  // TRANS_EMPTY_PARAMS
        }
    }
    return 0;
}

void KeyStoreCmd::decode(const std::string & params, task_info * info) {
}

GetPropertyCmd::GetPropertyCmd() {
    _param_names = &_cur_param_names;
}

GetPropertyCmd::~GetPropertyCmd() {
    // std::cout << "Destroy GetPropertyCmd" << std::endl;
}

int GetPropertyCmd::encode_body_params(const std::map<std::string, std::string> & params, xJson::Value & root) {
    if (nullptr == _param_names)
        return 0;

    std::string data1, data2, type;
    auto it = _param_names->begin();
    for (; it != _param_names->end(); ++it) {
        const std::string & name = *it;
        auto it_v = params.find(*it);
        if (it_v != params.end()) {
            if ("data1" == name) {
                data1 = it_v->second;
            } else if ("data2" == name) {
                data2 = it_v->second;
            } else {
                if ("type" == name) {
                    type = it_v->second;
                }
                root["params"][name] = it_v->second;
            }
        } else {
#ifdef TRANS_EMPTY_PARAMS
            root["params"][name] = "";
#endif  // TRANS_EMPTY_PARAMS
        }
    }

    if ("map" == type) {
        root["params"]["data"].append(data1);
        root["params"]["data"].append(data2);
    } else {
        root["params"]["data"] = data1;
    }

    return 0;
}
void GetPropertyCmd::decode(const std::string & params, task_info * info) {
    xJson::Reader reader;
    xJson::Value root;

    auto result = std::make_shared<GetPropertyResult>();

    assert(info != nullptr);
    auto it_type = info->params.find("type");
    if (it_type != info->params.end()) {
        result->_type = it_type->second;
    }

    auto it_data1 = info->params.find("data1");
    if (it_data1 != info->params.end()) {
        result->_data1 = it_data1->second;
    }

    auto it_data2 = info->params.find("data2");
    if (it_data2 != info->params.end()) {
        result->_data2 = it_data2->second;
    }

    try {
        if (reader.parse(params, root)) {
            if (root[SEQUENCE_ID].isString())
                result->sequence_id = atoi(root[SEQUENCE_ID].asString().c_str());
            if (root[ERROR_NO].isInt())
                result->error = root[ERROR_NO].asInt();
            if (root[ERROR_MSG].isString())
                result->errmsg = root[ERROR_MSG].asString();

            xJson::Value date = root["data"];
            if (!date.isNull()) {
                if (date["property_value"].isArray()) {
                    auto size = date["property_value"].size();
                    for (xJson::ArrayIndex i = 0; i < size; ++i) {
                        std::cout << "property_value:[" << i << "]" << date["property_value"][i].asString() << std::endl;  ///!!!
                        result->_values.push_back(date["property_value"][i].asString());
                    }
                }
            }
        }
    } catch (...) {
        result->error = static_cast<int>(ErrorCode::JsonParseError);
        result->errmsg = "Json Parse Error.";
    }

    auto info_callback = dynamic_cast<task_info_callback<GetPropertyResult> *>(info);
    if (info_callback != nullptr && info_callback->callback_ != nullptr) {
        info_callback->callback_(result.get());
    } else {
        task_dispatcher::get_instance()->handle_get_property_result(result);
    }
}

void CallContractCmd::decode(const std::string & params, task_info * info) {
}

PublishContractCmd::PublishContractCmd() {
}

PublishContractCmd::~PublishContractCmd() {
}

void PublishContractCmd::decode(const std::string & params, task_info * info) {
}

CreateChildAccountCmd::CreateChildAccountCmd() {
}

CreateChildAccountCmd::~CreateChildAccountCmd() {
}

void CreateChildAccountCmd::decode(const std::string & params, task_info * info) {
}

LockTokenCmd::LockTokenCmd() {
}

LockTokenCmd::~LockTokenCmd() {
}

void LockTokenCmd::decode(const std::string & params, task_info * info) {
}

UnlockTokenCmd::UnlockTokenCmd() {
}

UnlockTokenCmd::~UnlockTokenCmd() {
}

void UnlockTokenCmd::decode(const std::string & params, task_info * info) {
}

GetBlockCmd::GetBlockCmd() {
    _param_names = &_cur_param_names;
}

int GetBlockCmd::encode_body_params(const std::map<std::string, std::string> & params, xJson::Value & root) {
    if (nullptr == _param_names)
        return 0;

    auto it = _param_names->begin();
    for (; it != _param_names->end(); ++it) {
        const std::string & name = *it;
        auto it_v = params.find(*it);
        if (it_v != params.end()) {
            if (ParamBlockHeight == name) {
                // root["params"][name] = atoi(it_v->second.c_str());
                root["params"][name] = it_v->second;
            } else {
                root["params"][name] = it_v->second;
            }
        } else {
#ifdef TRANS_EMPTY_PARAMS
            root["params"][name] = "";
#endif  // TRANS_EMPTY_PARAMS
        }
    }
    return 0;
}

void GetBlockCmd::decode(const std::string & params, task_info * info) {
}

void NodeRegisterCmd::decode(const std::string & params, task_info * info) {
}

void NodeDeRegisterCmd::decode(const std::string & params, task_info * info) {
}

void NodeRedeemCmd::decode(const std::string & params, task_info * info) {
}

void SetVoteCmd::decode(const std::string & params, task_info * info) {
}

void ClaimNodeRewardCmd::decode(const std::string & params, task_info * info) {
}

void ClaimRewardCmd::decode(const std::string & params, task_info * info) {
}

void AddProposalCmd::decode(const std::string & params, task_info * info) {
}

void WithdrawProposalCmd::decode(const std::string & params, task_info * info) {
}

void VoteProposalCmd::decode(const std::string & params, task_info * info) {
}

ChainInfoCmd::ChainInfoCmd() {
    _param_names = &_cur_param_names;
}

// int ChainInfoCmd::encode_body_params(const std::map<std::string, std::string>& params, xJson::Value& root)
// {
// 	if (nullptr == _param_names)
// 		return 0;

// 	auto it = _param_names->begin();
// 	for (; it != _param_names->end(); ++it)
// 	{
// 		const std::string& name = *it;
// 		auto it_v = params.find(*it);
//         cout << "    " << name << "  " << it_v->second << endl;
// 		if (it_v != params.end())
// 		{
// 			root["params"][name] = it_v->second;
// 		}
// 	}
// 	return 0;
// }

ChainInfoCmd::~ChainInfoCmd() {
    // std::cout << "Destroy ChainInfoCmd" << std::endl;
}

void ChainInfoCmd::decode(const std::string & params, task_info * info) {
    xJson::Reader reader;
    xJson::Value root;

    auto result = std::make_shared<ChainInfoResult>();

    try {
        if (reader.parse(params, root)) {
            result->version = root["version"].asString();
            result->first_timerblock_hash = root["first_timerblock_hash"].asString();
            result->first_timerblock_stamp = root["first_timerblock_stamp"].asUInt64();
        }
    } catch (...) {
        result->error = static_cast<int>(ErrorCode::JsonParseError);
        result->errmsg = "Json Parse Error.";
    }
}

NodeInfoCmd::NodeInfoCmd() {
    _param_names = &_cur_param_names;
}

NodeInfoCmd::~NodeInfoCmd() {
    // std::cout << "Destroy NodeInfoCmd" << std::endl;
}

void NodeInfoCmd::decode(const std::string & params, task_info * info) {
    xJson::Reader reader;
    xJson::Value root;

    auto result = std::make_shared<NodeInfoResult>();

    try {
        if (reader.parse(params, root)) {
        }
    } catch (...) {
        result->error = static_cast<int>(ErrorCode::JsonParseError);
        result->errmsg = "Json Parse Error.";
    }
}

ElectInfoCmd::ElectInfoCmd() {
    _param_names = &_cur_param_names;
}

ElectInfoCmd::~ElectInfoCmd() {
    // std::cout << "Destroy NodeInfoCmd" << std::endl;
}

void ElectInfoCmd::decode(const std::string & params, task_info * info) {
    xJson::Reader reader;
    xJson::Value root;

    auto result = std::make_shared<NodeInfoResult>();

    try {
        if (reader.parse(params, root)) {
        }
    } catch (...) {
        result->error = static_cast<int>(ErrorCode::JsonParseError);
        result->errmsg = "Json Parse Error.";
    }
}

StandbysInfoCmd::StandbysInfoCmd() {
    _param_names = &_cur_param_names;
}

StandbysInfoCmd::~StandbysInfoCmd() {
}

void StandbysInfoCmd::decode(const std::string & params, task_info * info) {
}

NodeRewardCmd::NodeRewardCmd() {
    _param_names = &_cur_param_names;
}

NodeRewardCmd::~NodeRewardCmd() {
    // std::cout << "Destroy NodeRewardCmd" << std::endl;
}

void NodeRewardCmd::decode(const std::string & params, task_info * info) {
    xJson::Reader reader;
    xJson::Value root;

    auto result = std::make_shared<NodeRewardResult>();

    try {
        if (reader.parse(params, root)) {
        }
    } catch (...) {
        result->error = static_cast<int>(ErrorCode::JsonParseError);
        result->errmsg = "Json Parse Error.";
    }
}

VoteDistCmd::VoteDistCmd() {
    _param_names = &_cur_param_names;
}

VoteDistCmd::~VoteDistCmd() {
    // std::cout << "Destroy VoteDistCmd" << std::endl;
}

void VoteDistCmd::decode(const std::string & params, task_info * info) {
}

VoterRewardCmd::VoterRewardCmd() {
    _param_names = &_cur_param_names;
}

VoterRewardCmd::~VoterRewardCmd() {
    // std::cout << "Destroy VoterRewardCmd" << std::endl;
}

void VoterRewardCmd::decode(const std::string & params, task_info * info) {
    xJson::Reader reader;
    xJson::Value root;

    auto result = std::make_shared<VoterRewardResult>();

    try {
        if (reader.parse(params, root)) {
        }
    } catch (...) {
        result->error = static_cast<int>(ErrorCode::JsonParseError);
        result->errmsg = "Json Parse Error.";
    }
}

GetProposalCmd::GetProposalCmd() {
    _param_names = &_cur_param_names;
}

GetProposalCmd::~GetProposalCmd() {
    // std::cout << "Destroy GetProposalCmd" << std::endl;
}

void GetProposalCmd::decode(const std::string & params, task_info * info) {
    xJson::Reader reader;
    xJson::Value root;

    auto result = std::make_shared<GetProposalResult>();

    try {
        if (reader.parse(params, root)) {
        }
    } catch (...) {
        result->error = static_cast<int>(ErrorCode::JsonParseError);
        result->errmsg = "Json Parse Error.";
    }
}

}  // namespace xChainSDK
