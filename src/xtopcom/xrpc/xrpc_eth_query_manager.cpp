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
#include "xbasic/xhex.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xcommon/xip.h"
#include "xcommon/xerror/xerror.h"
#include "xconfig/xconfig_register.h"
// #include "xdata/xcodec/xmsgpack/xelection/xelection_network_result_codec.hpp"
// #include "xdata/xcodec/xmsgpack/xelection/xelection_result_store_codec.hpp"
// #include "xdata/xcodec/xmsgpack/xelection/xstandby_result_store_codec.hpp"
// #include "xdata/xcodec/xmsgpack/xelection_association_result_store_codec.hpp"
// #include "xdata/xelection/xelection_association_result_store.h"
// #include "xdata/xelection/xelection_cluster_result.h"
// #include "xdata/xelection/xelection_cluster_result.h"
// #include "xdata/xelection/xelection_group_result.h"
// #include "xdata/xelection/xelection_info_bundle.h"
// #include "xdata/xelection/xelection_network_result.h"
// #include "xdata/xelection/xelection_result.h"
// #include "xdata/xelection/xelection_result_property.h"
// #include "xdata/xelection/xelection_result_store.h"
// #include "xdata/xelection/xstandby_result_store.h"
// #include "xdata/xfull_tableblock.h"
#include "xdata/xtx_factory.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xproposal_data.h"
//#include "xdata/xslash.h"
#include "xdata/xtable_bstate.h"
#include "xdata/xtableblock.h"
// #include "xdata/xtransaction_cache.h"
#include "xevm_common/fixed_hash.h"
#include "xevm_common/common_data.h"
#include "xevm_common/common.h"
#include "xevm_common/rlp.h"
#include "xevm_common/address.h"
#include "xrouter/xrouter.h"
#include "xrpc/xuint_format.h"
#include "xrpc/xrpc_loader.h"
#include "xrpc/xrpc_eth_parser.h"
// #include "xstore/xaccount_context.h"
// #include "xstore/xtgas_singleton.h"
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
#include "xdata/xrelay_block.h"

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

