// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbase/xutl.h"
#include "xbasic/xhex.h"
#include "xvledger/xvblock_offdata.h"
#include "xdata/xblockextract.h"
#include "xdata/xblockbuild.h"
#include "xdata/xnative_contract_address.h"
#include "xbase/xutl.h"
#include "xcommon/xerror/xerror.h"
#include "xconfig/xpredefined_configurations.h"
#include "xconfig/xconfig_register.h"
#include "xpbase/base/top_utils.h"

NS_BEG2(top, data)

uint32_t xblockextract_t::get_txactions_count(base::xvblock_t* _block) {
    // TODO(jimmy) optimize
    std::vector<xlightunit_action_t> txactions = unpack_txactions(_block);
    return (uint32_t)txactions.size();
}

std::vector<xlightunit_action_t> xblockextract_t::unpack_txactions(base::xvblock_t* _block) {
    if (_block == nullptr) {
        xassert(false);
        return {};
    }

    if (_block->get_block_class() == base::enum_xvblock_class_nil) {
        return {};
    }

    std::vector<xlightunit_action_t> txactions;
    auto & all_entitys = _block->get_input()->get_entitys();
    for (auto & entity : all_entitys) {
        // it must be xinentitys
        base::xvinentity_t* _inentity = dynamic_cast<base::xvinentity_t*>(entity);
        if (_inentity == nullptr) {
            xassert(false);
            return {};
        }
        auto & all_actions = _inentity->get_actions();
        for (auto & action : all_actions) {
            if (action.get_org_tx_hash().empty()) {
                continue;
            }
            xlightunit_action_t txaction(action);
            txactions.push_back(txaction);
        }
    }

    base::xvinentity_t* primary_input_entity = _block->get_input()->get_primary_entity();
    xassert(primary_input_entity != nullptr);
    if (!primary_input_entity->get_extend_data().empty()) {
        xtable_primary_inentity_extend_t _extend;
        int32_t ret = _extend.serialize_from_string(primary_input_entity->get_extend_data());
        if (ret <= 0) {
            xassert(false);
            return {};
        }
        base::xvactions_t _ethreceipt_actions = _extend.get_txactions();
        for (auto & action : _ethreceipt_actions.get_actions()) {
            if (action.get_org_tx_hash().empty()) {
                xassert(false);
                continue;
            }
            xlightunit_action_t txaction(action);
            txactions.push_back(txaction);
        }        
    }

    return txactions;
}

std::vector<xlightunit_action_t> xblockextract_t::unpack_eth_txactions(base::xvblock_t* _block) {
    if (_block == nullptr) {
        xassert(false);
        return {};
    }

    if (_block->get_block_class() == base::enum_xvblock_class_nil) {
        return {};
    }

    std::vector<xlightunit_action_t> txactions;
    base::xvinentity_t* primary_input_entity = _block->get_input()->get_primary_entity();
    xassert(primary_input_entity != nullptr);
    if (!primary_input_entity->get_extend_data().empty()) {
        xtable_primary_inentity_extend_t _extend;
        int32_t ret = _extend.serialize_from_string(primary_input_entity->get_extend_data());
        if (ret <= 0) {
            xassert(false);
            return {};
        }
        base::xvactions_t _ethreceipt_actions = _extend.get_txactions();
        for (auto & action : _ethreceipt_actions.get_actions()) {
            if (action.get_org_tx_hash().empty()) {
                xassert(false);
                continue;
            }
            xlightunit_action_t txaction(action);
            txactions.push_back(txaction);
        }
    }
    return txactions;
}

xlightunit_action_ptr_t xblockextract_t::unpack_one_txaction(base::xvblock_t* _block, std::string const& txhash) {
    std::vector<xlightunit_action_t> txactions = unpack_txactions(_block);
    for (auto & action : txactions) {
        if (action.get_org_tx_hash() == txhash) {
            data::xlightunit_action_ptr_t txaction_ptr = std::make_shared<xlightunit_action_t>(action);
            return txaction_ptr;
        }
    }
    xerror("xblockextract_t::unpack_one_txaction fail-find tx.block=%s,tx=%s", _block->dump().c_str(), base::xstring_utl::to_hex(txhash).c_str());
    return nullptr;
}

