#include "xrpc_eth_query_manager.h"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <trezor-crypto/sha3.h>
#include <secp256k1/secp256k1.h>
#include <secp256k1/secp256k1_recovery.h>

#include "xbase/xbase.h"
#include "xbase/xcontext.h"
#include "xbase/xint.h"
#include "xbase/xutl.h"
#include "xbasic/xutility.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xcommon/xip.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xcodec/xmsgpack/xelection/xelection_network_result_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xelection_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xstandby_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection_association_result_store_codec.hpp"
#include "xdata/xelection/xelection_association_result_store.h"
#include "xdata/xelection/xelection_cluster_result.h"
#include "xdata/xelection/xelection_cluster_result.h"
#include "xdata/xelection/xelection_group_result.h"
#include "xdata/xelection/xelection_info_bundle.h"
#include "xdata/xelection/xelection_network_result.h"
#include "xdata/xelection/xelection_result.h"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xelection/xstandby_result_store.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xtx_factory.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xproposal_data.h"
//#include "xdata/xslash.h"
#include "xdata/xtable_bstate.h"
#include "xdata/xtableblock.h"
#include "xdata/xtransaction_cache.h"
#include "xevm_common/fixed_hash.h"
#include "xevm_common/common_data.h"
#include "xevm_common/common.h"
#include "xevm_common/rlp.h"
#include "xevm_common/address.h"
#include "xrouter/xrouter.h"
#include "xrpc/xuint_format.h"
#include "xrpc/xrpc_loader.h"
#include "xstore/xaccount_context.h"
#include "xstore/xtgas_singleton.h"
#include "xtxexecutor/xtransaction_fee.h"
#include "xvledger/xvblock.h"
#include "xvledger/xvledger.h"
#include "xvm/manager/xcontract_address_map.h"
#include "xvm/manager/xcontract_manager.h"
#include "xmbus/xevent_behind.h"
#include "xdata/xblocktool.h"
#include "xpbase/base/top_utils.h"
#include "xutility/xhash.h"

#include "xtxexecutor/xvm_face.h"
#include "xevm/xevm.h"
#include "xdata/xtransaction_v2.h"
#include "xdata/xtransaction_v3.h"
#include "xstatectx/xstatectx.h"

using namespace top::data;