enum_query_result xrpc_eth_query_manager::query_account_by_number(const std::string &unit_address, const std::string& table_height, xaccount_ptr_t& ptr) {
    base::xvaccount_t _vaddr(unit_address);
    std::string table_address = base::xvaccount_t::make_table_account_address(_vaddr);
    base::xvaccount_t _table_addr(table_address);

    XMETRICS_GAUGE(metrics::blockstore_access_from_store, 1);
    xobject_ptr_t<base::xvblock_t> _block;
    if (table_height == "latest")
        _block = base::xvchain_t::instance().get_xblockstore()->get_latest_cert_block(_table_addr);
    else if (table_height == "earliest")
        _block = base::xvchain_t::instance().get_xblockstore()->get_genesis_block(_table_addr);
    else if (table_height == "pending")
        _block = base::xvchain_t::instance().get_xblockstore()->get_latest_cert_block(_table_addr);
    else {
        uint64_t height = std::strtoul(table_height.c_str(), NULL, 16);
        _block = base::xvchain_t::instance().get_xblockstore()->load_block_object(_table_addr, height, base::enum_xvblock_flag_authenticated, false, metrics::blockstore_access_from_rpc_get_block_query_propery);
    }

    if (_block == nullptr) {
        xwarn("xstore::query_account_by_number fail-load. account=%s", unit_address.c_str());
        return enum_block_not_found;
    }

    base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(_block.get(), metrics::statestore_access_from_vnodesrv_load_state);
    if (bstate == nullptr) {
        xwarn("xstore::query_account_by_number fail-load state.block=%s", _block->dump().c_str());
        return enum_block_not_found;
    }

    data::xtablestate_ptr_t tablestate = std::make_shared<data::xtable_bstate_t>(bstate.get());
    base::xaccount_index_t account_index;
    tablestate->get_account_index(unit_address, account_index);
    uint64_t unit_height = account_index.get_latest_unit_height();
    uint64_t unit_view_id = account_index.get_latest_unit_viewid();

    auto unit_block = base::xvchain_t::instance().get_xblockstore()->load_block_object(
        _vaddr, unit_height, unit_view_id, false, metrics::blockstore_access_from_rpc_get_block_query_propery);
    if (unit_block == nullptr) {
        xwarn("xstore::query_account_by_number fail-load. account=%s", unit_address.c_str());
        return enum_unit_not_found;
    }
    base::xauto_ptr<base::xvbstate_t> unit_state =
        base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(unit_block.get(), metrics::statestore_access_from_vnodesrv_load_state);
    if (unit_state == nullptr) {
        xwarn("xstore::query_account_by_number fail-load state.block=%s", unit_block->dump().c_str());
        return enum_unit_not_found;
    }
    ptr = std::make_shared<xunit_bstate_t>(unit_state.get());
    return enum_success;
}
xobject_ptr_t<base::xvblock_t> xrpc_eth_query_manager::query_block_by_height(const std::string& table_height) {
    xdbg("xrpc_eth_query_manager::query_block_by_height: %s, %s",  sys_contract_eth_table_block_addr, table_height.c_str());
    base::xvaccount_t _table_addr(std::string(sys_contract_eth_table_block_addr) + "@0");

    xobject_ptr_t<base::xvblock_t> _block;
    if (table_height == "latest")
        _block = base::xvchain_t::instance().get_xblockstore()->get_latest_cert_block(_table_addr);
    else if (table_height == "earliest")
        _block = base::xvchain_t::instance().get_xblockstore()->get_genesis_block(_table_addr);
    else if (table_height == "pending")
        _block = base::xvchain_t::instance().get_xblockstore()->get_latest_cert_block(_table_addr);
    else {
        uint64_t height = std::strtoul(table_height.c_str(), NULL, 16);
        _block = m_block_store->load_block_object(_table_addr, height, base::enum_xvblock_flag_authenticated, false);
    }
    return _block;
}
uint64_t xrpc_eth_query_manager::get_block_height(const std::string& table_height) {
    uint64_t height = 0;
    std::string addr = std::string(sys_contract_eth_table_block_addr) + "@0";
    base::xvaccount_t _vaddress(addr);
    uint64_t max_height = m_block_store->get_latest_cert_block_height(_vaddress);

    if (table_height == "latest")
        height = max_height;
    else if (table_height == "earliest")
        height = 0;
    else if (table_height == "pending") {
        height = max_height;
    } else {
        height = std::strtoul(table_height.c_str(), NULL, 16);
        if (height > max_height)
            height = max_height;
    }
    return height;
}
void xrpc_eth_query_manager::eth_getBalance(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    if (!eth::EthErrorCode::check_req(js_req, js_rsp, 2))
        return;
    if (!eth::EthErrorCode::check_hex(js_req[0].asString(), js_rsp, 0, eth::enum_rpc_type_address))
        return;
    if (!eth::EthErrorCode::check_eth_address(js_req[0].asString(), js_rsp))
        return;
    if (!eth::EthErrorCode::check_hex(js_req[1].asString(), js_rsp, 1, eth::enum_rpc_type_block))
        return;

    std::string account = js_req[0].asString();
    account = xvaccount_t::to_evm_address(account);
    xdbg("xrpc_eth_query_manager::eth_getBalance account: %s,%s", account.c_str(), js_req[1].asString().c_str());

    ETH_ADDRESS_CHECK_VALID(account)

    xaccount_ptr_t account_ptr;
    enum_query_result ret = query_account_by_number(account, js_req[1].asString(), account_ptr);

    if (ret == enum_block_not_found) {
        std::string msg = "header not found";
        eth::EthErrorCode::deal_error(js_rsp, eth::enum_eth_rpc_execution_reverted, msg);
        return;
    } else if (ret == enum_unit_not_found) {
        js_rsp["result"] = "0x0";
    } else if (ret == enum_success) {
        evm_common::u256 balance = account_ptr->tep_token_balance(common::xtoken_id_t::eth);
        js_rsp["result"] = xrpc_eth_parser_t::u256_to_hex_prefixed(balance);
        xdbg("xrpc_eth_query_manager::eth_getBalance address=%s,balance=%s,%s", account.c_str(), balance.str().c_str(), xrpc_eth_parser_t::u256_to_hex_prefixed(balance).c_str());
    }
}
void xrpc_eth_query_manager::eth_getTransactionCount(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    if (!eth::EthErrorCode::check_req(js_req, js_rsp, 2))
        return;
    if (!eth::EthErrorCode::check_hex(js_req[0].asString(), js_rsp, 0, eth::enum_rpc_type_address))
        return;
    if (!eth::EthErrorCode::check_eth_address(js_req[0].asString(), js_rsp))
        return;
    if (!eth::EthErrorCode::check_hex(js_req[1].asString(), js_rsp, 1, eth::enum_rpc_type_block))
        return;

    std::string account = js_req[0].asString();
    account = xvaccount_t::to_evm_address(account);

    // add top address check
    ETH_ADDRESS_CHECK_VALID(account)
    xaccount_ptr_t account_ptr;
    enum_query_result ret = query_account_by_number(account, js_req[1].asString(), account_ptr);
    if (ret == enum_block_not_found) {
        std::string msg = "header not found";
        eth::EthErrorCode::deal_error(js_rsp, eth::enum_eth_rpc_execution_reverted, msg);
        return;
    } else if (ret == enum_unit_not_found) {
        js_rsp["result"] = "0x0";
        return;
    } else if (ret == enum_success) {
        uint64_t nonce = account_ptr->get_latest_send_trans_number();
        xdbg("xarc_query_manager::eth_getTransactionCount: %s, %llu", account.c_str(), nonce);
        std::stringstream outstr;
        outstr << "0x" << std::hex << nonce;
        js_rsp["result"] = std::string(outstr.str());
    }
}
void xrpc_eth_query_manager::eth_getTransactionByHash(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    if (!eth::EthErrorCode::check_req(js_req, js_rsp, 1))
        return;
    std::string tx_hash = js_req[0].asString();
    if (!eth::EthErrorCode::check_hex(tx_hash, js_rsp, 0, eth::enum_rpc_type_hash))
        return;
    if (!eth::EthErrorCode::check_hash(tx_hash, js_rsp))
        return;

    uint256_t hash = top::data::hex_to_uint256(js_req[0].asString());
    std::string tx_hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    xdbg("eth_getTransactionByHash tx hash: %s",  tx_hash.c_str());

    xtxindex_detail_ptr_t sendindex = xrpc_loader_t::load_ethtx_indx_detail(tx_hash_str);
    if (sendindex == nullptr) {
        xwarn("xrpc_eth_query_manager::eth_getTransactionByHash fail.tx hash:%s", tx_hash.c_str());
        js_rsp["result"] = xJson::Value::null;
        return;
    }

    std::string block_hash = top::to_hex_prefixed(sendindex->get_txindex()->get_block_hash());
    std::string block_num = xrpc_eth_parser_t::uint64_to_hex_prefixed(sendindex->get_txindex()->get_block_height());
    std::string tx_index = xrpc_eth_parser_t::uint64_to_hex_prefixed(sendindex->get_transaction_index());
    xtx_location_t txlocation(block_hash, block_num, tx_hash, tx_index);

    xJson::Value js_result;
    std::error_code ec;
    xrpc_eth_parser_t::transaction_to_json(txlocation, sendindex->get_raw_tx(), js_result, ec);
    if (ec) {
        xerror("xrpc_eth_query_manager::eth_getTransactionByHash fail-transaction_to_json.tx hash:%s", tx_hash.c_str());
        js_rsp["result"] = xJson::Value::null;
        return;
    }
    js_rsp["result"] = js_result;
    xdbg("xrpc_eth_query_manager::eth_getTransactionByHash ok.tx hash:%s", tx_hash.c_str());
    return;
}

