// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xcommon/xerror/xerror.h"
#include "xblockmaker/xerror/xerror.h"
#include "xblockmaker/xtable_cross_plugin.h"
#include "xdata/xtx_factory.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xblockextract.h"
#include "xdata/xsystem_contract/xdata_structures.h"

NS_BEG2(top, blockmaker)

#define RELAY_CHAIN_BIT_ETH (0x01)
#define RELAY_CHAIN_BIT_BSC (0x02)

const uint16_t EXPIRE_DURATION = 300;

// string key is u256.str()
// map<chainid_bits, gas_price)
using cross_chain_contract_gasprice_info = std::map<std::string, evm_common::u256>;
//<contract_address, map<topics, pair<speed_type, chainid_bits>>>
using cross_chain_contract_info = std::map<std::string, std::map<std::string, std::pair<uint32_t, evm_common::u256>>>;

static cross_chain_contract_info get_cross_chain_config() {
    auto cross_chain_config_str = XGET_ONCHAIN_GOVERNANCE_PARAMETER(cross_chain_contract_tx_list);
    std::vector<std::string> str_vec;
    base::xstring_utl::split_string(cross_chain_config_str, ',', str_vec);
    if (str_vec.size() <= 0) {
        return {};
    }

    cross_chain_contract_info cross_chain_config;
    for (auto & str : str_vec) {
        std::vector<std::string> config_str_vec;
        base::xstring_utl::split_string(str, ':', config_str_vec);
        if (config_str_vec.size() != 4) {
            xerror("get_cross_chain_config cross_chain_contract_tx_list invalid:%s", cross_chain_config_str.c_str());
            return {};
        }

        std::string & addr = config_str_vec[0];
        std::string & topic = config_str_vec[1];
        uint32_t tx_seppd_type = static_cast<std::uint32_t>(std::stoi(config_str_vec[2]));
        evm_common::u256 chain_bit = evm_common::u256(config_str_vec[3]);
        cross_chain_config[addr][topic] =  std::make_pair(tx_seppd_type,chain_bit);
    }

    if (str_vec.size() != cross_chain_config.size()) {
        xerror("get_cross_chain_config repeat addresses in cross_chain_contract_tx_list:%s", cross_chain_config_str.c_str());
    }
    return cross_chain_config;
}

static cross_chain_contract_gasprice_info get_cross_chain_gasprice_config() {
    auto cross_chain_gasprice_config_str = XGET_ONCHAIN_GOVERNANCE_PARAMETER(cross_chain_gasprice_list);
    std::vector<std::string> str_vec;
    base::xstring_utl::split_string(cross_chain_gasprice_config_str, ',', str_vec);
    if (str_vec.size() <= 0) {
        return {};
    }
    cross_chain_contract_gasprice_info cross_chain_gasprices_config;
    for (auto& str : str_vec) {
        std::vector<std::string> config_str_vec;
        base::xstring_utl::split_string(str, ':', config_str_vec);
        if (config_str_vec.size() != 2) {
            xerror("get_cross_chain_gasprice_config cross_chain_gasprice_list invalid:%s", cross_chain_gasprice_config_str.c_str());
            return {};
        }
        evm_common::u256 chain_bit = evm_common::u256(config_str_vec[0]);
        evm_common::u256 gasprice =  evm_common::u256(config_str_vec[1]);
        cross_chain_gasprices_config[chain_bit.str()] = gasprice;
    }

    if (str_vec.size() != cross_chain_gasprices_config.size()) {
        xerror("get_cross_chain_gasprice_config repeat addresses in cross_chain_gasprice_list:%s", cross_chain_gasprice_config_str.c_str());
    }
    return cross_chain_gasprices_config;
}

static bool cross_tx_info_check_and_get(const evm_common::xevm_logs_t& logs, const cross_chain_contract_info& cross_chain_config,
                                        uint32_t& speed_type, evm_common::u256& chain_bit) {
    for (auto& log : logs) {
        auto it = cross_chain_config.find(log.address.to_hex_string());
        if (it == cross_chain_config.end()) {
            continue;
        }

        std::string topic_hex = top::to_hex_prefixed(log.topics[0].asBytes());
        auto it2 = it->second.find(topic_hex);
        if (it2 != it->second.end()) {
            speed_type = it2->second.first;
            chain_bit = it2->second.second;
            return true;
        }
    }
    return false;
}