namespace top {
namespace xrpc {
using namespace std;
using namespace base;
using namespace store;
using namespace xrpc;

void xrpc_eth_query_manager::call_method(std::string strMethod, xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    auto iter = m_query_method_map.find(strMethod);
    if (iter != m_query_method_map.end()) {
        (iter->second)(js_req, js_rsp, strResult, nErrorCode);
    }
}

bool xrpc_eth_query_manager::handle(std::string & strReq, xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    std::string action = js_req["action"].asString();
    auto iter = m_query_method_map.find(action);
    if (iter != m_query_method_map.end()) {
        iter->second(js_req, js_rsp, strResult, nErrorCode);
    } else {
        xinfo("get_block action:%s not exist!", action.c_str());
        strResult = "Method not Found!";
        return false;
    }

    return true;
}

void xrpc_eth_query_manager::eth_getBalance(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    std::string account = js_req[0].asString();
    transform(account.begin(), account.end(), account.begin(), ::tolower);
    account = std::string(base::ADDRESS_PREFIX_EVM_TYPE_IN_MAIN_CHAIN) + account.substr(2);
    xdbg("xarc_query_manager::getBalance account: %s", account.c_str());

    // add top address check
    ADDRESS_CHECK_VALID(account)
    try {
        xaccount_ptr_t account_ptr = m_store->query_account(account);
        if (account_ptr == nullptr) {
            js_rsp["result"] = "0x0";
        } else {
            evm_common::u256 balance = account_ptr->tep_token_balance(data::XPROPERTY_TEP1_BALANCE_KEY, data::XPROPERTY_ASSET_ETH);

            std::string balance_str = toHex((top::evm_common::h256)balance);

            uint32_t i = 0;
            for (; i < balance_str.size() - 1; i++) {
                if (balance_str[i] != '0') {
                    break;
                }
            }
            js_rsp["result"] = "0x" + balance_str.substr(i);
            xdbg("xarc_query_manager::getBalance account: %s, balance:%s", account.c_str(), balance_str.substr(i).c_str());
        }
    } catch (exception & e) {
        strResult = std::string(e.what());
        nErrorCode = (uint32_t)enum_xrpc_error_code::rpc_param_unkown_error;
    }
}
void xrpc_eth_query_manager::eth_getTransactionCount(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    std::string account = js_req[0].asString();
    std::transform(account.begin(), account.end(), account.begin(),::tolower);
    account = std::string(base::ADDRESS_PREFIX_EVM_TYPE_IN_MAIN_CHAIN) + account.substr(2);

    // add top address check
    ADDRESS_CHECK_VALID(account)
    try {
        xaccount_ptr_t account_ptr = m_store->query_account(account);
        if (account_ptr == nullptr) {
            js_rsp["result"] = "0x0";
        } else {
            uint64_t nonce = account_ptr->get_latest_send_trans_number();
            xdbg("xarc_query_manager::eth_getTransactionCount: %s, %llu", account.c_str(), nonce);
            std::stringstream outstr;
            outstr << "0x" << std::hex << nonce;
            js_rsp["result"] = std::string(outstr.str());
        }
    } catch (exception & e) {
        strResult = std::string(e.what());
        nErrorCode = (uint32_t)enum_xrpc_error_code::rpc_param_unkown_error;
    }
}
void xrpc_eth_query_manager::eth_getTransactionByHash(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    uint256_t hash = top::data::hex_to_uint256(js_req[0].asString());
    std::string tx_hash = js_req[0].asString();
    std::string tx_hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    xdbg("eth_getTransactionByHash tx hash: %s",  tx_hash.c_str());

    xtxindex_detail_ptr_t sendindex = xrpc_loader_t::load_tx_indx_detail(tx_hash_str, base::enum_transaction_subtype_send);
    if (sendindex == nullptr) {
        xwarn("xrpc_eth_query_manager::eth_getTransactionByHash fail.tx hash:%s", tx_hash.c_str());
        js_rsp["result"] = xJson::Value::null;
        return;
    }

    xJson::Value js_result;
    std::stringstream outstr;
    outstr << "0x" << std::hex << sendindex->get_txindex()->get_block_height();

    js_result["blockHash"] = std::string("0x") + top::HexEncode(sendindex->get_txindex()->get_block_hash());
    js_result["blockNumber"] = outstr.str();
    js_result["from"] = std::string("0x") + sendindex->get_raw_tx()->get_source_addr().substr(6);
    js_result["gas"] = "0x40276";
    js_result["gasPrice"] = "0xb2d05e00";
    js_result["hash"] = tx_hash;
    js_result["input"] = "0x" + data::to_hex_str(sendindex->get_raw_tx()->get_data());
    uint64_t nonce = sendindex->get_raw_tx()->get_last_nonce();
    std::stringstream outstr_nonce;
    outstr_nonce << "0x" << std::hex << nonce;
    js_result["nonce"] = outstr_nonce.str();

    uint16_t tx_type = sendindex->get_raw_tx()->get_tx_type();
    js_result["from"] = std::string("0x") + sendindex->get_raw_tx()->get_source_addr().substr(6);
    if (tx_type == xtransaction_type_transfer) {
        js_result["to"] = std::string("0x") + sendindex->get_raw_tx()->get_target_addr().substr(6);
    } else {
        js_result["to"] = xJson::Value::null;
    }
    js_result["transactionIndex"] = "0x0";
    std::stringstream outstr_type;
    outstr_type << "0x" << std::hex << sendindex->get_raw_tx()->get_eip_version();
    js_result["type"] = std::string(outstr_type.str());
    js_result["value"] = "0x0";

    std::string str_v = sendindex->get_raw_tx()->get_SignV();
    uint32_t i = 0;
    for (; i < str_v.size() - 1; i++) {
        if (str_v[i] != '0') {
            break;
        }
    }
    js_result["v"] = std::string("0x") + str_v.substr(i);
    js_result["r"] = std::string("0x") + sendindex->get_raw_tx()->get_SignR();
    js_result["s"] = std::string("0x") + sendindex->get_raw_tx()->get_SignS();

    js_rsp["result"] = js_result;
    xdbg("xrpc_eth_query_manager::eth_getTransactionByHash ok.tx hash:%s", tx_hash.c_str());
    return;
}

top::evm_common::h2048 xrpc_eth_query_manager::calculate_bloom(const std::string & hexstr) {
    top::evm_common::h2048 bloom;
    char szDigest[32] = {0};
    std::string str = HexDecode(hexstr);
    keccak_256((const unsigned char *)str.data(), str.size(), (unsigned char *)szDigest);
    top::evm_common::h256 hash_h256;
    top::evm_common::bytesConstRef((const unsigned char *)szDigest, 32).copyTo(hash_h256.ref());
    bloom.shiftBloom<3>(hash_h256);
    return bloom;
}

void xrpc_eth_query_manager::eth_getTransactionReceipt(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    uint256_t hash = top::data::hex_to_uint256(js_req[0].asString());
    std::string tx_hash = js_req[0].asString();
    std::string tx_hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    xdbg("eth_getTransactionReceipt tx hash: %s",  tx_hash.c_str());

    xtxindex_detail_ptr_t sendindex = xrpc_loader_t::load_tx_indx_detail(tx_hash_str, base::enum_transaction_subtype_send);
    if (sendindex == nullptr) {
        xwarn("xrpc_query_manager::eth_getTransactionReceipt load tx index fail.%s", tx_hash.c_str());
        nErrorCode = (uint32_t)enum_xrpc_error_code::rpc_shard_exec_error;
        js_rsp["result"] = xJson::Value::null;
        return;
    }

    xJson::Value js_result;
    js_result["transactionHash"] = tx_hash;
    std::string block_hash = std::string("0x") + top::HexEncode(sendindex->get_txindex()->get_block_hash());
    js_result["blockHash"] = block_hash;
    std::string tx_idx = "0x0";
    js_result["transactionIndex"] = tx_idx;
    std::stringstream outstr;
    outstr << "0x" << std::hex << sendindex->get_txindex()->get_block_height();
    std::string block_num = outstr.str();
    js_result["blockNumber"] = block_num;
    js_result["cumulativeGasUsed"] = "0x404b2";
    js_result["effectiveGasPrice"] = "0x77359400";
    js_result["gasUsed"] = "0x404b2";

    uint16_t tx_type = sendindex->get_raw_tx()->get_tx_type();
    js_result["from"] = std::string("0x") + sendindex->get_raw_tx()->get_source_addr().substr(6);
    if (tx_type == xtransaction_type_transfer) {
        js_result["to"] = std::string("0x") + sendindex->get_raw_tx()->get_target_addr().substr(6);
        js_result["status"] = "0x1";
    } else {
        evm_common::xevm_transaction_result_t evm_result;
        auto ret = sendindex->get_txaction().get_evm_transaction_result(evm_result);

        if (tx_type == xtransaction_type_run_contract) {
            std::string contract_addr  = std::string("0x") + sendindex->get_raw_tx()->get_target_addr().substr(6);
            js_result["to"] = contract_addr;
            js_result["contractAddress"] = contract_addr;
        } else {
            js_result["to"] = xJson::Value::null;
            std::string contract_addr  = evm_result.extra_msg;
            js_result["contractAddress"] = contract_addr;
        }

        evm_common::h2048 logs_bloom;
        uint32_t index = 0;
        for (auto & log : evm_result.logs) {
            xJson::Value js_log;

            std::stringstream outstr1;
            outstr1 << "0x" << std::hex << index;
            js_log["logIndex"] = outstr1.str();
            js_log["blockNumber"] = block_num;
            js_log["blockHash"] = block_hash;
            js_log["transactionHash"] = tx_hash;
            js_log["transactionIndex"] = tx_idx;
            js_log["address"] = log.address;

            evm_common::h2048 bloom = calculate_bloom(log.address);
            logs_bloom |= bloom;

            for (auto & topic : log.topics) {
                js_log["topics"].append(topic);
                evm_common::h2048 topic_bloom = calculate_bloom(topic);
                logs_bloom |= topic_bloom;
            }

            js_log["data"] = log.data;
            js_log["removed"] = false;
            js_result["logs"].append(js_log);
            index++;
        }
        if (evm_result.logs.empty()) {
            js_result["logs"].resize(0);
        }

        std::stringstream outstrbloom;
        outstrbloom << logs_bloom;
        js_result["logsBloom"] = std::string("0x") + outstrbloom.str();
        // js_result["logsBloom"] = "0x00000000000000000000000000000000000000000400000000000000000000000000000000000000000080000000000000000000000000000000000000000000000000000000000000000000800000000000000000000000000000000000004000000000020000000000000000000800000000000000000000000000000000000000010000200000000400000000000000000000000000000000000000000000000000010000000000000000040000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000000000";
        js_result["status"] = (evm_result.status == 0) ?  "0x1" : "0x0";
        std::stringstream outstr;
        outstr << "0x" << std::hex << sendindex->get_raw_tx()->get_eip_version();
        js_result["type"] = std::string(outstr.str());
    }

    js_rsp["result"] = js_result;

    xdbg("xrpc_query_manager::eth_getTransactionReceipt ok.tx hash:%s", js_req[0].asString().c_str());
    return;
}
void xrpc_eth_query_manager::eth_blockNumber(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    std::string addr = std::string(sys_contract_eth_table_block_addr) + "@0";
    base::xvaccount_t _vaddress(addr);
    uint64_t height = m_block_store->get_latest_committed_block_height(_vaddress);

    std::stringstream outstr;
    outstr << "0x" << std::hex << height;
    js_rsp["result"] = std::string(outstr.str());
    xdbg("xarc_query_manager::eth_blockNumber: %llu", height);
}

void xrpc_eth_query_manager::eth_getBlockByHash(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    uint256_t hash = top::data::hex_to_uint256(js_req[0].asString());
    std::string tx_hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    xdbg("eth_getBlockByHash tx hash: %s",  top::HexEncode(tx_hash_str).c_str());

    base::xauto_ptr<base::xvblock_t>  block = m_block_store->get_block_by_hash(tx_hash_str);
    if (block == nullptr)
        return;

    xJson::Value js_result;
    set_block_result(block, js_result);
    js_rsp["result"] = js_result;
    return;
}
void xrpc_eth_query_manager::eth_getBlockByNumber(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    uint64_t height = std::strtoul(js_req[0].asString().c_str(), NULL, 16);
    xdbg("eth_getBlockByNumber: %s, %llu",  sys_contract_eth_table_block_addr, height);
    base::xvaccount_t account(std::string(sys_contract_eth_table_block_addr) + "@0");
    base::xauto_ptr<base::xvblock_t>  block = m_block_store->load_block_object(account, height, base::enum_xvblock_flag_committed, false);
    if (block == nullptr)
        return;

    xJson::Value js_result;
    set_block_result(block, js_result);
    js_result["baseFeePerGas"] = "0x10";
    js_rsp["result"] = js_result;
    return;
}
void xrpc_eth_query_manager::set_block_result(const base::xauto_ptr<base::xvblock_t>&  block, xJson::Value& js_result) {
    js_result["difficulty"] = "0x0";
    js_result["extraData"] = "0x0";
    js_result["gasLimit"] = "0x0";
    js_result["gasUsed"] = "0x0";
    std::string hash = block->get_block_hash();
    js_result["hash"] = std::string("0x") + top::HexEncode(hash); //js_req["tx_hash"].asString();
    js_result["logsBloom"] = "0x0";
    js_result["miner"] = std::string("0x") + std::string(40, '0');
    js_result["mixHash"] = "0x0";
    js_result["nonce"] = "0x0";
    std::stringstream outstr;
    outstr << "0x" << std::hex << block->get_height();
    js_result["number"] = std::string(outstr.str());
    js_result["parentHash"] = std::string("0x") + top::HexEncode(block->get_last_block_hash());
    js_result["receiptsRoot"] = "0x0";
    js_result["sha3Uncles"] = "0x0";
    js_result["size"] = "0x0";
    js_result["stateRoot"] = "0x0";
    outstr.seekp(0);
    outstr << "0x" << std::hex << block->get_timestamp();
    js_result["timestamp"] = std::string(outstr.str());
    js_result["totalDifficulty"] = "0x0";
    js_result["transactionsRoot"] = "0x0";

    const std::vector<base::xvaction_t> input_actions = block->get_tx_actions();
    for(auto action : input_actions) {
        if (action.get_org_tx_hash().empty()) {  // not txaction
            continue;
        }
        js_result["transactions"].append(std::string("0x") + to_hex_str(action.get_org_tx_hash()));
    }    
}
void xrpc_eth_query_manager::eth_getCode(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    std::string account = js_req[0].asString();
    std::transform(account.begin(), account.end(), account.begin(), ::tolower);
    account = std::string(base::ADDRESS_PREFIX_EVM_TYPE_IN_MAIN_CHAIN) + account.substr(2);

    // add top address check
    ADDRESS_CHECK_VALID(account)
    try {
        xaccount_ptr_t account_ptr = m_store->query_account(account);
        if (account_ptr == nullptr) {
            js_rsp["result"] = "0x";
            xdbg("xarc_query_manager::eth_getCode account: %s", account.c_str());
        } else {
            std::string code = account_ptr->get_code();
            code = std::string("0x") + top::HexEncode(code);
            xdbg("xarc_query_manager::eth_getCode account: %s, %s", account.c_str(), code.c_str());
            js_rsp["result"] = code;
        }
    } catch (exception & e) {
        strResult = std::string(e.what());
        js_rsp["result"] = xJson::Value::null;
        nErrorCode = (uint32_t)enum_xrpc_error_code::rpc_param_unkown_error;
    }
}
std::string xrpc_eth_query_manager::safe_get_json_value(xJson::Value & js_req, const std::string& key) {
    if (js_req.isMember(key))
        return js_req[key].asString();
    return "";
}
void xrpc_eth_query_manager::eth_call(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    std::string to = safe_get_json_value(js_req[0], "to");
    if (to.empty()) {
        xinfo("generate_tx to: %s", to.c_str());
        return;
    }
    std::transform(to.begin(), to.end(), to.begin(), ::tolower);
    to = std::string(base::ADDRESS_PREFIX_EVM_TYPE_IN_MAIN_CHAIN) + to.substr(2);

    std::string from = safe_get_json_value(js_req[0], "from");
    if (from.empty()) {
        xinfo("generate_tx from: %s", from.c_str());
        return;
    }
    std::transform(from.begin(), from.end(), from.begin(), ::tolower);
    from = std::string(base::ADDRESS_PREFIX_EVM_TYPE_IN_MAIN_CHAIN) + from.substr(2);

    std::string data = safe_get_json_value(js_req[0], "data");
    std::string value = safe_get_json_value(js_req[0], "value");
    std::string gas = safe_get_json_value(js_req[0], "gas");
    top::data::xtransaction_ptr_t tx = top::data::xtx_factory::create_ethcall_v3_tx(from, to, top::HexDecode(data.substr(2)), std::strtoul(value.c_str(), NULL, 16), std::strtoul(gas.c_str(), NULL, 16));
    auto cons_tx = top::make_object_ptr<top::data::xcons_transaction_t>(tx.get());

    std::string addr = std::string(sys_contract_eth_table_block_addr) + "@0";
    base::xvaccount_t _vaddress(addr);
    auto block = m_block_store->get_latest_committed_block(_vaddress);   
    uint64_t gmtime = block->get_second_level_gmtime();
    xblock_consensus_para_t cs_para(addr, block->get_clock(), block->get_viewid(), block->get_viewtoken(), block->get_height(), gmtime);

    statectx::xstatectx_ptr_t statectx_ptr = statectx::xstatectx_factory_t::create_latest_commit_statectx(_vaddress);
    if (statectx_ptr == nullptr) {
        xinfo("create_latest_commit_statectx fail: %s", addr.c_str());
        return;
    }
    txexecutor::xvm_para_t vmpara(cs_para.get_clock(), cs_para.get_random_seed(), cs_para.get_total_lock_tgas_token());
    vmpara.set_evm_gas_limit(std::strtoul(gas.c_str(), NULL, 16));

    txexecutor::xvm_input_t input{statectx_ptr, vmpara, cons_tx};
    txexecutor::xvm_output_t output;
    top::evm::xtop_evm evm{nullptr, statectx_ptr};

    auto ret = evm.execute(input, output);
    if (ret != txexecutor::enum_exec_success) {
        xinfo("evm call fail.");
        return;
    }
    xinfo("evm call: %d, %s", output.m_tx_result.status, output.m_tx_result.extra_msg.c_str());
    if (output.m_tx_result.status == 0)
        js_rsp["result"] = output.m_tx_result.extra_msg;
}
}  // namespace chain_info
}  // namespace top