void xrpc_eth_query_manager::eth_getTransactionReceipt(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    if (!eth::EthErrorCode::check_req(js_req, js_rsp, 1))
        return;
    std::string tx_hash = js_req[0].asString();
    if (!eth::EthErrorCode::check_hex(tx_hash, js_rsp, 0, eth::enum_rpc_type_hash))
        return;
    if (!eth::EthErrorCode::check_hash(tx_hash, js_rsp))
        return;

    std::error_code ec;
    auto tx_hash_bytes = top::from_hex(tx_hash, ec);
    if (ec) {
        xdbg("eth_getTransactionReceipt from_hex fail: %s",  tx_hash.c_str());
        return;
    }
    std::string raw_tx_hash = top::to_string(tx_hash_bytes);
    xdbg("eth_getTransactionReceipt tx hash: %s",  tx_hash.c_str());

    xtxindex_detail_ptr_t sendindex = xrpc_loader_t::load_ethtx_indx_detail(raw_tx_hash);
    if (sendindex == nullptr) {
        xwarn("xrpc_query_manager::eth_getTransactionReceipt load tx index fail.%s", tx_hash.c_str());
        nErrorCode = (uint32_t)enum_xrpc_error_code::rpc_shard_exec_error;
        js_rsp["result"] = xJson::Value::null;
        return;
    }

    xJson::Value js_result;    
    xrpc_eth_parser_t::receipt_to_json(tx_hash, sendindex, js_result, ec);
    if (ec) {
        xerror("xrpc_query_manager::eth_getTransactionReceipt parse json fail.%s", tx_hash.c_str());
        nErrorCode = (uint32_t)enum_xrpc_error_code::rpc_shard_exec_error;
        js_rsp["result"] = xJson::Value::null;
        return;
    }
    js_rsp["result"] = js_result;
    xdbg("xrpc_query_manager::eth_getTransactionReceipt ok.tx hash:%s", js_req[0].asString().c_str());
    return;
}
void xrpc_eth_query_manager::eth_blockNumber(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    if (!eth::EthErrorCode::check_req(js_req, js_rsp, 0))
        return;
    std::string addr = std::string(sys_contract_eth_table_block_addr) + "@0";
    base::xvaccount_t _vaddress(addr);
    uint64_t height = m_block_store->get_latest_cert_block_height(_vaddress);

    std::stringstream outstr;
    outstr << "0x" << std::hex << height;
    js_rsp["result"] = std::string(outstr.str());
    xinfo("xarc_query_manager::eth_blockNumber: %llu", height);
}