void xblockextract_t::unpack_ethheader(base::xvblock_t* _block, xeth_header_t & ethheader, std::error_code & ec) {
    if (_block->is_genesis_block()) {
        uint64_t gas_limit = XGET_ONCHAIN_GOVERNANCE_PARAMETER(block_gas_limit);
        ethheader.set_gaslimit(gas_limit);
        return;
    }

    xassert(_block->get_block_level() == base::enum_xvblock_level_table);
    data::xtableheader_extra_t header_extra;
    get_tableheader_extra_from_block(_block, header_extra, ec);
    if (ec) {
        return;
    }
    std::string eth_header_str = header_extra.get_ethheader();
    if (eth_header_str.empty()) {
        ec = common::error::xerrc_t::invalid_block;
        xerror("xblockextract_t::unpack_ethheader fail-eth_header_str empty.block=%s",_block->dump().c_str());
        return;
    }
    ethheader.serialize_from_string(eth_header_str, ec);
    if (ec) {
        xerror("xeth_header_builder::string_to_eth_header decode fail");
        return;
    }
}

bool xblockextract_t::get_state_root(base::xvblock_t* _block, evm_common::xh256_t & state_root) {
    if (_block->get_height() == 0 || !base::xvblock_fork_t::is_block_match_version(_block->get_block_version(), base::enum_xvblock_fork_version_5_0_0)) {
        xdbg("xblockextract_t::get_state_root block is old version or height = 0 block:%s", _block->dump().c_str());
        state_root = evm_common::xh256_t();
        return true;
    }

    data::xeth_header_t ethheader;
    std::error_code ec;
    unpack_ethheader(_block, ethheader, ec);
    if (ec) {
        return false;
    }

   state_root = ethheader.get_state_root();
   return true;
}

xhash256_t xblockextract_t::get_state_root_from_block(base::xvblock_t * block) {
    evm_common::xh256_t state_root;
    auto ret = data::xblockextract_t::get_state_root(block, state_root);
    if (!ret) {  // should not happen
        xerror("xblockextract_t::get_state_root_from_block get state root fail. block:%s", block->dump().c_str());
        return xhash256_t{};
    }
    xhash256_t root_hash = xhash256_t{state_root};
    return root_hash;
}

xtransaction_ptr_t xblockextract_t::unpack_raw_tx(base::xvblock_t* _block, std::string const& txhash, std::error_code & ec) {
    std::string orgtx_bin = _block->get_input()->query_resource(txhash);
    if (orgtx_bin.empty()) {
        ec = common::error::xerrc_t::invalid_block;
        xerror("xblockextract_t::unpack_raw_tx fail-query tx resouce._block=%s,tx=%s", _block->dump().c_str(), base::xstring_utl::to_hex(txhash).c_str());
        return nullptr;
    }
    base::xauto_ptr<base::xdataunit_t> raw_tx = base::xdataunit_t::read_from(orgtx_bin);
    if(nullptr == raw_tx) {
        ec = common::error::xerrc_t::invalid_block;
        xerror("xblockextract_t::unpack_raw_tx fail-query tx resouce._block=%s,tx=%s", _block->dump().c_str(), base::xstring_utl::to_hex(txhash).c_str());
        return nullptr;
    }
    xtransaction_ptr_t tx;
    data::xtransaction_t* _tx_ptr = dynamic_cast<data::xtransaction_t*>(raw_tx.get());
    _tx_ptr->add_ref();
    tx.attach(_tx_ptr);
    return tx;
}

std::shared_ptr<xrelay_block> xblockextract_t::unpack_relay_block_from_table(base::xvblock_t* _block, std::error_code & ec) {
    if (_block->get_account() != sys_contract_relay_table_block_addr) {
        ec = common::error::xerrc_t::invalid_block;
        xerror("xblockextract_t::unpack_relay_block_from_table fail-invalid addr._block=%s", _block->dump().c_str());
        return nullptr;
    }

    std::string relayblock_resource = _block->get_output()->query_resource(base::xvoutput_t::RESOURCE_RELAY_BLOCK);
    if (relayblock_resource.empty()) {
        ec = common::error::xerrc_t::invalid_block;
        xerror("xblockextract_t::unpack_relay_block_from_table fail-relayblock_resource empty._block=%s", _block->dump().c_str());
        return nullptr;
    }

    std::shared_ptr<xrelay_block> relayblock = std::make_shared<xrelay_block>();
    relayblock->decodeBytes(to_bytes(relayblock_resource), ec);
    if (ec) {
        xerror("xblockextract_t::unpack_relay_block_from_table fail-decode relayblock。error=%s", ec.message().c_str());
        return nullptr;
    }
    return relayblock;
}