void xtable_cross_plugin_t::init(statectx::xstatectx_ptr_t const& statectx_ptr, std::error_code& ec)
{
    common::xaccount_address_t const& address = eth_table_cross_chain_txs_collection_contract_address;
    m_cross_block_contract_state = statectx_ptr->load_account_state(address);
    if (nullptr == m_cross_block_contract_state) {
        ec = blockmaker::error::xerrc_t::blockmaker_load_unitstate;
        xerror("xtable_cross_plugin_t::init fail-load unitstate.%s", address.to_string().c_str());
        return;
    }
    xdbg("xtable_cross_plugin_t::init accountnonce=%ld", m_cross_block_contract_state->get_tx_nonce());
}

std::vector<data::xcons_transaction_ptr_t> xtable_cross_plugin_t::make_contract_txs_after_execution(statectx::xstatectx_ptr_t const& statectx_ptr, uint64_t timestamp,
    std::vector<top::txexecutor::xatomictx_output_t> const& pack_outputs, std::error_code& ec)
{
    data::xrelayblock_crosstx_infos_t all_crosstxs;
    std::vector<data::xcons_transaction_ptr_t> cross_tx_vec {};
    if (pack_outputs.size() == 0) {
        return {};
    }

#ifndef CROSS_TX_DBG
    cross_chain_contract_info cross_chain_config = get_cross_chain_config();
    if (cross_chain_config.empty()) {
        xdbg("xtable_cross_plugin_t::make_contract_txs_after_execution cross_chain_config is empty.");
        return {};
    }
    auto cross_chain_contract_gasprice_info = get_cross_chain_gasprice_config();
#endif

    uint64_t gas_used = 0;
    data::xeth_receipts_t eth_receipts;
    for (auto& txout : pack_outputs) {
        if (txout.m_tx->get_tx_version() != data::xtransaction_version_3) {
            continue;
        }

        auto& evm_result = txout.m_vm_output.m_tx_result;
        gas_used += evm_result.used_gas;
        data::enum_ethreceipt_status status = (evm_result.status == evm_common::xevm_transaction_status_t::Success) ? data::ethreceipt_status_successful : data::ethreceipt_status_failed;
        if (status != data::ethreceipt_status_successful) {
            continue;
        }
       
        uint32_t speed_type = 0;
        evm_common::u256 gasprice = ULONG_LONG_MAX;
        evm_common::u256 chain_bit = RELAY_CHAIN_BIT_ETH;

#ifndef CROSS_TX_DBG
        if (!cross_tx_info_check_and_get(evm_result.logs, cross_chain_config, speed_type, chain_bit)) {
            // xdbg("xtable_cross_plugin_t::make_contract_txs_after_execution  topic not match.tx:%s is not a cross chain tx", top::to_hex_prefixed(top::to_bytes(txout.m_tx.get_tx_hash())).c_str());
            continue;
        }

        auto iter = cross_chain_contract_gasprice_info.find(chain_bit.str());
        if (iter != cross_chain_contract_gasprice_info.end()) {
            gasprice = iter->second;
        }
#endif

        data::xeth_transaction_t ethtx = txout.m_tx->get_transaction()->to_eth_tx(ec);
        if (ec) {
            xerror("xtable_cross_plugin_t::xeth_header_builder::build fail-to eth tx");
            continue;
        }

        if (speed_type == 1 && ethtx.get_max_priority_fee_per_gas() >= gasprice) {
            speed_type = 1;
        } else {
            speed_type = 0;
        }
#ifdef CROSS_TX_DBG // debug mode always fast speed
        speed_type = 1;
#endif
        data::xeth_receipt_t eth_receipt(ethtx.get_tx_version(), status, gas_used, evm_result.logs);
        eth_receipt.create_bloom();
        eth_receipts.push_back(eth_receipt);
        data::xrelayblock_crosstx_info_t info(ethtx, eth_receipt, speed_type, chain_bit);
        all_crosstxs.tx_infos.push_back(info);
    }

    if (all_crosstxs.tx_infos.size() > 0) {
        std::string func_name = "updateCrossTx";
        std::string call_params = all_crosstxs.serialize_to_string();
        base::xstream_t stream(base::xcontext_t::instance());
        stream << call_params;
        data::xtransaction_ptr_t tx = data::xtx_factory::create_sys_contract_call_self_tx(eth_table_cross_chain_txs_collection_contract_address.to_string(),
            m_cross_block_contract_state->get_tx_nonce(), func_name,
            std::string((char*)stream.data(), stream.size()), timestamp, EXPIRE_DURATION);
        data::xcons_transaction_ptr_t contx = make_object_ptr<data::xcons_transaction_t>(tx.get());
        cross_tx_vec.push_back(contx);
    } else {
        xdbg("xtable_cross_plugin_t all corsstx is empty.");
    }

    return cross_tx_vec;
}

NS_END2
