#include "xvledger/xvtxindex.h"
#include "xrpc/xrpc_eth_parser.h"
#include "xbasic/xhex.h"
#include "xcommon/xerror/xerror.h"
#include "xcommon/xeth_address.h"
#include "xcommon/xaddress.h"
#include "xdata/xethheader.h"
#include "xdata/xblockextract.h"
#include "xdata/xethreceipt.h"

namespace top {

namespace xrpc {

std::string xrpc_eth_parser_t::uint64_to_hex_prefixed(uint64_t value) {
    std::stringstream outstr;
    outstr << "0x" << std::hex << value;
    return outstr.str();
}

void xrpc_eth_parser_t::txlocation_to_json(xtx_location_t const& txlocation, xJson::Value & js_v) {
    js_v["blockNumber"] = txlocation.m_block_number;
    js_v["blockHash"] = txlocation.m_block_hash;
    js_v["transactionHash"] = txlocation.m_tx_hash;
    js_v["transactionIndex"] = txlocation.m_transaction_index;
}

void xrpc_eth_parser_t::log_to_json(xlog_location_t const& loglocation, evm_common::xevm_log_t const& log, xJson::Value & js_v) {
    js_v["logIndex"] = loglocation.m_log_index;
    txlocation_to_json(loglocation, js_v);
    js_v["removed"] = false;
    js_v["address"] = log.address.to_hex_string();
    for (auto & topic : log.topics) {
        js_v["topics"].append(top::to_hex_prefixed(topic.asBytes()));
    }
    js_v["data"] = top::to_hex_prefixed(log.data);
}

void xrpc_eth_parser_t::receipt_to_json(const std::string & tx_hash, xtxindex_detail_ptr_t const& sendindex, xJson::Value & js_v, std::error_code & ec) {
    std::string block_hash = top::to_hex_prefixed(sendindex->get_txindex()->get_block_hash());
    std::string block_num = uint64_to_hex_prefixed(sendindex->get_txindex()->get_block_height());
    std::string tx_idx = uint64_to_hex_prefixed((uint64_t)sendindex->get_transaction_index());
    common::xeth_address_t _from_addr = common::xeth_address_t::build_from(common::xaccount_address_t(sendindex->get_raw_tx()->get_source_addr()));
    std::string _from_addr_str = _from_addr.to_hex_string();
    xtx_location_t txlocation(block_hash,block_num,tx_hash,tx_idx);

    data::xeth_store_receipt_t evm_tx_receipt;
    auto ret = sendindex->get_txaction().get_evm_transaction_receipt(evm_tx_receipt);
    if (!ret) {
        ec = common::error::xerrc_t::invalid_block;
        xerror("xrpc_eth_parser_t::receipt_to_json fail-get_evm_receipt");
        return;
    }

    txlocation_to_json(txlocation, js_v);

    js_v["status"] = (evm_tx_receipt.get_tx_status() == data::ethreceipt_status_successful) ?  "0x1" : "0x0";
    js_v["type"] = uint64_to_hex_prefixed((uint64_t)sendindex->get_raw_tx()->get_eip_version());
    js_v["gasUsed"] = uint64_to_hex_prefixed(evm_tx_receipt.get_gas_used());
    js_v["cumulativeGasUsed"] = uint64_to_hex_prefixed(evm_tx_receipt.get_cumulative_gas_used());  // TODO(jimmy)
    js_v["effectiveGasPrice"] = "0x77359400";// TODO(jimmy)

    uint16_t tx_type = sendindex->get_raw_tx()->get_tx_type();
    js_v["from"] = _from_addr_str;
    if (tx_type == data::xtransaction_type_transfer) {
        js_v["to"] = std::string("0x") + sendindex->get_raw_tx()->get_target_addr().substr(6);
    } else {
        if (tx_type == data::xtransaction_type_run_contract) {
            std::string contract_addr  = std::string("0x") + sendindex->get_raw_tx()->get_target_addr().substr(6);
            js_v["to"] = contract_addr;
            js_v["contractAddress"] = contract_addr;
        } else {
            js_v["to"] = xJson::Value::null;
            if (!evm_tx_receipt.get_contract_address().empty()) {
                js_v["contractAddress"] = evm_tx_receipt.get_contract_address().to_hex_string();
            } else {
                xinfo("xrpc_eth_parser_t::receipt_to_json contractAddress empty");
            }
        }
    }

    evm_common::xbloom9_t logs_bloom = evm_tx_receipt.bloom();
    js_v["logsBloom"] = top::to_hex_prefixed(logs_bloom.get_data());

    if (!evm_tx_receipt.get_logs().empty()) {
        xlog_location_t loglocation(block_hash,block_num,tx_hash,tx_idx);
        uint64_t index = 0;
        for (auto & log : evm_tx_receipt.get_logs()) {
            loglocation.m_log_index = uint64_to_hex_prefixed(index);
            index++;
            xJson::Value js_log;
            log_to_json(loglocation, log, js_log);
            js_v["logs"].append(js_log);
        }
    } else {
        js_v["logs"].resize(0);
    }
}

void xrpc_eth_parser_t::transaction_to_json(const std::string & tx_hash, xtxindex_detail_ptr_t const& sendindex, xJson::Value & js_v, std::error_code & ec) {
    std::string block_hash = top::to_hex_prefixed(sendindex->get_txindex()->get_block_hash());
    std::string block_num = uint64_to_hex_prefixed(sendindex->get_txindex()->get_block_height());
    std::string tx_idx = uint64_to_hex_prefixed((uint64_t)sendindex->get_transaction_index());
    xtx_location_t txlocation(block_hash,block_num,tx_hash,tx_idx);

    // txlocation
    js_v["blockHash"] = block_hash;
    js_v["blockNumber"] = block_num;
    js_v["hash"] = tx_hash;
    js_v["transactionIndex"] = tx_idx;

    common::xeth_address_t _from_addr = common::xeth_address_t::build_from(common::xaccount_address_t(sendindex->get_raw_tx()->get_source_addr()));
    js_v["from"] = _from_addr.to_hex_string();
    js_v["gas"] = std::string("0x") + sendindex->get_raw_tx()->get_gaslimit().str();
    js_v["gasPrice"] = std::string("0x") + sendindex->get_raw_tx()->get_max_fee_per_gas().str();
    js_v["input"] = top::to_hex_prefixed(sendindex->get_raw_tx()->get_data());
    js_v["nonce"] = uint64_to_hex_prefixed(sendindex->get_raw_tx()->get_last_nonce());
    uint16_t tx_type = sendindex->get_raw_tx()->get_tx_type();
    if (tx_type != data::xtransaction_type_deploy_evm_contract) {
        common::xeth_address_t _to_addr = common::xeth_address_t::build_from(common::xaccount_address_t(sendindex->get_raw_tx()->get_target_addr()));
        js_v["to"] = _to_addr.to_hex_string();
    } else {
        js_v["to"] = xJson::Value::null;
    }
    js_v["type"] = uint64_to_hex_prefixed((uint64_t)sendindex->get_raw_tx()->get_eip_version());
    js_v["value"] = std::string("0x") + sendindex->get_raw_tx()->get_amount_256().str();

    // TODO(jimmy)
    std::string str_v = sendindex->get_raw_tx()->get_SignV();
    uint32_t i = 0;
    for (; i < str_v.size() - 1; i++) {
        if (str_v[i] != '0') {
            break;
        }
    }
    js_v["v"] = std::string("0x") + str_v.substr(i);
    js_v["r"] = std::string("0x") + sendindex->get_raw_tx()->get_SignR();
    js_v["s"] = std::string("0x") + sendindex->get_raw_tx()->get_SignS();
}

void xrpc_eth_parser_t::blockheader_to_json(base::xvblock_t* _block, xJson::Value & js_v, std::error_code & ec) {
    data::xeth_header_t ethheader;
    data::xblockextract_t::unpack_ethheader(_block, ethheader, ec);
    if (ec) {
        return;
    }

    js_v["difficulty"] = "0x0";
    js_v["extraData"] = std::string("0x") + std::string(50, '0');  // TODO(jimmy)
    js_v["gasLimit"] = uint64_to_hex_prefixed(ethheader.get_gaslimit());
    js_v["gasUsed"] = uint64_to_hex_prefixed(ethheader.get_gasused());
    js_v["hash"] = top::to_hex_prefixed(_block->get_block_hash());
    js_v["logsBloom"] = top::to_hex_prefixed(ethheader.get_logBloom().get_data());
    js_v["miner"] = std::string("0x") + std::string(40, '0');
    js_v["mixHash"] = std::string("0x") + std::string(64, '0');
    js_v["nonce"] = std::string("0x") + std::string(16, '0');
    js_v["number"] = uint64_to_hex_prefixed(_block->get_height());
    js_v["parentHash"] = top::to_hex_prefixed(_block->get_last_block_hash());
    js_v["receiptsRoot"] = top::to_hex_prefixed(ethheader.get_receipts_root().asBytes());
    js_v["sha3Uncles"] = std::string("0x") + std::string(64, '0');
    js_v["size"] = "0x219";// TODO(jimmy)
    js_v["stateRoot"] = std::string("0x") + std::string(64, '0');
    js_v["timestamp"] = uint64_to_hex_prefixed(_block->get_timestamp());
    js_v["totalDifficulty"] = "0x0";
    js_v["transactionsRoot"] = std::string("0x") + std::string(64, '0');// TODO(jimmy)
    js_v["transactions"].resize(0);
    js_v["uncles"].resize(0);
    js_v["baseFeePerGas"] = "0x10";// TODO(jimmy)
}


}  // namespace chain_info
}  // namespace top