std::shared_ptr<xrelay_block> xblockextract_t::unpack_commit_relay_block_from_relay_table(base::xvblock_t* _block, std::error_code & ec) {
    if (!(_block->get_cert()->get_consensus_flags() & base::enum_xconsensus_flag_extend_vote)) {
        xdbg("xblockextract_t::unpack_commit_relay_block_from_relay_table it's not commit relayblock. %s", _block->dump().c_str());
        return nullptr;
    }

    std::shared_ptr<xrelay_block> relayblock = unpack_relay_block_from_table(_block, ec);
    if (nullptr == relayblock) {
        xerror("xblockextract_t::unpack_commit_relay_block_from_relay_table fail-decode relayblock,block:%s,error=%s", _block->dump().c_str(),ec.message().c_str());
        return nullptr;
    }

    // commit relay table must has signature
    std::string extend_data = _block->get_cert()->get_extend_data();
    if (extend_data.empty()) {
        ec = common::error::xerrc_t::invalid_block;
        xerror("xblockextract_t::unpack_commit_relay_block_from_relay_table fail-extend data empty.");
        return nullptr;
    }

    data::xrelay_signature_group_t siggroup;
    siggroup.decodeBytes(top::to_bytes(extend_data), ec);
    if (ec) {
        xerror("xblockextract_t::unpack_commit_relay_block_from_relay_table fail-decode extend.block:%s", _block->dump().c_str());
        return nullptr;        
    }

    relayblock->set_viewid(_block->get_viewid());
    relayblock->set_signature_groups(siggroup);
    return relayblock;
}

xobject_ptr_t<base::xvblock_t> xblockextract_t::pack_relayblock_to_wrapblock(xrelay_block const& relayblock, std::error_code & ec) {
    xbytes_t _bs = relayblock.encodeBytes();
    std::string bin_data = top::to_string(_bs);
    if (relayblock.get_block_height() == 0) {
        xemptyblock_build_t bbuild(sys_contract_relay_block_addr);
        bbuild.set_header_extra(bin_data);
        xobject_ptr_t<base::xvblock_t> _new_block = bbuild.build_new_block();
        return _new_block;
    } else {
        xemptyblock_build_t bbuild(sys_contract_relay_block_addr, relayblock.get_block_height(), relayblock.get_viewid(), bin_data);
        xobject_ptr_t<base::xvblock_t> _new_block = bbuild.build_new_block();
        xvip2_t target_xip{(xvip_t)(1),(uint64_t)1};// mock leader xip for xvblock rules
        _new_block->get_cert()->set_validator(target_xip); 
        _new_block->set_verify_signature(std::string(1,0));  // mock signature 
        _new_block->set_block_flag(base::enum_xvblock_flag_authenticated);
        return _new_block;
    }
}
void xblockextract_t::unpack_relayblock_from_wrapblock(base::xvblock_t* _block, xrelay_block & relayblock, std::error_code & ec) {
    if (_block->get_account() != sys_contract_relay_block_addr) {
        ec = common::error::xerrc_t::invalid_block;
        xerror("xblockextract_t::unpack_relayblock_from_wrapblock fail-invalid address.");
        return;
    }
    auto relay_block_data_str = _block->get_header()->get_extra_data();
    if (relay_block_data_str.empty()) {
        ec = common::error::xerrc_t::invalid_block;
        xerror("xblockextract_t::unpack_relayblock_from_wrapblock fail-extra data empty.");
        return;
    }
    relayblock.decodeBytes(to_bytes(relay_block_data_str), ec);
    if (ec) {
        xerror("xblockextract_t::unpack_relayblock_from_wrapblock fail-decode.");
    }
    return;
}
xobject_ptr_t<base::xvblock_t> xblockextract_t::unpack_wrap_relayblock_from_relay_table(base::xvblock_t* _block, std::error_code & ec) {
    std::shared_ptr<xrelay_block> relayblock = unpack_commit_relay_block_from_relay_table(_block, ec);
    if (ec) {
        ec = common::error::xerrc_t::invalid_block;
        xerror("xblockextract_t::unpack_wrap_relayblock_from_relay_table fail-unpack commit relayblock.");        
        return nullptr;
    }
    if (nullptr == relayblock) {
        // it's ok, the table block may not commit
        return nullptr;
    }
    xobject_ptr_t<base::xvblock_t> wrap_relayblock = pack_relayblock_to_wrapblock(*relayblock, ec);
    if (ec) {
        ec = common::error::xerrc_t::invalid_block;
        xerror("xblockextract_t::unpack_wrap_relayblock_from_relay_table fail-pack relayblock.");        
        return nullptr;
    }
    xinfo("xblockextract_t::unpack_wrap_relayblock_from_relay_table,%s,%s",relayblock->dump().c_str(),wrap_relayblock->dump().c_str());
    return wrap_relayblock;
}