void xrpc_eth_query_manager::eth_getBlockByHash(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    if (!eth::EthErrorCode::check_req(js_req, js_rsp, 2))
        return;
    std::string tx_hash = js_req[0].asString();
    if (!eth::EthErrorCode::check_hex(tx_hash, js_rsp, 0, eth::enum_rpc_type_hash))
        return;
    if (!eth::EthErrorCode::check_hash(tx_hash, js_rsp))
        return;
    if (!js_req[1].isBool()) {
        std::string msg = "parse error";
        eth::EthErrorCode::deal_error(js_rsp, eth::enum_eth_rpc_parse_error, msg);
        return;
    }

    uint256_t hash = top::data::hex_to_uint256(js_req[0].asString());
    std::string block_hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    xdbg("eth_getBlockByHash block hash: %s",  top::HexEncode(block_hash_str).c_str());

    base::xauto_ptr<base::xvblock_t>  block = m_block_store->get_block_by_hash(block_hash_str);
    if (block == nullptr) {
        js_rsp["result"] = xJson::Value::null;
        return;
    }

    std::error_code ec;
    xJson::Value js_result;
    set_block_result(block, js_result, js_req[1].asBool(), ec);
    js_rsp["result"] = js_result;
    return;
}
void xrpc_eth_query_manager::eth_getBlockByNumber(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    if (!eth::EthErrorCode::check_req(js_req, js_rsp, 2))
        return;
    if (!eth::EthErrorCode::check_hex(js_req[0].asString(), js_rsp, 0, eth::enum_rpc_type_block))
        return;
    if (!js_req[1].isBool()) {
        std::string msg = "parse error";
        eth::EthErrorCode::deal_error(js_rsp, eth::enum_eth_rpc_parse_error, msg);
        return;
    }

    xobject_ptr_t<base::xvblock_t>  block = query_block_by_height(js_req[0].asString());
    if (block == nullptr) {
        js_rsp["result"] = xJson::Value::null;
        return;
    }

    std::error_code ec;
    xJson::Value js_result;
    set_block_result(block, js_result, js_req[1].asBool(), ec);
    js_rsp["result"] = js_result;
    return;
}
void xrpc_eth_query_manager::set_block_result(const xobject_ptr_t<base::xvblock_t>&  block, xJson::Value& js_result, bool fullTx, std::error_code & ec) {
    // TODO(jimmy) block size need load input and output. transactions hash need load input
    if (block->get_block_class() != base::enum_xvblock_class_nil) {
        base::xvaccount_t _vaddress(block->get_account());
        if (false == base::xvchain_t::instance().get_xblockstore()->load_block_input(_vaddress, block.get())) {
            ec = common::error::xerrc_t::invalid_db_load;
            xerror("xrpc_eth_query_manager::set_block_result,fail to load block input for block:%s", block->dump().c_str());
            return;
        }
        if (false == base::xvchain_t::instance().get_xblockstore()->load_block_output(_vaddress, block.get())) {
            ec = common::error::xerrc_t::invalid_db_load;
            xerror("xrpc_eth_query_manager::set_block_result,fail to load block output for block:%s", block->dump().c_str());
            return;
        }
    }

    xrpc_eth_parser_t::blockheader_to_json(block.get(), js_result, ec);
    if (ec) {
        return;
    }

    std::string block_hash = top::to_hex_prefixed(block->get_block_hash());
    std::string block_num = xrpc_eth_parser_t::uint64_to_hex_prefixed(block->get_height());

    auto input_actions = data::xblockextract_t::unpack_eth_txactions(block.get());
    for (uint64_t txindex = 0; txindex < (uint64_t)input_actions.size(); txindex++) {
        auto & action = input_actions[txindex];
        std::string tx_hash = top::to_hex_prefixed(action.get_org_tx_hash());
        if (!fullTx) {
            js_result["transactions"].append(tx_hash);
        } else {
            data::xtransaction_ptr_t raw_tx = data::xblockextract_t::unpack_raw_tx(block.get(), action.get_org_tx_hash(), ec);
            if (raw_tx == nullptr) {
                xerror("xrpc_eth_query_manager::set_block_result fail.tx hash:%s", to_hex_str(action.get_org_tx_hash()).c_str());
                continue;
            }

            xtx_location_t txlocation(block_hash, block_num, tx_hash, xrpc_eth_parser_t::uint64_to_hex_prefixed(txindex));
            xJson::Value js_tx;
            xrpc_eth_parser_t::transaction_to_json(txlocation, raw_tx, js_tx, ec);
            if (ec) {
                xerror("xrpc_eth_query_manager::set_block_result fail-transaction_to_json.tx hash:%s", to_hex_str(action.get_org_tx_hash()).c_str());
                continue;
            }
            js_result["transactions"].append(js_tx);
        }
    }    
}
void xrpc_eth_query_manager::eth_getCode(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    if (!eth::EthErrorCode::check_req(js_req, js_rsp, 2))
        return;
    if (!eth::EthErrorCode::check_hex(js_req[0].asString(), js_rsp, 0, eth::enum_rpc_type_address))
        return;
    if (!eth::EthErrorCode::check_eth_address(js_req[0].asString(), js_rsp))
        return;
    if (!eth::EthErrorCode::check_hex(js_req[1].asString(), js_rsp, 1, eth::enum_rpc_type_block))
        return;

    std::string account = js_req[0].asString();
    account = xvaccount_t::to_evm_address(account);

    // add top address check
    ETH_ADDRESS_CHECK_VALID(account)

    xaccount_ptr_t account_ptr;
    enum_query_result ret = query_account_by_number(account, js_req[1].asString(), account_ptr);
    if (ret == enum_block_not_found) {
        std::string msg = "header not found";
        eth::EthErrorCode::deal_error(js_rsp, eth::enum_eth_rpc_execution_reverted, msg);
        return;
    } else if (ret == enum_unit_not_found) {
        js_rsp["result"] = "0x0";
        return;
    } else if (ret == enum_success) {
        std::string code = account_ptr->get_code();
        code = std::string("0x") + top::HexEncode(code);
        xdbg("xrpc_eth_query_manager::eth_getCode account: %s, %s", account.c_str(), code.c_str());
        js_rsp["result"] = code;
    }
    return;
}
std::string xrpc_eth_query_manager::safe_get_json_value(xJson::Value & js_req, const std::string& key) {
    if (js_req.isMember(key))
        return js_req[key].asString();
    return "";
}
void xrpc_eth_query_manager::eth_call(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    if (!eth::EthErrorCode::check_req(js_req, js_rsp, 2))
        return;
    if (!eth::EthErrorCode::check_hex(js_req[1].asString(), js_rsp, 1, eth::enum_rpc_type_block))
        return;

    std::string to = safe_get_json_value(js_req[0], "to");
    if (!eth::EthErrorCode::check_eth_address(to, js_rsp))
        return;
    to = xvaccount_t::to_evm_address(to);
    ETH_ADDRESS_CHECK_VALID(to)

    std::string from = safe_get_json_value(js_req[0], "from");
    if (from.empty())
        from = std::string("0x") + std::string(40, '0');
    if (!eth::EthErrorCode::check_eth_address(from, js_rsp))
        return;

    from = xvaccount_t::to_evm_address(from);
    ETH_ADDRESS_CHECK_VALID(from)

    std::string data;
    std::string jdata = safe_get_json_value(js_req[0], "data");
    if (jdata.empty()) {
        js_rsp["result"] = "0x";
        return;
    } else {
        if (!eth::EthErrorCode::check_hex(jdata, js_rsp, 0, eth::enum_rpc_type_data))
            return;
        data = top::HexDecode(jdata.substr(2));
    }

    std::string value = safe_get_json_value(js_req[0], "value");
    std::string gas = safe_get_json_value(js_req[0], "gas");
    std::string gas_price = safe_get_json_value(js_req[0], "gasPrice");
    if (!gas.empty()) {
        if (!eth::EthErrorCode::check_hex(gas, js_rsp, 0, eth::enum_rpc_type_unknown))
            return;
    }
    if (!gas_price.empty()) {
        if (!eth::EthErrorCode::check_hex(gas_price, js_rsp, 0, eth::enum_rpc_type_unknown))
            return;
    }

    evm_common::u256 gas_value = (evm_common::u256)XGET_ONCHAIN_GOVERNANCE_PARAMETER(block_gas_limit);
    top::data::xtransaction_ptr_t tx = top::data::xtx_factory::create_ethcall_v3_tx(from, to, data, std::strtoul(value.c_str(), NULL, 16), gas_value, 0);
    auto cons_tx = top::make_object_ptr<top::data::xcons_transaction_t>(tx.get());
    xinfo("xrpc_eth_query_manager::eth_call, %s, %s, %s", jdata.c_str(), value.c_str(), gas_value.str().c_str());

    std::string addr = std::string(sys_contract_eth_table_block_addr) + "@0";
    base::xvaccount_t _vaddress(addr);
    //auto block = m_block_store->get_latest_committed_block(_vaddress);   
    xobject_ptr_t<base::xvblock_t> block = query_block_by_height(js_req[1].asString());
    if (block == nullptr) {
        std::string msg = "header not found";
        eth::EthErrorCode::deal_error(js_rsp, eth::enum_eth_rpc_execution_reverted, msg);
        return;
    }
    uint64_t gmtime = block->get_second_level_gmtime();
    xblock_consensus_para_t cs_para(addr, block->get_clock(), block->get_viewid(), block->get_viewtoken(), block->get_height() + 1, gmtime);

    // TODO(jimmy) should create statectx by block
    statectx::xstatectx_ptr_t statectx_ptr = statectx::xstatectx_factory_t::create_latest_cert_statectx(_vaddress);
    if (statectx_ptr == nullptr) {
        xwarn("create_latest_cert_statectx fail: %s", addr.c_str());
        return;
    }

    base::xvaccount_t _vaddr(from);
    data::xunitstate_ptr_t unitstate = statectx_ptr->load_unit_state(_vaddr);
    if (nullptr == unitstate) {
        xwarn("eth_call fail-load unit state, %s", from.c_str());
        return;
    }
//        unitstate->tep_token_deposit(data::XPROPERTY_TEP1_BALANCE_KEY, data::XPROPERTY_ASSET_ETH, std::strtoul(value.c_str(), NULL, 16));
    if (!gas.empty() && !gas_price.empty()) {
        evm_common::u256 fund = unitstate->balance();
        uint64_t supplied_gas = std::strtoul(gas.c_str(), NULL, 16);
        evm_common::u256 gasfee = supplied_gas;
        gasfee *= std::strtoul(gas_price.c_str(), NULL, 16);
        xinfo("eth_call, gas: %s,%s,%llu, %s, %s", gas.c_str(), gas_price.c_str(), supplied_gas, fund.str().c_str(), gasfee.str().c_str());
        if (fund < gasfee) {
            std::string msg = std::string("err: insufficient funds for gas * price: address ") + safe_get_json_value(js_req[0], "from")
                + " have " + fund.str() + " want " + gasfee.str() + " (supplied gas " + std::to_string(supplied_gas) + ")";
            eth::EthErrorCode::deal_error(js_rsp, eth::enum_eth_rpc_execution_reverted, msg);
            return;
        }
    }

    uint64_t gas_limit = XGET_ONCHAIN_GOVERNANCE_PARAMETER(block_gas_limit);
    txexecutor::xvm_para_t vmpara(cs_para.get_clock(), cs_para.get_random_seed(), cs_para.get_total_lock_tgas_token(), gas_limit, cs_para.get_table_proposal_height(), eth_zero_address);
    txexecutor::xvm_input_t input{statectx_ptr, vmpara, cons_tx};
    txexecutor::xvm_output_t output;
    top::evm::xtop_evm evm{top::make_observer(contract_runtime::evm::xevm_contract_manager_t::instance()), statectx_ptr};

    auto ret = evm.execute(input, output);
    if (ret != txexecutor::enum_exec_success) {
        xwarn("evm call fail.");
        return;
    }
    xinfo("evm call: %d, %s", output.m_tx_result.status, output.m_tx_result.extra_msg.c_str());
    if (!gas.empty()) {
        gas_value = std::strtoul(gas.c_str(), NULL, 16);
        if (gas_value < output.m_tx_result.used_gas) {
            std::string msg = std::string("err: intrinsic gas too low: have ") + gas_value.str() + ", want " + std::to_string(output.m_tx_result.used_gas)
                + " (supplied gas " + gas_value.str() + ")";
            eth::EthErrorCode::deal_error(js_rsp, eth::enum_eth_rpc_execution_reverted, msg);
            return;
        }
    }
    if (output.m_tx_result.status == evm_common::OutOfFund) {
        std::string msg = "insufficient funds for transfer";
        eth::EthErrorCode::deal_error(js_rsp, eth::enum_eth_rpc_execution_reverted, msg);
        return;
    } else if (output.m_tx_result.status == evm_common::Success) {
        if (output.m_tx_result.extra_msg.empty())
            output.m_tx_result.extra_msg = "0x";
        js_rsp["result"] = output.m_tx_result.extra_msg;
    } else {
        js_rsp["error"]["code"] = eth::enum_eth_rpc_execution_reverted;
        js_rsp["error"]["message"] = "execution reverted";
    }
}

