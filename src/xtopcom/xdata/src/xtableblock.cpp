// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbase/xutl.h"
#include "xvledger/xvblockbuild.h"
#include "xvledger/xvledger.h"
#include "xdata/xtableblock.h"
#include "xdata/xlightunit.h"
#include "xdata/xrootblock.h"
#include "xdata/xemptyblock.h"
#include "xdata/xblockbuild.h"
#include "xdata/xblockaction.h"

NS_BEG2(top, data)

xtable_block_t::xtable_block_t(base::xvheader_t & header, base::xvqcert_t & cert, base::xvinput_t* input, base::xvoutput_t* output)
: xblock_t(header, cert, input, output, (enum_xdata_type)object_type_value) {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_block_lighttable, 1);
}

xtable_block_t::xtable_block_t()
: xblock_t((enum_xdata_type)object_type_value) {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_block_lighttable, 1);
}

xtable_block_t::~xtable_block_t() {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_block_lighttable, -1);
}

base::xobject_t * xtable_block_t::create_object(int type) {
    (void)type;
    return new xtable_block_t;
}

void * xtable_block_t::query_interface(const int32_t _enum_xobject_type_) {
    if (object_type_value == _enum_xobject_type_)
        return this;
    return xvblock_t::query_interface(_enum_xobject_type_);
}

void xtable_block_t::parse_to_json(xJson::Value & root, const std::string & rpc_version) {
    if (rpc_version == RPC_VERSION_V2) {
        parse_to_json_v2(root);
    } else {
        parse_to_json_v1(root);
    }
}

void xtable_block_t::parse_to_json_v1(xJson::Value & root) {
    const std::vector<base::xventity_t*> & _table_inentitys = get_input()->get_entitys();
    uint32_t entitys_count = _table_inentitys.size();
    for (uint32_t index = 1; index < entitys_count; index++) {  // unit entity from index#1
        base::xvinentity_t* _table_unit_inentity = dynamic_cast<base::xvinentity_t*>(_table_inentitys[index]);
        base::xtable_inentity_extend_t extend;
        extend.serialize_from_string(_table_unit_inentity->get_extend_data());
        const xobject_ptr_t<base::xvheader_t> & _unit_header = extend.get_unit_header();

        xJson::Value jui;
        jui["unit_height"] = static_cast<xJson::UInt64>(_unit_header->get_height());

        xJson::Value jv;
        // for block version 2, no action in _table_unit_inentity
        const std::vector<base::xvaction_t> &  input_actions = _table_unit_inentity->get_actions();
        for (auto & action : input_actions) {
            if (action.get_org_tx_hash().empty()) {  // not txaction
                continue;
            }
            xlightunit_action_t txaction(action);
            xJson::Value juj;
            juj["tx_consensus_phase"] = txaction.get_tx_subtype_str();
            juj["sender_tx_locked_gas"] = static_cast<unsigned int>(txaction.get_send_tx_lock_tgas());

            xtransaction_ptr_t _rawtx = query_raw_transaction(txaction.get_tx_hash());
            uint64_t last_tx_nonce = 0;
            if (_rawtx != nullptr) {
                last_tx_nonce = _rawtx->get_last_nonce();
                juj["sender"] = _rawtx->get_source_addr();
                juj["receiver"] = _rawtx->get_target_addr();
                juj["action_name"] = _rawtx->get_target_action_name();
                juj["action_param"] = to_hex_str(_rawtx->get_target_action_para());
            }
            if (txaction.is_self_tx() || txaction.is_send_tx()) {
                juj["last_tx_nonce"] = static_cast<unsigned int>(last_tx_nonce);
            }

            jv["0x" + txaction.get_tx_hex_hash()] = juj;
        }
        jui["lightunit_input"] = jv;
        root["tableblock"]["units"][_unit_header->get_account()] = jui;
    }
}

void xtable_block_t::parse_to_json_v2(xJson::Value & root) {
    xJson::Value jv;
    const std::vector<base::xvaction_t> input_actions = get_tx_actions();
    for(auto action : input_actions) {
        if (action.get_org_tx_hash().empty()) {  // not txaction
            continue;
        }
        xlightunit_action_t txaction(action);
        xJson::Value ju;
        ju["tx_consensus_phase"] = txaction.get_tx_subtype_str();
        ju["tx_hash"] = "0x" + to_hex_str(action.get_org_tx_hash());
        jv["txs"].append(ju);
    }

    auto headers = get_sub_block_headers();
    for (auto _unit_header : headers) {
        xJson::Value ju;
        ju["unit_height"] = static_cast<xJson::UInt64>(_unit_header->get_height());
        ju["account"] = _unit_header->get_account();
        jv["units"].append(ju);
    }
    root["tableblock"] = jv;
}

std::vector<base::xvaction_t> xtable_block_t::get_tx_actions() const {
    const std::vector<base::xventity_t*> & _table_inentitys = get_input()->get_entitys();
    auto version = get_block_version();
    if (base::xvblock_fork_t::is_block_older_version(version, base::enum_xvblock_fork_version_unit_opt)) {
        std::vector<base::xvaction_t> tx_actions;
        uint32_t entitys_count = _table_inentitys.size();
        for (uint32_t index = 1; index < entitys_count; index++) {  // unit entity from index#1
            base::xvinentity_t* _table_unit_inentity = dynamic_cast<base::xvinentity_t*>(_table_inentitys[index]);
            const std::vector<base::xvaction_t> &  input_actions = _table_unit_inentity->get_actions();
            for (auto & action : input_actions) {
                tx_actions.push_back(base::xvaction_t(action));
            }
        }
        return tx_actions;
    } else {
        base::xvinentity_t* tx_inentity = dynamic_cast<base::xvinentity_t*>(_table_inentitys[0]);
        return tx_inentity->get_actions();
    }
}