void xblockextract_t::get_tableheader_extra_from_block(base::xvblock_t* _block, data::xtableheader_extra_t &header_extra, std::error_code & ec) {
    auto & header_extra_str = _block->get_header()->get_extra_data();
    if (header_extra_str.empty()) {
        ec = common::error::xerrc_t::invalid_block;
        xerror("xblockextract_t::get_tableheader_extra_from_block parameters invalid.");
        return;
    }

    auto ret = header_extra.deserialize_from_string(header_extra_str);
    if (ret <= 0) {
        ec = common::error::xerrc_t::invalid_block;
        xerror("xblockextract_t::get_tableheader_extra_from_block header extra data deserialize fail.");
        return;
    }
}

cross_chain_contract_info xblockextract_t::get_cross_chain_config() {
    auto cross_chain_config_str = XGET_ONCHAIN_GOVERNANCE_PARAMETER(cross_chain_contract_list);
    std::vector<std::string> str_vec;
    base::xstring_utl::split_string(cross_chain_config_str, ',', str_vec);
    if (str_vec.size() <= 0) {
        return {};
    }

    cross_chain_contract_info cross_chain_config;
    for (auto & str : str_vec) {
        std::vector<std::string> config_str_vec;
        base::xstring_utl::split_string(str, ':', config_str_vec);
        if (config_str_vec.size() != 3) {
            xerror("xblockextract_t::get_cross_chain_config cross_chain_contract_list invalid:%s", cross_chain_config_str.c_str());
            return {};
        }
        cross_chain_contract_info info;
        std::string & addr = config_str_vec[0];
        std::string & topic = config_str_vec[1];
        uint32_t chain_bits_shift = static_cast<std::uint32_t>(std::stoi(config_str_vec[2]));
        evm_common::u256 chain_bits = (1 << chain_bits_shift);
        cross_chain_config[addr] = std::make_pair(topic, chain_bits);
    }

    if (str_vec.size() != cross_chain_config.size()) {
        xerror("xblockextract_t::get_cross_chain_config repeat addresses in cross_chain_contract_list:%s", cross_chain_config_str.c_str());
    }

    return cross_chain_config;
}

bool xblockextract_t::is_cross_tx(const evm_common::xevm_logs_t & logs, const cross_chain_contract_info & cross_chain_config) {
    for (auto & log : logs) {
        auto it = cross_chain_config.find(log.address.to_hex_string());
        if (it == cross_chain_config.end()) {
            continue;
        }

        std::string topic_hex = top::to_hex_prefixed(log.topics[0].asBytes());
        if ((topic_hex == it->second.first)) {
            return true;
        }
    }
    return false;
}

bool xblockextract_t::get_chain_bits(const evm_common::xevm_logs_t & logs, const cross_chain_contract_info & cross_chain_config, evm_common::u256 & chain_bits) {
    for (auto & log : logs) {
        auto it = cross_chain_config.find(log.address.to_hex_string());
        if (it == cross_chain_config.end()) {
            continue;
        }
        chain_bits = it->second.second;
        return true;
    }
    return false;
}