void xrpc_eth_query_manager::eth_estimateGas(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    std::string block_number = "latest";
    if (js_req.size() >= 2)
        block_number = js_req[1].asString();
    if (!eth::EthErrorCode::check_hex(block_number, js_rsp, 1, eth::enum_rpc_type_block))
        return;

    std::string to = safe_get_json_value(js_req[0], "to");
    if (to.empty()) {
        to = std::string("0x") + std::string(40, '0');
    } else {
        if (!eth::EthErrorCode::check_eth_address(to, js_rsp))
            return;
    }
    to = xvaccount_t::to_evm_address(to);
    xdbg("xrpc_eth_query_manager::eth_estimateGas, to: %s", to.c_str());
    ETH_ADDRESS_CHECK_VALID(to)

    std::string from = safe_get_json_value(js_req[0], "from");
    if (from.empty())
        from = std::string("0x") + std::string(40, '0');
    if (!eth::EthErrorCode::check_eth_address(from, js_rsp))
        return;

    from = xvaccount_t::to_evm_address(from);
    xdbg("xrpc_eth_query_manager::eth_estimateGas, from: %s", from.c_str());
    ETH_ADDRESS_CHECK_VALID(from)

    std::string data;
    std::string jdata = safe_get_json_value(js_req[0], "data");
    if (jdata.empty()) {
        js_rsp["result"] = "0x5208";
        return;
    } else {
        if (!eth::EthErrorCode::check_hex(jdata, js_rsp, 0, eth::enum_rpc_type_data))
            return;
        data = top::HexDecode(jdata.substr(2));
    }

    std::string value = safe_get_json_value(js_req[0], "value");
    std::string gas = safe_get_json_value(js_req[0], "gas");
    std::string gas_price = safe_get_json_value(js_req[0], "gasPrice");
    if (!gas.empty()) {
        if (!eth::EthErrorCode::check_hex(gas, js_rsp, 0, eth::enum_rpc_type_unknown))
            return;
    }
    if (!gas_price.empty()) {
        if (!eth::EthErrorCode::check_hex(gas_price, js_rsp, 0, eth::enum_rpc_type_unknown))
            return;
    }

    evm_common::u256 gas_value = (evm_common::u256)XGET_ONCHAIN_GOVERNANCE_PARAMETER(block_gas_limit);
    top::data::xtransaction_ptr_t tx = top::data::xtx_factory::create_ethcall_v3_tx(from, to, data, std::strtoul(value.c_str(), NULL, 16), gas_value, 0);
    auto cons_tx = top::make_object_ptr<top::data::xcons_transaction_t>(tx.get());
    xinfo("xrpc_eth_query_manager::eth_estimateGas, %s, %s, %s", jdata.c_str(), value.c_str(), gas_value.str().c_str());

    std::string addr = std::string(sys_contract_eth_table_block_addr) + "@0";
    base::xvaccount_t _vaddress(addr);
    xobject_ptr_t<base::xvblock_t> block = query_block_by_height(block_number);
    if (block == nullptr) {
        std::string msg = "block not found";
        eth::EthErrorCode::deal_error(js_rsp, eth::enum_eth_rpc_execution_reverted, msg);
        return;
    }
    uint64_t gmtime = block->get_second_level_gmtime();
    xblock_consensus_para_t cs_para(addr, block->get_clock(), block->get_viewid(), block->get_viewtoken(), block->get_height() + 1, gmtime);

    statectx::xstatectx_ptr_t statectx_ptr = statectx::xstatectx_factory_t::create_latest_cert_statectx(_vaddress);
    if (statectx_ptr == nullptr) {
        xwarn("create_latest_cert_statectx fail: %s", addr.c_str());
        return;
    }

    base::xvaccount_t _vaddr(from);
    data::xunitstate_ptr_t unitstate = statectx_ptr->load_unit_state(_vaddr);
    if (nullptr == unitstate) {
        xwarn("eth_estimateGas fail-load unit state, %s", from.c_str());
        return;
    }
//        unitstate->tep_token_deposit(data::XPROPERTY_TEP1_BALANCE_KEY, data::XPROPERTY_ASSET_ETH, std::strtoul(value.c_str(), NULL, 16));
    //evm_common::u256 allowance = unitstate->tep_token_balance(data::XPROPERTY_TEP1_BALANCE_KEY, data::XPROPERTY_ASSET_TOP);
    if (!gas.empty() && !gas_price.empty()) {
        evm_common::u256 fund = unitstate->balance();
        uint64_t supplied_gas = std::strtoul(gas.c_str(), NULL, 16);
        evm_common::u256 gasfee = supplied_gas;
        gasfee *= std::strtoul(gas_price.c_str(), NULL, 16);
        xinfo("eth_call, gas: %s,%s,%llu, %s, %s", gas.c_str(), gas_price.c_str(), supplied_gas, fund.str().c_str(), gasfee.str().c_str());
        if (fund < gasfee) {
            std::string msg = std::string("gas required exceeds allowance (") + fund.str() + ")";
            eth::EthErrorCode::deal_error(js_rsp, eth::enum_eth_rpc_execution_reverted, msg);
            return;
        }
    }

    uint64_t gas_limit = XGET_ONCHAIN_GOVERNANCE_PARAMETER(block_gas_limit);
    txexecutor::xvm_para_t vmpara(cs_para.get_clock(), cs_para.get_random_seed(), cs_para.get_total_lock_tgas_token(), gas_limit, cs_para.get_table_proposal_height(), eth_zero_address);

    txexecutor::xvm_input_t input{statectx_ptr, vmpara, cons_tx};
    txexecutor::xvm_output_t output;
    top::evm::xtop_evm evm{top::make_observer(contract_runtime::evm::xevm_contract_manager_t::instance()), statectx_ptr};

    auto ret = evm.execute(input, output);
    if (ret != txexecutor::enum_exec_success) {
        xwarn("evm call fail.");
        return;
    }
    xinfo("eth_estimateGas call: %d, %d, %s, %llu", ret, output.m_tx_result.status, output.m_tx_result.extra_msg.c_str(), output.m_tx_result.used_gas);

    switch (output.m_tx_result.status) {
    case evm_common::Success: {
        std::stringstream outstr;
        outstr << "0x" << std::hex << output.m_tx_result.used_gas + output.m_tx_result.used_gas / 2;
        js_rsp["result"] = outstr.str();
        break;
    }

    case evm_common::OutOfFund: {
        std::string msg = "insufficient funds for transfer";
        eth::EthErrorCode::deal_error(js_rsp, eth::enum_eth_rpc_execution_reverted, msg);
        break;
    }

    case evm_common::Revert:
        js_rsp["error"]["code"] = eth::enum_eth_rpc_execution_reverted;
        js_rsp["error"]["message"] = "execution reverted";
        break;

    case evm_common::OutOfGas:
        js_rsp["error"]["code"] = eth::enum_eth_rpc_execution_reverted;
        js_rsp["error"]["message"] = "execution out of gas";
        break;

    case evm_common::OutOfOffset:
        js_rsp["error"]["code"] = eth::enum_eth_rpc_execution_reverted;
        js_rsp["error"]["message"] = "execution out of offset";
        break;

    case evm_common::OtherExecuteError:
        js_rsp["error"]["code"] = eth::enum_eth_rpc_execution_reverted;
        js_rsp["error"]["message"] = "execution unknown error";
        break;

    default:
        assert(false);
        break;
    }
}
void xrpc_eth_query_manager::eth_getStorageAt(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    if (!eth::EthErrorCode::check_req(js_req, js_rsp, 3))
        return;
    std::string account = js_req[0].asString();
    if (!eth::EthErrorCode::check_hex(account, js_rsp, 0, eth::enum_rpc_type_address))
        return;
    if (!eth::EthErrorCode::check_eth_address(account, js_rsp))
        return;
    if (!eth::EthErrorCode::check_hex(js_req[1].asString(), js_rsp, 1, eth::enum_rpc_type_unknown))
        return;
    if (!eth::EthErrorCode::check_hex(js_req[2].asString(), js_rsp, 2, eth::enum_rpc_type_block))
        return;

    account = xvaccount_t::to_evm_address(account);
    xdbg("xarc_query_manager::eth_getStorageAt account: %s,%s", account.c_str(), js_req[1].asString().c_str());

    ETH_ADDRESS_CHECK_VALID(account)

    xunitstate_ptr_t account_ptr;
    enum_query_result ret = query_account_by_number(account, js_req[2].asString(), account_ptr);
    if (ret == enum_block_not_found) {
        std::string msg = "header not found";
        eth::EthErrorCode::deal_error(js_rsp, eth::enum_eth_rpc_execution_reverted, msg);
        return;
    } else if (ret == enum_unit_not_found) {
        js_rsp["result"] = "0x0";
        return;
    }
    std::string index_str = js_req[1].asString().substr(2);
    if (index_str.size() % 2 != 0)
        index_str = std::string("0") + index_str;
    index_str = top::HexDecode(index_str);
    std::string value_str = account_ptr->get_storage(index_str);
    xinfo("xrpc_eth_query_manager::eth_getStorageAt, %s,%s,%s, %s", js_req[0].asString().c_str(), js_req[1].asString().c_str(), js_req[2].asString().c_str(),
        top::HexEncode(value_str).c_str());
    js_rsp["result"] = std::string("0x") + top::HexEncode(value_str);
}
int xrpc_eth_query_manager::parse_topics(const xJson::Value& topics, std::vector<std::set<std::string>>& vTopics, xJson::Value & js_rsp) {
    for (int i = 0; i < (int)topics.size(); i++) {
        xJson::Value one_topic = topics[i];
        if (one_topic.isString()) {
            if (!eth::EthErrorCode::check_hex(one_topic.asString(), js_rsp, 0, eth::enum_rpc_type_topic))
                return 1;
            std::set<std::string> s;
            s.insert(one_topic.asString());
            vTopics.push_back(s);
            xdbg("eth_getLogs, topics: %s", one_topic.asString().c_str());
        } else if (one_topic.isArray()) {
            std::set<std::string> s;
            for (int j = 0; j < (int)one_topic.size(); j++) {
                if (one_topic[j].isString()) {
                    if (!eth::EthErrorCode::check_hex(one_topic[j].asString(), js_rsp, 0, eth::enum_rpc_type_topic))
                        return 1;
                    s.insert(one_topic[j].asString());
                    continue;
                } else if (one_topic[j].isNull()) {
                    continue;
                }
                std::string msg = "parse error";
                eth::EthErrorCode::deal_error(js_rsp, eth::enum_eth_rpc_parse_error, msg);
                return 1;
            }
            vTopics.push_back(s);
            xdbg("eth_getLogs, topics: %d", s.size());
        } else if (one_topic.isNull()) {
            std::set<std::string> s;
            vTopics.push_back(s);
            xdbg("eth_getLogs, topics: null");
        } else {
            std::string msg = "parse error";
            eth::EthErrorCode::deal_error(js_rsp, eth::enum_eth_rpc_parse_error, msg);
            return 1;
        }
    }
    return 0;
}
void xrpc_eth_query_manager::eth_getLogs(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    if (js_req.size() == 0) {
        std::string msg = std::string("missing value for required argument 0");
        eth::EthErrorCode::deal_error(js_rsp, eth::enum_eth_rpc_invalid_params, msg);
        return;
    }
    std::string from_block;
    if (js_req[0].isMember("fromBlock")) {
        from_block = js_req[0]["fromBlock"].asString();
        if (!eth::EthErrorCode::check_hex(from_block, js_rsp, 0, eth::enum_rpc_type_block))
            return;
    }

    std::string to_block;
    if (js_req[0].isMember("toBlock")) {
        to_block = js_req[0]["toBlock"].asString();
        if (!eth::EthErrorCode::check_hex(to_block, js_rsp, 0, eth::enum_rpc_type_block))
            return;
    }

    std::string blockhash;
    if (js_req[0].isMember("blockhash")) {
        blockhash = js_req[0]["blockhash"].asString();
        if (!eth::EthErrorCode::check_hex(blockhash, js_rsp, 0, eth::enum_rpc_type_hash))
            return;
        if (!eth::EthErrorCode::check_hash(blockhash, js_rsp))
            return;
    }

    std::vector<std::set<std::string>> vTopics;
    if (js_req[0].isMember("topics")) {
        xJson::Value t = js_req[0]["topics"];
        if (!t.isArray()) {
            std::string msg = "parse error";
            eth::EthErrorCode::deal_error(js_rsp, eth::enum_eth_rpc_parse_error, msg);
            return;
        }
        if (parse_topics(t, vTopics, js_rsp) != 0)
            return;
    }
    std::set<std::string> sAddress;
    if (js_req[0].isMember("address")) {
        xJson::Value t = js_req[0]["address"];
        if (t.isString()) {
            if (!eth::EthErrorCode::check_hex(t.asString(), js_rsp, 0, eth::enum_rpc_type_address))
                return;
            if (!eth::EthErrorCode::check_eth_address(t.asString(), js_rsp))
                return;

            sAddress.insert(t.asString());
            xdbg("eth_getLogs, address : %s", t.asString().c_str());
        } else if (t.isArray()) {
            for (int i = 0; i < (int)t.size(); i++) {
                if (!eth::EthErrorCode::check_hex(t[i].asString(), js_rsp, 0, eth::enum_rpc_type_address))
                    return;
                if (!eth::EthErrorCode::check_eth_address(t[i].asString(), js_rsp))
                    return;
                sAddress.insert(t[i].asString());
                xdbg("eth_getLogs, address: %s", t[i].asString().c_str());
            }
        }
    }

    if ((!from_block.empty() || !to_block.empty()) && !blockhash.empty()) {
        std::string msg = "invalid argument 0: cannot specify both BlockHash and FromBlock/ToBlock, choose one or the other";
        eth::EthErrorCode::deal_error(js_rsp, eth::enum_eth_rpc_invalid_params, msg);
        return;
    }
    uint64_t begin;
    uint64_t end;
    if (!blockhash.empty()) {
        uint256_t hash = top::data::hex_to_uint256(blockhash);
        std::string block_hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
        base::xauto_ptr<base::xvblock_t> block = m_block_store->get_block_by_hash(block_hash_str);
        if (block == nullptr) {
            std::string msg = "unknown block";
            eth::EthErrorCode::deal_error(js_rsp, eth::enum_eth_rpc_execution_reverted, msg);
            return;
        }
        begin = block->get_height();
        end = block->get_height();
    } else if (from_block.empty() && to_block.empty()){
        js_rsp["result"] = xJson::Value::null;
        return;
    } else {
        if (from_block.empty()) {
            std::string addr = std::string(sys_contract_eth_table_block_addr) + "@0";
            base::xvaccount_t _vaddress(addr);
            begin = m_block_store->get_latest_cert_block_height(_vaddress);
        } else
            begin = get_block_height(from_block);
        if (to_block.empty()) {
            std::string addr = std::string(sys_contract_eth_table_block_addr) + "@0";
            base::xvaccount_t _vaddress(addr);
            end = m_block_store->get_latest_cert_block_height(_vaddress);
        } else
            end = get_block_height(to_block);
        if (begin > end) {
            js_rsp["result"] = xJson::Value::null;
            return;
        }
        if (end - begin > 1024)
            begin = end - 1024;
    }
    xinfo("xrpc_eth_query_manager::eth_getLogs, %llu, %llu", begin, end);

    get_log(js_rsp, begin, end, vTopics, sAddress);
    return;
}

