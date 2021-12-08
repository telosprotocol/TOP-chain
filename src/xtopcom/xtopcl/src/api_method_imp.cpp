#include "api_method_imp.h"

#include "base/log.h"
#include "base/utility.h"
#include "global_definition.h"
#include "task/request_task.h"
#include "task/task_dispatcher.h"
#include "topchain_type.h"
#include "xaction_param.h"
#include "xcrypto/xckey.h"
#include "xcrypto_util.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xtransaction.h"
#include "xrpc/xuint_format.h"
#include "xvm/xvm_define.h"

#include <stdio.h>

#include <algorithm>
#include <memory>

using namespace top::utl;

namespace xChainSDK {
using namespace top::xrpc;
using namespace top::data;
using namespace top::xvm;
using std::cout;
using std::endl;

uint32_t get_sequence_id() {
    static uint32_t sequence_id = 0;
    return ++sequence_id;
}

template <typename T>
static void set_user_info(task_info_callback<T> * info,
                          const user_info & uinfo,
                          const std::string & method,
                          typename task_info_callback<T>::callback_t func,
                          bool use_transaction = true);

bool api_method_imp::ChangeHost(const std::string host) {
    if (utility::is_ipaddress_valid(host)) {
        g_server_host_port = host;
        return true;
    }
    return false;
}

bool api_method_imp::make_private_key(std::array<uint8_t, PRI_KEY_LEN> & private_key, std::string & address) {
    xcrypto_util::make_private_key(private_key);
    address = xcrypto_util::make_address_by_assigned_key(private_key);
    return !address.empty();
}

bool api_method_imp::make_child_private_key(const std::string & parent_addr, std::array<uint8_t, PRI_KEY_LEN> & private_key, std::string & address) {
    xcrypto_util::make_private_key(private_key);
    address = xcrypto_util::make_child_address_by_assigned_key(parent_addr, private_key, top::base::enum_vaccount_addr_type_secp256k1_user_sub_account);
    return !address.empty();
}

bool api_method_imp::set_private_key(user_info & uinfo, const std::string & private_key) {
    std::vector<uint8_t> keys = hex_to_uint(private_key);
    if (keys.size() != 32)
        return false;
    for (size_t i = 0; i < keys.size(); ++i) {
        uinfo.private_key[i] = keys[i];
    }
    uinfo.account = xcrypto_util::make_address_by_assigned_key(uinfo.private_key);
    return !uinfo.account.empty();
}

bool api_method_imp::validate_key(const std::string & priv, const std::string & pub, const std::string & addr, const std::string & main_addr) {
    std::vector<uint8_t> keys = hex_to_uint(priv);
    if (keys.size() != 32)
        return false;

    xecprikey_t ecpriv(keys.data());
    xecpubkey_t ecpub = ecpriv.get_public_key();
    if (pub != uint_to_str(ecpub.data(), ecpub.size()))
        return false;

    std::string regenerate_addr;
    if (main_addr.empty()) {
        regenerate_addr = ecpub.to_address(top::base::enum_vaccount_addr_type_secp256k1_user_account,
                                           top::base::xvaccount_t::make_ledger_id(top::base::enum_main_chain_id, top::base::enum_chain_zone_consensus_index));
    } else {
        regenerate_addr = ecpub.to_address(main_addr,
                                           top::base::enum_vaccount_addr_type_secp256k1_user_sub_account,
                                           top::base::xvaccount_t::make_ledger_id(top::base::enum_main_chain_id, top::base::enum_chain_zone_consensus_index));
    }

    return addr == regenerate_addr;
}

bool api_method_imp::create_account(const user_info & uinfo, std::function<void(ResultBase *)> func) {
    if (uinfo.account.empty()) {
        LOG("uinfo.account.empty()=", uinfo.account.empty());
        return false;
    }

    auto info = new task_info_callback<ResultBase>();
    set_user_info(info, uinfo, CMD_CREATE_ACCOUNT, func);

    top::base::xstream_t stream(top::base::xcontext_t::instance());
    std::string param = stream_params(stream, uinfo.account);
    auto tx_info = top::data::xtx_action_info(uinfo.account, "", "", uinfo.account, "", param);
    info->trans_action->construct_tx(xtransaction_type_create_user_account, 100, m_deposit, 0, "", tx_info);

    if (!hash_signature(info->trans_action.get(), uinfo.private_key)) {
        delete info;
        return false;
    }

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto res = task_dispatcher::get_instance()->get_result();
    cout << res;
    return true;
}

bool api_method_imp::getAccount(const user_info & uinfo, const std::string & account, std::ostringstream & out_str, std::function<void(AccountInfoResult *)> func) {
    if (uinfo.account.empty()) {
        LOG("uinfo.account.empty()=", uinfo.account.empty());
        return false;
    }

    auto info = new task_info_callback<AccountInfoResult>();
    set_user_info(info, uinfo, CMD_ACCOUNT_INFO, func, false);

    // body->params
    info->params["account_addr"] = account;
    info->callback_ = func;
    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::passport(const user_info & uinfo, std::function<void(RequestTokenResult *)> func) {
    if (uinfo.account.empty())
        return false;

    auto info = new task_info_callback<RequestTokenResult>();
    set_user_info(info, uinfo, CMD_TOKEN, func, false);
    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    task_dispatcher::get_instance()->get_result();
    return true;
}

bool api_method_imp::key_store(const user_info & uinfo, const std::string & type, const std::string & value, std::function<void(ResultBase *)> func) {
    xassert(false); // XTODO not support
    return true;
}

bool api_method_imp::transfer(const user_info & uinfo,
                              const std::string & from,
                              const std::string & to,
                              uint64_t amount,
                              const std::string & memo,
                              std::ostringstream & out_str,
                              std::function<void(TransferResult *)> func) {
    auto info = new task_info_callback<TransferResult>();
    set_user_info(info, uinfo, CMD_TRANSFER, func);

    // body->params
    xaction_asset_param asset_param(this, "", amount);
    std::string param = asset_param.create();

    info->trans_action->set_memo(memo);
    info->trans_action->set_deposit(m_deposit);
    info->trans_action->set_tx_type(xtransaction_type_transfer);
    info->trans_action->set_last_nonce(uinfo.nonce);
    info->trans_action->set_fire_timestamp(get_timestamp());
    info->trans_action->set_expire_duration(100);

    if (info->trans_action->get_tx_version() == xtransaction_version_2) {
        info->trans_action->set_amount(amount);
        info->trans_action->set_source_addr(from);
        info->trans_action->set_target_addr(to);
    } else {
        info->trans_action->set_last_hash(uinfo.last_hash_xxhash64);
        info->trans_action->set_source_action_type(xaction_type_asset_out);
        info->trans_action->set_source_addr(from);
        info->trans_action->set_source_action_para(param);
        info->trans_action->set_target_action_type(xaction_type_asset_in);
        info->trans_action->set_target_addr(to);
        info->trans_action->set_target_action_para(param);
    }
    
    if (!hash_signature(info->trans_action.get(), uinfo.private_key)) {
        delete info;
        return false;
    }

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::stakeGas(const user_info & uinfo,
                              const std::string & from,
                              const std::string & to,
                              uint64_t amount,
                              std::ostringstream & out_str,
                              std::function<void(TransferResult *)> func) {
    auto info = new task_info_callback<TransferResult>();
    set_user_info(info, uinfo, CMD_PLEDGETGAS, func);

    xaction_asset_param asset_param(this, "", amount);
    std::string param = asset_param.create();
    auto tx_info = top::data::xtx_action_info(from, "", "", to, "", param);
    info->trans_action->construct_tx(xtransaction_type_pledge_token_tgas, 100, m_deposit, uinfo.nonce, "", tx_info);
    if (!hash_signature(info->trans_action.get(), uinfo.private_key)) {
        delete info;
        return false;
    }

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::unStakeGas(const user_info & uinfo,
                                const std::string & from,
                                const std::string & to,
                                uint64_t amount,
                                std::ostringstream & out_str,
                                std::function<void(TransferResult *)> func) {
    auto info = new task_info_callback<TransferResult>();
    set_user_info(info, uinfo, CMD_REDEEMTGAS, func);

    xaction_asset_param asset_param(this, "", amount);
    std::string param = asset_param.create();
    auto tx_info = top::data::xtx_action_info(from, "", "", to, "", param);
    info->trans_action->construct_tx(xtransaction_type_redeem_token_tgas, 100, m_deposit, uinfo.nonce, "", tx_info);

    if (!hash_signature(info->trans_action.get(), uinfo.private_key)) {
        delete info;
        return false;
    }

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::getTransaction(const user_info & uinfo,
                                    const std::string & account,
                                    const std::string & last_hash,
                                    std::ostringstream & out_str,
                                    std::function<void(AccountTransactionResult *)> func) {
    if (uinfo.account.empty()) {
        LOG("uinfo.account.empty()=", uinfo.account.empty());
        return false;
    }

    auto info = new task_info_callback<AccountTransactionResult>();
    set_user_info(info, uinfo, CMD_ACCOUNT_TRANSACTION, func, false);

    // body->params
    info->params["account_addr"] = account;
    info->params["tx_hash"] = last_hash;

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::deployContract(user_info & uinfo,
                                    std::string & contract_account,
                                    const uint64_t tgas_limit,
                                    const uint64_t amount,
                                    const std::string & contract_code,
                                    std::ostringstream & out_str,
                                    std::function<void(PublishContractResult *)> func) {
    xassert(false);
    return true;
}

std::string serialize(const std::string & params) {
    std::cout << "original para: " << params << std::endl;
    if (params == "") {
        return "";
    }
    std::vector<std::string> v_p;
    size_t beg = 0;
    size_t end = 0;
    while ((end = params.find('|', beg)) != std::string::npos) {
        v_p.push_back(params.substr(beg, end - beg));
        std::cout << params.substr(beg, end - beg) << std::endl;
        beg = end + 1;
    }
    v_p.push_back(params.substr(beg, end - beg));

    top::base::xstream_t stream(top::base::xcontext_t::instance());
    // the number of parameters
    stream << static_cast<uint8_t>(v_p.size());
    for (auto pair : v_p) {
        std::cout << pair << std::endl;
        int type = std::stoi(pair.substr(0, 1));
        switch (type) {
        case 1: {
            uint64_t num = std::stoull(pair.substr(2));
            stream << ARG_TYPE_UINT64;
            stream << num;
            break;
        }

        case 2: {
            stream << ARG_TYPE_STRING;
            stream << pair.substr(2);
            break;
        }

        case 3: {
            bool flag(true);
            if (pair.substr(2) == "false") {
                flag = false;
            }
            stream << ARG_TYPE_BOOL;
            stream << flag;
            break;
        }

        default:
            break;
        }
    }

    return std::string((char *)stream.data(), stream.size());
}

bool api_method_imp::runContract(const user_info & uinfo,
                                 const uint64_t amount,
                                 const std::string & contract_account,
                                 const std::string & contract_func,
                                 const std::string & contract_params,
                                 std::ostringstream & out_str,
                                 std::function<void(CallContractResult *)> func) {
    auto info = new task_info_callback<CallContractResult>();
    set_user_info(info, uinfo, CMD_CALL_CONTRACT, func);

    xaction_asset_param asset_param(this, "", amount);
    info->trans_action->set_source_action_para(asset_param.create());
    std::string code_stream = serialize(contract_params);
    auto tx_info = top::data::xtx_action_info(uinfo.account, "", asset_param.create(), contract_account, contract_func, code_stream);
    info->trans_action->construct_tx(xtransaction_type_run_contract, 100, m_deposit, uinfo.nonce, "", tx_info);

    if (!hash_signature(info->trans_action.get(), uinfo.private_key)) {
        delete info;
        return false;
    }

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::getProperty(const user_info & uinfo,
                                 const std::string & account,
                                 const std::string & type,
                                 const std::string & data1,
                                 const std::string & data2,
                                 std::function<void(GetPropertyResult *)> func) {
    auto info = new task_info_callback<GetPropertyResult>();
    set_user_info(info, uinfo, CMD_GET_PROPERTY, func, false);

    // body->params
    info->params["account_addr"] = account;
    info->params["type"] = type;
    info->params["data1"] = data1;
    info->params["data2"] = data2;

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);
    return true;
}

bool api_method_imp::hash_signature(top::data::xtransaction_t * trans_action, const std::array<std::uint8_t, PRI_KEY_LEN> & private_key) {
    trans_action->set_digest();
    std::string auth_str = xcrypto_util::digest_sign(trans_action->digest(), private_key);
    trans_action->set_authorization(uint_to_str(auth_str.c_str(), auth_str.size()));
    return true;
}

bool api_method_imp::create_sub_account(const user_info & uinfo,
                                        const std::string & child_address,
                                        const std::array<std::uint8_t, PRI_KEY_LEN> & child_private_key,
                                        std::function<void(CreateSubAccountResult *)> func) {
    xassert(false);
    return true;
}

bool api_method_imp::make_private_key(std::array<uint8_t, PRI_KEY_LEN> & private_key) {
    xcrypto_util::make_private_key(private_key);
    return true;
}

std::string api_method_imp::get_public_key(const std::array<uint8_t, PRI_KEY_LEN> & private_key) {
    xecprikey_t pri_key_obj((uint8_t *)private_key.data());
    xecpubkey_t pub_key_obj = pri_key_obj.get_public_key();
    return uint_to_str(pub_key_obj.data(), pub_key_obj.size());
}

std::string api_method_imp::make_account_address(std::array<uint8_t, PRI_KEY_LEN> & private_key, const uint8_t addr_type, const std::string & parent_addr) {
    std::string address{""};
    xecprikey_t pri_key_obj(private_key.data());
    xecpubkey_t pub_key_obj = pri_key_obj.get_public_key();

    if (addr_type == top::base::enum_vaccount_addr_type_secp256k1_user_account) {
        address = pub_key_obj.to_address(addr_type, top::base::xvaccount_t::make_ledger_id(top::base::enum_main_chain_id, top::base::enum_chain_zone_consensus_index));
    } else if (addr_type == top::base::enum_vaccount_addr_type_secp256k1_user_sub_account || addr_type == top::base::enum_vaccount_addr_type_custom_contract) {
        if (!parent_addr.empty()) {
            address =
                pub_key_obj.to_address(parent_addr, addr_type, top::base::xvaccount_t::make_ledger_id(top::base::enum_main_chain_id, top::base::enum_chain_zone_consensus_index));
        }
    } else {
        assert(0);
    }
    return address;
}

std::string api_method_imp::get_sign(const top::uint256_t & hash, std::array<uint8_t, PRI_KEY_LEN> & private_key) {
    std::string auth_str = xcrypto_util::digest_sign(hash, private_key);
    return uint_to_str(auth_str.c_str(), auth_str.size());
}

bool api_method_imp::getChainInfo(const user_info & uinfo, std::ostringstream & out_str, std::function<void(ChainInfoResult *)> func) {
    if (uinfo.account.empty())
        return false;

    auto info = new task_info_callback<ChainInfoResult>();
    set_user_info(info, uinfo, CMD_CHAIN_INFO, func, false);
    info->params["account_addr"] = uinfo.account;

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::queryNodeInfo(const user_info & uinfo, const std::string & target, std::ostringstream & out_str, std::function<void(NodeInfoResult *)> func) {
    if (uinfo.account.empty())
        return false;

    auto info = new task_info_callback<NodeInfoResult>();
    set_user_info(info, uinfo, CMD_NODE_INFO, func, false);
    info->params["account_addr"] = uinfo.account;
    info->params["node_account_addr"] = target;

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::getElectInfo(const user_info & uinfo, const std::string & target, std::function<void(ElectInfoResult *)> func) {
    if (uinfo.account.empty())
        return false;

    auto info = new task_info_callback<ElectInfoResult>();
    set_user_info(info, uinfo, CMD_ELECT_INFO, func, false);
    info->params["account_addr"] = uinfo.account;
    info->params["node_account_addr"] = target;

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    xJson::Value jv;
    xJson::Reader reader;
    if (reader.parse(rpc_response, jv)) {
        auto data = jv["data"];
        if (data.isNull()) {
            auto err_no = jv["errno"].asInt();
            auto errmsg = jv["errmsg"].asString();
            if (errmsg == "OK" && err_no == 0) {
                cout << "No data!" << endl;
            } else {
                cout << "[" << err_no << "]Errmsg: " << errmsg << endl;
            }
        } else {
            cout << data.asString() << endl;
        }
    }

    return true;
}

bool api_method_imp::getStandbys(const user_info & uinfo, const std::string & account, std::function<void(AccountInfoResult *)> func) {
    if (uinfo.account.empty()) {
        return false;
    }

    auto info = new task_info_callback<AccountInfoResult>();
    set_user_info(info, uinfo, CMD_STANDBYS_INFO, func, false);

    // body->params
    info->params["account_addr"] = uinfo.account;
    info->params["node_account_addr"] = account;
    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    xJson::Value jv;
    xJson::Reader reader;
    if (reader.parse(rpc_response, jv)) {
        auto data = jv["data"];
        if (data.isNull()) {
            cout << "The miner has not joined in node-standby-pool, please start node by command 'topio node startnode'." << endl;
            return false;
        } else {
            // cout << "The miner has joined in node-standby-pool. ";
        }
    }

    return true;
}

bool api_method_imp::queryNodeReward(const user_info & uinfo, const std::string & target, std::ostringstream & out_str, std::function<void(NodeRewardResult *)> func) {
    if (uinfo.account.empty())
        return false;

    auto info = new task_info_callback<NodeRewardResult>();
    set_user_info(info, uinfo, CMD_NODE_REWARD, func, false);
    info->params["account_addr"] = uinfo.account;
    info->params["node_account_addr"] = target;

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::listVoteUsed(const user_info & uinfo, const std::string & target, std::ostringstream & out_str, std::function<void(VoteDistResult *)> func) {
    if (uinfo.account.empty())
        return false;

    auto info = new task_info_callback<VoteDistResult>();
    set_user_info(info, uinfo, CMD_VOTE_DIST, func, false);
    info->params["account_addr"] = uinfo.account;
    info->params["node_account_addr"] = target;

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::queryVoterDividend(const user_info & uinfo, const std::string & target, std::ostringstream & out_str, std::function<void(VoterRewardResult *)> func) {
    if (uinfo.account.empty())
        return false;

    auto info = new task_info_callback<VoterRewardResult>();
    set_user_info(info, uinfo, CMD_VOTER_REWARD, func, false);
    info->params["account_addr"] = uinfo.account;
    info->params["node_account_addr"] = target;

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::queryProposal(const user_info & uinfo, const std::string & target, std::ostringstream & out_str, std::function<void(GetProposalResult *)> func) {
    if (uinfo.account.empty())
        return false;

    auto info = new task_info_callback<GetProposalResult>();
    set_user_info(info, uinfo, CMD_GET_PROPOSAL, func, false);
    info->params["account_addr"] = uinfo.account;
    info->params["node_account_addr"] = target;
    info->params["proposal_version"] = "v1";

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::getCGP(const user_info & uinfo, const std::string & target, std::ostringstream & out_str, std::function<void(GetProposalResult *)> func) {
    if (uinfo.account.empty())
        return false;

    auto info = new task_info_callback<GetProposalResult>();
    set_user_info(info, uinfo, CMD_GET_ONCHAINPARAMS, func, false);
    info->params["account_addr"] = uinfo.account;

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

uint64_t api_method_imp::get_timestamp() {
    return utility::gmttime_ms() / 1000;
}

bool api_method_imp::make_keys_base64(std::string & private_key, std::string & public_key, std::string & address) {
    xecprikey_t prikey;

    std::string pubkey = prikey.get_compress_public_key();

    private_key = utility::base64_encode((unsigned char const *)prikey.data(), (unsigned int)prikey.size());
    public_key = utility::base64_encode((unsigned char const *)pubkey.c_str(), (unsigned int)pubkey.size());
    //        std::string pubhex = uint_to_str2(pubkey.data(), pubkey.size());

    xecpubkey_t pub_key_obj = prikey.get_public_key();
    address = pub_key_obj.to_address(top::base::enum_vaccount_addr_type_secp256k1_user_account,
                                     top::base::xvaccount_t::make_ledger_id(top::base::enum_main_chain_id, top::base::enum_chain_zone_consensus_index));

    std::string initpri = std::string((char *)prikey.data(), prikey.size());
    std::string decodepri = utility::base64_decode(private_key);
    return (initpri == decodepri);
}

void api_method_imp::make_keys_base64(std::ostream & out) {
    // main key
    xecprikey_t main_privkey;
    xecpubkey_t main_pubkey = main_privkey.get_public_key();

    auto main_priv_str = uint_to_str(main_privkey.data(), main_privkey.size());
    auto main_pub_str = uint_to_str(main_pubkey.data(), main_pubkey.size());
    auto main_address = main_pubkey.to_address(top::base::enum_vaccount_addr_type_secp256k1_user_account,
                                               top::base::xvaccount_t::make_ledger_id(top::base::enum_main_chain_id, top::base::enum_chain_zone_consensus_index));
    out << utility::base64_encode((unsigned char const *)main_priv_str.data(), (unsigned int)main_priv_str.size()) << ',';
    out << main_pub_str << ',';
    out << main_address << '\n';

    // sub keys
    int sub_keys = 2;
    for (auto i = 0; i < sub_keys; ++i) {
        xecprikey_t sub_privkey;
        xecpubkey_t sub_pubkey = sub_privkey.get_public_key();

        auto sub_priv_str = uint_to_str(sub_privkey.data(), sub_privkey.size());
        auto sub_pub_str = uint_to_str(sub_pubkey.data(), sub_pubkey.size());
        auto sub_address = sub_pubkey.to_address(main_address,
                                                 top::base::enum_vaccount_addr_type_secp256k1_user_sub_account,
                                                 top::base::xvaccount_t::make_ledger_id(top::base::enum_main_chain_id, top::base::enum_chain_zone_consensus_index));
        out << utility::base64_encode((unsigned char const *)sub_priv_str.data(), (unsigned int)sub_priv_str.size()) << ',';
        out << sub_pub_str << ',';
        out << sub_address << '\n';
    }
}

bool api_method_imp::getBlock(const user_info & uinfo,
                              const std::string & owner,
                              const std::string & height,
                              std::ostringstream & out_str,
                              std::function<void(GetBlockResult *)> func) {
    if (uinfo.account.empty()) {
        LOG("uinfo.account.empty()=", uinfo.account.empty(), " uinfo.identity_token.empty()=", uinfo.identity_token.empty());
        return false;
    }

    auto info = new task_info_callback<GetBlockResult>();
    set_user_info(info, uinfo, CMD_GET_BLOCK, func, false);

    // body->params
    // info->params[ParamBlockOwner] = owner;
    // info->params[ParamGetBlockType] = type;
    info->params[ParamAccount] = owner;
    info->params[ParamBlockHeight] = height;
    info->callback_ = func;
    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::registerNode(const user_info & uinfo,
                                  const uint64_t mortgage,
                                  const std::string & role,
                                  const std::string & nickname,
                                  const std::string & signing_key,
                                  const uint32_t dividend_rate,
                                  std::ostringstream & out_str,
                                  std::function<void(NodeRegResult *)> func) {
    auto info = new task_info_callback<NodeRegResult>();
    set_user_info(info, uinfo, CMD_NODE_REGISTER, func);

    std::string target_action_name = "registerNode";
    top::base::xstream_t stream_t(top::base::xcontext_t::instance());
#ifdef XENABLE_MOCK_ZEC_STAKE
    std::string param_t = stream_params(stream_t, role, nickname, signing_key, dividend_rate, uinfo.account);
#else
    std::string param_t = stream_params(stream_t, role, nickname, signing_key, dividend_rate);
#endif
    xaction_asset_param asset_param(this, "", mortgage);
    std::string param = asset_param.create();

    auto tx_info = top::data::xtx_action_info(uinfo.account, "", param, top::sys_contract_rec_registration_addr, target_action_name, param_t);
    info->trans_action->construct_tx(xtransaction_type_run_contract, 100, m_deposit, uinfo.nonce, "", tx_info);

    if (!hash_signature(info->trans_action.get(), uinfo.private_key)) {
        delete info;
        return false;
    }

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::updateNodeType(const user_info & uinfo, const std::string & role, std::ostringstream & out_str, std::function<void(NodeRegResult *)> func) {
    auto info = new task_info_callback<NodeRegResult>();
    set_user_info(info, uinfo, CMD_NODE_REGISTER, func);

    xaction_asset_param asset_param(this, "", 0);
    std::string param_s = asset_param.create();

    top::base::xstream_t stream_t(top::base::xcontext_t::instance());
    std::string target_action_name = "updateNodeType";
    std::string param_t = stream_params(stream_t, role);

    auto tx_info = top::data::xtx_action_info(uinfo.account, "", param_s, top::sys_contract_rec_registration_addr, target_action_name, param_t);
    info->trans_action->construct_tx(xtransaction_type_run_contract, 100, m_deposit, uinfo.nonce, "", tx_info);

    if (!hash_signature(info->trans_action.get(), uinfo.private_key)) {
        delete info;
        return false;
    }

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::stake_node_deposit(const user_info & uinfo, uint64_t deposit, std::ostringstream & out_str, std::function<void(NodeRegResult *)> func) {
    auto info = new task_info_callback<NodeRegResult>();
    set_user_info(info, uinfo, CMD_NODE_REGISTER, func);

    std::string target_action_name;
    top::base::xstream_t stream_t(top::base::xcontext_t::instance());
    target_action_name = "stakeDeposit";
    xaction_asset_param asset_param(this, "", deposit);
    std::string param = asset_param.create();

    auto tx_info = top::data::xtx_action_info(uinfo.account, "", param, top::sys_contract_rec_registration_addr, target_action_name, "");
    info->trans_action->construct_tx(xtransaction_type_run_contract, 100, m_deposit, uinfo.nonce, "", tx_info);

    if (!hash_signature(info->trans_action.get(), uinfo.private_key)) {
        delete info;
        return false;
    }

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::updateNodeInfo(const user_info & uinfo,
                                    const std::string & role,
                                    const std::string & name,
                                    const uint32_t type,
                                    uint64_t mortgage,
                                    const uint32_t rate,
                                    const std::string & node_sign_key,
                                    std::ostringstream & out_str,
                                    std::function<void(NodeRegResult *)> func) {
    auto info = new task_info_callback<NodeRegResult>();
    set_user_info(info, uinfo, CMD_NODE_REGISTER, func);

    // decrease node deposit
    uint64_t transfer_amount = mortgage;
    if (type == 2) {
        transfer_amount = 0;
    }
    xaction_asset_param asset_param(this, "", transfer_amount);
    std::string param = asset_param.create();

    std::string target_action_name;
    std::string param_t;
    top::base::xstream_t stream_t(top::base::xcontext_t::instance());
    target_action_name = "updateNodeInfo";
    param_t = stream_params(stream_t, name, type, mortgage, rate, role, node_sign_key);

    auto tx_info = top::data::xtx_action_info(uinfo.account, "", param, top::sys_contract_rec_registration_addr, target_action_name, param_t);
    info->trans_action->construct_tx(xtransaction_type_run_contract, 100, m_deposit, uinfo.nonce, "", tx_info);

    if (!hash_signature(info->trans_action.get(), uinfo.private_key)) {
        delete info;
        return false;
    }

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::unstake_node_deposit(const user_info & uinfo, uint64_t unstake_deposit, std::ostringstream & out_str, std::function<void(NodeRegResult *)> func) {
    auto info = new task_info_callback<NodeRegResult>();
    set_user_info(info, uinfo, CMD_NODE_REGISTER, func);

    xaction_asset_param asset_param(this, "", 0);
    std::string param_s = asset_param.create();

    std::string target_action_name = "unstakeDeposit";
    top::base::xstream_t stream_t(top::base::xcontext_t::instance());
    std::string param_t = stream_params(stream_t, unstake_deposit);

    auto tx_info = top::data::xtx_action_info(uinfo.account, "", param_s, top::sys_contract_rec_registration_addr, target_action_name, param_t);
    info->trans_action->construct_tx(xtransaction_type_run_contract, 100, m_deposit, uinfo.nonce, "", tx_info);

    if (!hash_signature(info->trans_action.get(), uinfo.private_key)) {
        delete info;
        return false;
    }

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::setNodeName(const user_info & uinfo, const std::string & nickname, std::ostringstream & out_str, std::function<void(NodeRegResult *)> func) {
    auto info = new task_info_callback<NodeRegResult>();
    set_user_info(info, uinfo, CMD_NODE_REGISTER, func);

    xaction_asset_param asset_param(this, "", 0);
    std::string param_s = asset_param.create();

    top::base::xstream_t stream_t(top::base::xcontext_t::instance());
    std::string target_action_name = "setNodeName";
    std::string param_t = stream_params(stream_t, nickname);

    auto tx_info = top::data::xtx_action_info(uinfo.account, "", param_s, top::sys_contract_rec_registration_addr, target_action_name, param_t);
    info->trans_action->construct_tx(xtransaction_type_run_contract, 100, m_deposit, uinfo.nonce, "", tx_info);

    if (!hash_signature(info->trans_action.get(), uinfo.private_key)) {
        delete info;
        return false;
    }

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::updateNodeSignKey(const user_info & uinfo, const std::string & node_sign_key, std::ostringstream & out_str, std::function<void(NodeRegResult *)> func) {
    auto info = new task_info_callback<NodeRegResult>();
    set_user_info(info, uinfo, CMD_NODE_REGISTER, func);

    xaction_asset_param asset_param(this, "", 0);
    std::string param_s = asset_param.create();

    top::base::xstream_t stream_t(top::base::xcontext_t::instance());
    std::string target_action_name = "updateNodeSignKey";
    std::string param_t = stream_params(stream_t, node_sign_key);

    auto tx_info = top::data::xtx_action_info(uinfo.account, "", param_s, top::sys_contract_rec_registration_addr, target_action_name, param_t);
    info->trans_action->construct_tx(xtransaction_type_run_contract, 100, m_deposit, uinfo.nonce, "", tx_info);

    if (!hash_signature(info->trans_action.get(), uinfo.private_key)) {
        delete info;
        return false;
    }

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::unRegisterNode(const user_info & uinfo, std::ostringstream & out_str, std::function<void(NodeRegResult *)> func) {
    auto info = new task_info_callback<NodeRegResult>();
    set_user_info(info, uinfo, CMD_NODE_DEREGISTER, func);

    xaction_asset_param asset_param(this, "", 0);
    std::string param_s = asset_param.create();

    auto tx_info = top::data::xtx_action_info(uinfo.account, "", param_s, top::sys_contract_rec_registration_addr, "unregisterNode", "");
    info->trans_action->construct_tx(xtransaction_type_run_contract, 100, m_deposit, uinfo.nonce, "", tx_info);

    if (!hash_signature(info->trans_action.get(), uinfo.private_key)) {
        delete info;
        return false;
    }

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::redeemNodeDeposit(const user_info & uinfo, std::ostringstream & out_str, std::function<void(NodeRegResult *)> func) {
    auto info = new task_info_callback<NodeRegResult>();
    set_user_info(info, uinfo, CMD_REDEEM, func);

    xaction_asset_param asset_param(this, "", 0);
    std::string param_s = asset_param.create();

    auto tx_info = top::data::xtx_action_info(uinfo.account, "", param_s, top::sys_contract_rec_registration_addr, "redeemNodeDeposit", "");
    info->trans_action->construct_tx(xtransaction_type_run_contract, 100, m_deposit, uinfo.nonce, "", tx_info);

    if (!hash_signature(info->trans_action.get(), uinfo.private_key)) {
        delete info;
        return false;
    }

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::setDividendRatio(const user_info & uinfo, uint32_t dividend_rate, std::ostringstream & out_str, std::function<void(NodeRegResult *)> func) {
    auto info = new task_info_callback<NodeRegResult>();
    set_user_info(info, uinfo, CMD_NODE_REGISTER, func);

    xaction_asset_param asset_param(this, "", 0);
    std::string param_s = asset_param.create();

    top::base::xstream_t stream_t(top::base::xcontext_t::instance());
    std::string param_t = stream_params(stream_t, dividend_rate);

    auto tx_info = top::data::xtx_action_info(uinfo.account, "", param_s, top::sys_contract_rec_registration_addr, "setDividendRatio", param_t);
    info->trans_action->construct_tx(xtransaction_type_run_contract, 100, m_deposit, uinfo.nonce, "", tx_info);

    if (!hash_signature(info->trans_action.get(), uinfo.private_key)) {
        delete info;
        return false;
    }

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::stakeVote(const user_info & uinfo, uint64_t mortgage, uint16_t lock_duration, std::ostringstream & out_str, std::function<void(NodeRegResult *)> func) {
    auto info = new task_info_callback<NodeRegResult>();
    set_user_info(info, uinfo, CMD_PLEDGE_VOTE, func);

    xaction_pledge_token_vote_param pledge_vote_param(this, mortgage, lock_duration);
    std::string param = pledge_vote_param.create();

    auto tx_info = top::data::xtx_action_info(uinfo.account, "", "", uinfo.account, "", param);
    info->trans_action->construct_tx(xtransaction_type_pledge_token_vote, 100, m_deposit, uinfo.nonce, "", tx_info);

    if (!hash_signature(info->trans_action.get(), uinfo.private_key)) {
        delete info;
        return false;
    }

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::unStakeVote(const user_info & uinfo, uint64_t amount, std::ostringstream & out_str, std::function<void(NodeRegResult *)> func) {
    auto info = new task_info_callback<NodeRegResult>();
    set_user_info(info, uinfo, CMD_REDEEM_VOTE, func);
    xaction_redeem_token_vote_param asset_param(this, amount);
    std::string param = asset_param.create();

    auto tx_info = top::data::xtx_action_info(uinfo.account, "", "", uinfo.account, "", param);
    info->trans_action->construct_tx(xtransaction_type_redeem_token_vote, 100, m_deposit, uinfo.nonce, "", tx_info);

    if (!hash_signature(info->trans_action.get(), uinfo.private_key)) {
        delete info;
        return false;
    }

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::voteNode(const user_info & uinfo, std::map<std::string, int64_t> & vote_infos, std::ostringstream & out_str, std::function<void(SetVoteResult *)> func) {
    auto info = new task_info_callback<SetVoteResult>();
    set_user_info(info, uinfo, CMD_SET_VOTE, func);

    top::base::xstream_t stream_t(top::base::xcontext_t::instance());
    std::string param_t = stream_params(stream_t, vote_infos);

    auto tx_info = top::data::xtx_action_info(uinfo.account, "", "", top::sys_contract_sharding_vote_addr, "voteNode", param_t);
    info->trans_action->construct_tx(xtransaction_type_vote, 100, m_deposit, uinfo.nonce, "", tx_info);

    if (!hash_signature(info->trans_action.get(), uinfo.private_key)) {
        delete info;
        return false;
    }

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::unVoteNode(const user_info & uinfo, std::map<std::string, int64_t> & vote_infos, std::ostringstream & out_str, std::function<void(SetVoteResult *)> func) {
    auto info = new task_info_callback<SetVoteResult>();
    set_user_info(info, uinfo, CMD_SET_VOTE, func);

    top::base::xstream_t stream_t(top::base::xcontext_t::instance());
    std::string param_t = stream_params(stream_t, vote_infos);

    auto tx_info = top::data::xtx_action_info(uinfo.account, "", "", top::sys_contract_sharding_vote_addr, "unvoteNode", param_t);
    info->trans_action->construct_tx(xtransaction_type_abolish_vote, 100, m_deposit, uinfo.nonce, "", tx_info);

    if (!hash_signature(info->trans_action.get(), uinfo.private_key)) {
        delete info;
        return false;
    }

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::claimNodeReward(const user_info & uinfo, std::ostringstream & out_str, std::function<void(ClaimRewardResult *)> func) {
    auto info = new task_info_callback<ClaimRewardResult>();
    set_user_info(info, uinfo, CMD_CLAIM_NODE_REWARD, func);

    xaction_asset_param asset_param(this, "", 0);
    std::string param_s = asset_param.create();

    auto tx_info = top::data::xtx_action_info(uinfo.account, "", param_s, top::sys_contract_sharding_reward_claiming_addr, "claimNodeReward", "");
    info->trans_action->construct_tx(xtransaction_type_run_contract, 100, m_deposit, uinfo.nonce, "", tx_info);

    if (!hash_signature(info->trans_action.get(), uinfo.private_key)) {
        delete info;
        return false;
    }

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::claimVoterDividend(const user_info & uinfo, std::ostringstream & out_str, std::function<void(ClaimRewardResult *)> func) {
    auto info = new task_info_callback<ClaimRewardResult>();
    set_user_info(info, uinfo, CMD_CLAIM_REWARD, func);

    xaction_asset_param asset_param(this, "", 0);
    std::string param_s = asset_param.create();

    auto tx_info = top::data::xtx_action_info(uinfo.account, "", param_s, top::sys_contract_sharding_reward_claiming_addr, "claimVoterDividend", "");
    info->trans_action->construct_tx(xtransaction_type_run_contract, 100, m_deposit, uinfo.nonce, "", tx_info);

    if (!hash_signature(info->trans_action.get(), uinfo.private_key)) {
        delete info;
        return false;
    }

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::submitProposal(const user_info & uinfo,
                                    uint8_t type,
                                    const std::string & target,
                                    const std::string & value,
                                    uint64_t deposit,
                                    uint64_t effective_timer_height,
                                    std::ostringstream & out_str,
                                    std::function<void(AddProposalResult *)> func) {
    auto info = new task_info_callback<AddProposalResult>();
    set_user_info(info, uinfo, CMD_ADD_PROPOSAL, func);

    top::base::xstream_t stream_t(top::base::xcontext_t::instance());
    std::string param_t = stream_params(stream_t, target, value, type, effective_timer_height);

    xaction_asset_param asset_param(this, "", deposit);
    std::string param_s = asset_param.create();

    auto tx_info = top::data::xtx_action_info(uinfo.account, "", param_s, top::sys_contract_rec_tcc_addr, "submitProposal", param_t);
    info->trans_action->construct_tx(xtransaction_type_run_contract, 100, m_deposit, uinfo.nonce, "", tx_info);

    if (!hash_signature(info->trans_action.get(), uinfo.private_key)) {
        delete info;
        return false;
    }

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::withdrawProposal(const user_info & uinfo, const std::string & proposal_id, std::ostringstream & out_str, std::function<void(WithdrawProposalResult *)> func) {
    auto info = new task_info_callback<WithdrawProposalResult>();
    set_user_info(info, uinfo, CMD_WITHDRAW_PROPOSAL, func);

    xaction_asset_param asset_param(this, "", 0);
    std::string param_s = asset_param.create();

    top::base::xstream_t stream_t(top::base::xcontext_t::instance());
    std::string param_t = stream_params(stream_t, proposal_id);

    auto tx_info = top::data::xtx_action_info(uinfo.account, "", param_s, top::sys_contract_rec_tcc_addr, "withdrawProposal", param_t);
    info->trans_action->construct_tx(xtransaction_type_run_contract, 100, m_deposit, uinfo.nonce, "", tx_info);

    if (!hash_signature(info->trans_action.get(), uinfo.private_key)) {
        delete info;
        return false;
    }

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

bool api_method_imp::tccVote(const user_info & uinfo, const std::string & proposal_id, bool option, std::ostringstream & out_str, std::function<void(VoteProposalResult *)> func) {
    auto info = new task_info_callback<VoteProposalResult>();
    set_user_info(info, uinfo, CMD_VOTE_PROPOSAL, func);

    xaction_asset_param asset_param(this, "", 0);
    std::string param_s = asset_param.create();

    top::base::xstream_t stream_t(top::base::xcontext_t::instance());
    std::string param_t = stream_params(stream_t, proposal_id, option);

    auto tx_info = top::data::xtx_action_info(uinfo.account, "", param_s, top::sys_contract_rec_tcc_addr, "tccVote", param_t);
    info->trans_action->construct_tx(xtransaction_type_run_contract, 100, m_deposit, uinfo.nonce, "", tx_info);

    if (!hash_signature(info->trans_action.get(), uinfo.private_key)) {
        delete info;
        return false;
    }

    task_dispatcher::get_instance()->post_message(msgAddTask, (uint32_t *)info, 0);

    auto rpc_response = task_dispatcher::get_instance()->get_result();
    out_str << rpc_response;
    return true;
}

// void  api_method_imp::handle_result(std::string const& reuslt, ResultBase* reformat_result) {
//     result_queue.push(result);
// }

template <typename T>
static void set_user_info(task_info_callback<T> * info,
                          const user_info & uinfo,
                          const std::string & method,
                          typename task_info_callback<T>::callback_t func,
                          bool use_transaction) {
    info->use_transaction = use_transaction;
    info->host = g_server_host_port;
    info->callback_ = func;
    // only getTransaction & getBlock has 2.0 version rpc
    if (method == CMD_ACCOUNT_TRANSACTION || method == CMD_GET_BLOCK) {
        info->params["version"] = top::data::RPC_VERSION_V2;
    } else {
        info->params["version"] = top::data::RPC_VERSION_V1;
    }
    info->params["balance"] = uinfo.balance;
    info->method = method;
    info->params["target_account_addr"] = uinfo.account;
    info->params["method"] = use_transaction ? "sendTransaction" : method;
    info->params["identity_token"] = uinfo.identity_token;
    info->task_id = get_sequence_id();
    info->params["sequence_id"] = utility::num_to_str(info->task_id);

    // set secret key for rpc sign.
    // info->params[top::xChainRPC::xrpc_signature::secretkey_key_] = uinfo.secret_key;
    // info->params[top::xChainRPC::xrpc_signature::method_key_] = uinfo.sign_method;
    // info->params[top::xChainRPC::xrpc_signature::version_key_] = uinfo.sign_version;
}
}  // namespace xChainSDK