void xblockextract_t::unpack_crosschain_txs(base::xvblock_t* _block, xrelayblock_crosstx_infos_t & infos, std::error_code & ec) {
    bool config_loaded = false;
    cross_chain_contract_info cross_chain_config;
    data::xblock_t * block = dynamic_cast<data::xblock_t*>(_block);
    if (nullptr == block) {
        ec = common::error::xerrc_t::invalid_block;
        xerror("xblockextract_t::unpack_crosschain_txs block nullptr.");
        return;
    }

    xdbg("xblockextract_t::unpack_crosschain_txs process. block:%s", block->dump().c_str());
    if (_block->get_block_class() == base::enum_xvblock_class_nil) {
        return;
    }

    auto input_actions = data::xblockextract_t::unpack_eth_txactions(_block);
    for (auto & txaction : input_actions) {
#ifndef CROSS_TX_DBG
        if (txaction.get_tx_subtype() != base::enum_transaction_subtype_send) {
            continue;
        }
#endif

        data::xeth_store_receipt_t evm_result;
        auto ret = txaction.get_evm_transaction_receipt(evm_result);
        if (!ret) {
            ec = common::error::xerrc_t::invalid_block;
            xerror("xblockextract_t::unpack_crosschain_txs get evm tx result fail. block:%s", block->dump().c_str());
            return;
        }

        if (evm_result.get_tx_status() != data::enum_ethreceipt_status::ethreceipt_status_successful) {
            continue;
        }

#ifndef CROSS_TX_DBG
        if (!config_loaded) {
            cross_chain_config = get_cross_chain_config();
            config_loaded = true;
        }

        if (!is_cross_tx(evm_result.get_logs(), cross_chain_config)) {
            xdbg("xblockextract_t::unpack_crosschain_txs topic not match.tx:%s is not a cross chain tx", top::to_hex_prefixed(top::to_bytes(txaction.get_tx_hash())).c_str());
            continue;
        }
#endif

        data::xtransaction_ptr_t _rawtx = block->query_raw_transaction(txaction.get_tx_hash());
        if (nullptr == _rawtx) {
            ec = common::error::xerrc_t::invalid_block;
            xerror("xblockextract_t::unpack_crosschain_txs tx nullptr.");
            return;
        }

        xeth_transaction_t ethtx = _rawtx->to_eth_tx(ec);
        if (ec) {
            xerror("xblockextract_t::unpack_crosschain_txs to eth tx fail.");
            return;
        }
        data::xeth_receipt_t receipt;
        receipt.set_tx_status(evm_result.get_tx_status());
        receipt.set_cumulative_gas_used(evm_result.get_cumulative_gas_used());
        receipt.set_logs(evm_result.get_logs());
        receipt.create_bloom();

        xrelayblock_crosstx_info_t info(ethtx, receipt);
        infos.tx_infos.push_back(info);
        xinfo("xblockextract_t::unpack_crosschain_txs succ.block=%s,tx=%s", block->dump().c_str(), _rawtx->dump().c_str());
    }
}

void xblockextract_t::unpack_subblocks(base::xvblock_t* _block, std::vector<xobject_ptr_t<base::xvblock_t>> & sublocks, std::error_code & ec) {
    if (_block->get_block_level() != base::enum_xvblock_level_table) {
        ec = common::error::xerrc_t::invalid_block;
        xerror("xblockextract_t::unpack_subblocks should be table level block.");
        return;
    }

    if (_block->get_block_class() == base::enum_xvblock_class_nil) {
        return;
    }

    // TODO(jimmy) full-table block should be same with light-table in future
    if (_block->get_block_class() == base::enum_xvblock_class_full) {
        return;
    }

    if (!_block->is_body_and_offdata_ready(false)) {
        ec = common::error::xerrc_t::invalid_block;
        xerror("xblockextract_t::unpack_subblocks input and output should ready. %s", _block->dump().c_str());
        return;
    }

    if (base::xvblock_fork_t::is_block_older_version(_block->get_block_version(), base::enum_xvblock_fork_version_5_0_0)) {
        sublocks = xlighttable_build_t::unpack_units_from_table(_block);
    } else {
        sublocks = xtable_build2_t::unpack_units_from_table(_block, ec);
    }

    xdbg("xblockextract_t::unpack_subblocks succ.block=%s,sublocks=%zu",_block->dump().c_str(),sublocks.size());
}

NS_END2