bool xrpc_eth_query_manager::check_log_is_match(evm_common::xevm_log_t const& log, const std::vector<std::set<std::string>>& vTopics, const std::set<std::string>& sAddress) const {
    std::string log_address_hex = log.address.to_hex_string();
    if (!sAddress.empty() && sAddress.find(log_address_hex) == sAddress.end()) {
        xdbg("address not match: %s", log_address_hex.c_str());
        return false;
    }
    if (vTopics.size() > log.topics.size()) {
        xdbg("topics size not match %zu,%zu", vTopics.size(), log.topics.size());
        return false;
    }

    uint32_t topic_index = 0;
    for (auto & vtopic : vTopics) {
        if (vtopic.empty()) {
            topic_index++;
            continue;
        }
        std::string log_topic_hex = top::to_hex_prefixed(log.topics[topic_index].asBytes());
        if (vtopic.find(log_topic_hex) == vtopic.end()) {  // Topics are order-dependent.
            xdbg("topic value not match %d", topic_index);
            return false;
        }
        topic_index++;
    }
    return true;
}

int xrpc_eth_query_manager::get_log(xJson::Value & js_rsp, const uint64_t begin, const uint64_t end, const std::vector<std::set<std::string>>& vTopics, const std::set<std::string>& sAddress) {
    base::xvaccount_t _table_addr(std::string(sys_contract_eth_table_block_addr) + "@0");
    for (uint64_t i = begin; i <= end; i++) {  // traverse blocks
        xobject_ptr_t<base::xvblock_t> block = m_block_store->load_block_object(_table_addr, i, base::enum_xvblock_flag_authenticated, false);
        if (block == nullptr) {
            xwarn("xrpc_eth_query_manager::get_log, load_block_object fail:%llu", i);
            continue;
        }

        auto input_actions = data::xblockextract_t::unpack_eth_txactions(block.get());
        xdbg("input_actions size:%d", input_actions.size());
        for (uint64_t txindex = 0; txindex < (uint64_t)input_actions.size(); txindex++) {
            auto & action = input_actions[txindex];

            data::xeth_store_receipt_t evm_tx_receipt;
            auto ret = action.get_evm_transaction_receipt(evm_tx_receipt);
            if (!ret) {
                xerror("xrpc_eth_query_manager::get_log, fail-get evm transaction receipt,i=%llu,txindex=%llu", i, txindex);
                continue;
            }
            if (evm_tx_receipt.get_logs().empty()) {
                continue;
            }

            std::string block_hash = top::to_hex_prefixed(block->get_block_hash());
            std::string block_num = xrpc_eth_parser_t::uint64_to_hex_prefixed(block->get_height());
            std::string tx_hash = top::to_hex_prefixed(action.get_org_tx_hash());
            std::string tx_index = xrpc_eth_parser_t::uint64_to_hex_prefixed(txindex);
            xlog_location_t loglocation(block_hash, block_num, tx_hash, tx_index);

            uint32_t index = 0;
            for (uint64_t logindex = 0; logindex < (uint64_t)evm_tx_receipt.get_logs().size(); logindex++) {
                auto & log = evm_tx_receipt.get_logs()[logindex];
                if (false == check_log_is_match(log, vTopics, sAddress)) {
                    continue;
                }
                loglocation.m_log_index = xrpc_eth_parser_t::uint64_to_hex_prefixed(logindex);

                xJson::Value js_log;
                xrpc_eth_parser_t::log_to_json(loglocation, log, js_log);
                js_rsp["result"].append(js_log);
                if (js_rsp["result"].size() >= 1024) {
                    xwarn("xrpc_eth_query_manager::get_log,too many logs: %d,begin=%ld,end=%ld", js_rsp["result"].size(),begin,end);
                    return 0;
                }
            }
        }
    }
    if (js_rsp["result"].empty())
        js_rsp["result"].resize(0);
    return 0;
}