std::vector<xvheader_ptr_t> xtable_block_t::get_sub_block_headers() const {
    std::vector<xvheader_ptr_t> unit_headers;
    const std::vector<base::xventity_t*> & _table_inentitys = get_input()->get_entitys();
    uint32_t entitys_count = _table_inentitys.size();
    for (uint32_t index = 1; index < entitys_count; index++) {  // unit entity from index#1
        base::xvinentity_t* _table_unit_inentity = dynamic_cast<base::xvinentity_t*>(_table_inentitys[index]);
        base::xtable_inentity_extend_t extend;
        extend.serialize_from_string(_table_unit_inentity->get_extend_data());
        const xvheader_ptr_t & _unit_header = extend.get_unit_header();
        unit_headers.push_back(_unit_header);
    }
    return unit_headers;
}


uint32_t xtable_block_t::get_txs_count() const {
    // txs count is equal to actions count, mini-block is enough
    uint32_t tx_count = 0;
    const std::vector<base::xventity_t*> & _table_inentitys = get_input()->get_entitys();
    uint32_t entitys_count = _table_inentitys.size();
    for (uint32_t index = 1; index < entitys_count; index++) {  // unit entity from index#1
        base::xvinentity_t* _table_unit_inentity = dynamic_cast<base::xvinentity_t*>(_table_inentitys[index]);
        const std::vector<base::xvaction_t> &  input_actions = _table_unit_inentity->get_actions();
        for (auto & action : input_actions) {
            if (action.get_org_tx_hash().empty()) {  // not txaction
                continue;
            }
            tx_count++;
        }
    }
    return tx_count;
}

int64_t xtable_block_t::get_pledge_balance_change_tgas() const {
    auto out_entity = get_output()->get_primary_entity();
    xassert(out_entity != nullptr);
    int64_t tgas_balance_change = 0;
    if (out_entity != nullptr) {
        tgas_balance_change = base::xstring_utl::toint64(out_entity->query_value(base::xvoutentity_t::key_name_tgas_pledge_change()));
    }
    return tgas_balance_change;
}

bool  xtable_block_t::extract_sub_blocks(std::vector<xobject_ptr_t<base::xvblock_t>> & sub_blocks) {
    std::vector<xobject_ptr_t<base::xvblock_t>> _units = xlighttable_build_t::unpack_units_from_table(this);
    sub_blocks = _units;
    return true;
}

bool xtable_block_t::extract_one_sub_block(uint32_t entity_id, const std::string & extend_cert, const std::string & extend_data, xobject_ptr_t<xvblock_t> & sub_block) {
    sub_block = xlighttable_build_t::unpack_one_unit_from_table(this, entity_id, extend_cert, extend_data);
    return sub_block != nullptr ? true : false;
}

void xtable_block_t::update_txs_by_actions(const std::vector<base::xvaction_t> & actions, std::vector<xlightunit_tx_info_ptr_t> & txs) const {
    for (auto & action : actions) {
        if (action.get_org_tx_hash().empty()) {  // not txaction
            continue;
        }
        xtransaction_ptr_t raw_tx = query_raw_transaction(action.get_org_tx_hash());
        xlightunit_tx_info_ptr_t txinfo = std::make_shared<xlightunit_tx_info_t>(action, raw_tx.get());
        txs.push_back(txinfo);
    }
}

const std::vector<xlightunit_tx_info_ptr_t> xtable_block_t::get_txs() const {
    std::vector<xlightunit_tx_info_ptr_t> txs;
    auto txactions = get_tx_actions();
    update_txs_by_actions(txactions, txs);
    return txs;
}

bool xtable_block_t::extract_sub_txs(std::vector<base::xvtxindex_ptr> & sub_txs) {
    auto tx_actions = get_tx_actions();
    xdbg("xtable_block_t::extract_sub_txs tx_action size:%zu, account:%s, height:%llu", tx_actions.size(), get_account().c_str(), get_height());
    for (auto & tx : tx_actions) {
        xtransaction_ptr_t raw_tx = query_raw_transaction(tx.get_org_tx_hash());
        if (!tx.get_org_tx_hash().empty()) {
            xlightunit_action_t txaction(tx);
            xdbg("extract_sub_txs tx_hash:%s, account: %s, height:%llu, type:%s", to_hex_str(tx.get_org_tx_hash()).c_str(), get_account().c_str(), get_height(), txaction.get_tx_subtype_str().c_str());
            base::xvtxindex_ptr tx_index = make_object_ptr<base::xvtxindex_t>(*this, dynamic_cast<xdataunit_t*>(raw_tx.get()), tx.get_org_tx_hash(), static_cast<enum_transaction_subtype>(txaction.get_tx_subtype()));
            sub_txs.push_back(tx_index);
        }
    }

    return true;
}

NS_END2