xobject_ptr_t<base::xvblock_t> xrpc_eth_query_manager::query_relay_block_by_height(const std::string& table_height) {
    xdbg("xrpc_eth_query_manager::query_relay_block_by_height: %s, %s", sys_contract_relay_table_block_addr, table_height.c_str());
    base::xvaccount_t _table_addr(sys_contract_relay_table_block_addr);

    xobject_ptr_t<base::xvblock_t> _block;
    if (table_height == "latest")
        _block = base::xvchain_t::instance().get_xblockstore()->get_latest_cert_block(_table_addr);
    else if (table_height == "earliest")
        _block = base::xvchain_t::instance().get_xblockstore()->get_genesis_block(_table_addr);
    else if (table_height == "pending")
        _block = base::xvchain_t::instance().get_xblockstore()->get_latest_cert_block(_table_addr);
    else {
        uint64_t height = std::strtoul(table_height.c_str(), NULL, 16);
        _block = m_block_store->load_block_object(_table_addr, height * 3, base::enum_xvblock_flag_authenticated, false);
    }
    return _block;
}
void xrpc_eth_query_manager::top_getRelayBlockByNumber(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    if (!eth::EthErrorCode::check_req(js_req, js_rsp, 1))
        return;
    if (!eth::EthErrorCode::check_hex(js_req[0].asString(), js_rsp, 0, eth::enum_rpc_type_block))
        return;

    if (std::strtoul(js_req[0].asString().c_str(), NULL, 16) == 0) {
        uint64_t block_version = 0;
       // evm_common::u256 chain_bits = 0;
        uint64_t epochID = 0;
        evm_common::h256 prev_hash;
        data::xrelay_block relay_block(block_version, prev_hash,  0, epochID, 0);

        xbytes_t header_data = relay_block.get_header().streamRLP_header_to_contract();

        xJson::Value js_result;
        js_rsp["result"] = top::to_hex_prefixed(header_data);
        return;
    }

    xobject_ptr_t<base::xvblock_t>  block = query_relay_block_by_height(js_req[0].asString());
    if (block == nullptr) {
        js_rsp["result"] = xJson::Value::null;
        return;
    }

    std::string sign_list = block->get_cert()->get_extend_data();
    if (sign_list.empty()) {
        js_rsp["result"] = xJson::Value::null;
        return;
    }
    base::xstream_t stream{base::xcontext_t::instance(), (uint8_t *)sign_list.data(), static_cast<uint32_t>(sign_list.size())};
    uint16_t size = 0;
    stream >> size;

    data::xrelay_block relay_block;
    for (uint16_t i = 0; i < size; i++) {
        xvip2_t xip;
        std::string sign_str;
        stream >> xip.high_addr;
        stream >> xip.low_addr;
        stream >> sign_str;

        if (sign_str.size() != 65) {
            js_rsp["result"] = xJson::Value::null;
            xwarn("xrpc_eth_query_manager::top_getRelayBlockByNumber, sign size error: %d", sign_str.size());
            return;
        }

        xdbg("top_getRelayBlockByNumber, sign_str: %s", top::HexEncode(sign_str).c_str());
        uint8_t compact_signature[65];
        memcpy(compact_signature, sign_str.data(), sign_str.size());
        xrelay_signature signature;
        signature.v = compact_signature[0];
        top::evm_common::bytes r_bytes(compact_signature + 1, compact_signature + 33);
        signature.r = top::evm_common::fromBigEndian<top::evm_common::u256>(r_bytes);
        top::evm_common::bytes s_bytes(compact_signature + 33, compact_signature + 65);
        signature.s = top::evm_common::fromBigEndian<top::evm_common::u256>(s_bytes);
        relay_block.add_signature(signature);
    }

    std::string relay_block_data = block->get_header()->get_extra_data();
    xdbg("top_getRelayBlockByNumber, relay_block_data: %s", top::HexEncode(relay_block_data).c_str());
    std::error_code ec;
    relay_block.decodeBytes(to_bytes(relay_block_data), ec);
    if (ec) {
        xwarn("xrpc_eth_query_manager::top_getRelayBlockByNumber relay_block decodeBytes error %s; err msg %s", ec.category().name(), ec.message().c_str());
        return ;
    }

    //relay_block.streamRLP(rlp_stream, true);
    xbytes_t header_data = relay_block.get_header().streamRLP_header_to_contract();

    xJson::Value js_result;
    js_rsp["result"] = top::to_hex_prefixed(header_data);
    return;    
}
void xrpc_eth_query_manager::top_relayBlockNumber(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    if (!eth::EthErrorCode::check_req(js_req, js_rsp, 0))
        return;

    base::xvaccount_t _vaddress(sys_contract_relay_table_block_addr);
    uint64_t height = m_block_store->get_latest_cert_block_height(_vaddress);
    xinfo("xarc_query_manager::top_relayBlockNumber: %llu", height);
    if (height != 0)
        height = (height-1)/3 + 1;

    std::stringstream outstr;
    outstr << "0x" << std::hex << height;
    js_rsp["result"] = std::string(outstr.str());
}
}  // namespace chain_info
}  // namespace top
