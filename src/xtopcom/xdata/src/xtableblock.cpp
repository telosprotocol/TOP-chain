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
    XMETRICS_GAUGE(metrics::dataobject_block_lighttable, 1);
}

xtable_block_t::xtable_block_t()
: xblock_t((enum_xdata_type)object_type_value) {
    XMETRICS_GAUGE(metrics::dataobject_block_lighttable, 1);
}

xtable_block_t::~xtable_block_t() {
    XMETRICS_GAUGE(metrics::dataobject_block_lighttable, -1);
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
        parse_to_json_v2(root, rpc_version);
    } else {
        // need input
        base::xvaccount_t _vaccount(get_account());
        base::xvchain_t::instance().get_xblockstore()->load_block_input(_vaccount, this, metrics::blockstore_access_from_rpc_get_block_set_table);

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

            const std::vector<base::xvaction_t> &  input_actions = _table_unit_inentity->get_actions();
            for (auto & action : input_actions) {
                if (action.get_org_tx_hash().empty()) {  // not txaction
                    continue;
                }
                xlightunit_action_t txaction(action);
                xJson::Value juj;
                juj["tx_consensus_phase"] = txaction.get_tx_subtype_str();
                juj["sender_tx_locked_gas"] = static_cast<unsigned int>(txaction.get_send_tx_lock_tgas());
                jv["0x" + txaction.get_tx_hex_hash()] = juj;
            }
            jui["lightunit_input"] = jv;
            root["tableblock"]["units"][_unit_header->get_account()] = jui;
        }
    }
}

void xtable_block_t::parse_to_json_v2(xJson::Value & root, const std::string & rpc_version) {
    // need input
    base::xvaccount_t _vaccount(get_account());
    base::xvchain_t::instance().get_xblockstore()->load_block_input(_vaccount, this, metrics::blockstore_access_from_rpc_get_block_set_table);
    const std::vector<base::xventity_t*> & _table_inentitys = get_input()->get_entitys();

    xJson::Value jv;
    base::xvinentity_t* tx_inentity = dynamic_cast<base::xvinentity_t*>(_table_inentitys[0]);
    const std::vector<base::xvaction_t> &  input_actions = tx_inentity->get_actions();
    for(auto action : input_actions) {
        if (action.get_org_tx_hash().empty()) {  // not txaction
            continue;
        }
        xlightunit_action_t txaction(action);
        xJson::Value ju;
        ju["tx_consensus_phase"] = txaction.get_tx_subtype_str();
        ju["sender_tx_locked_gas"] = static_cast<unsigned int>(txaction.get_send_tx_lock_tgas());
        ju["tx_hash"] = to_hex_str(action.get_org_tx_hash());
        jv["txs"].append(ju);
    }

    uint32_t entitys_count = _table_inentitys.size();
    for (uint32_t index = 1; index < entitys_count; index++) {  // unit entity from index#1
        base::xvinentity_t* _table_unit_inentity = dynamic_cast<base::xvinentity_t*>(_table_inentitys[index]);
        base::xtable_inentity_extend_t extend;
        extend.serialize_from_string(_table_unit_inentity->get_extend_data());
        const xobject_ptr_t<base::xvheader_t> & _unit_header = extend.get_unit_header();

        xJson::Value ju;
        ju["unit_height"] = static_cast<xJson::UInt64>(_unit_header->get_height());
        ju["account"] = _unit_header->get_account();
        jv["units"].append(ju);
    }
    root["tableblock"] = jv;
}

const std::vector<xblock_ptr_t> & xtable_block_t::get_tableblock_units(bool need_parent_cert) const {
    return unpack_and_get_units(need_parent_cert);
}

void xtable_block_t::unpack_proposal_units(std::vector<xblock_ptr_t> & units) const {
    std::vector<xobject_ptr_t<base::xvblock_t>> _units = xlighttable_build_t::unpack_units_from_table(this);
    xassert(!_units.empty());
    for (auto & v : _units) {  // TODO(jimmy)
        units.push_back(dynamic_xobject_ptr_cast<xblock_t>(v));
    }
}

const std::vector<xblock_ptr_t> & xtable_block_t::unpack_and_get_units(bool need_parent_cert) const {
    if (check_block_flag(base::enum_xvblock_flag_authenticated)) {
        std::call_once(m_once_unpack_flag, [this] () {
            unpack_proposal_units(m_cache_units);
        });
    } else {
        xassert(false);
    }
    return m_cache_units;
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
    const std::vector<xblock_ptr_t> & subblocks = get_tableblock_units(true);
    xassert(!subblocks.empty());
    for (auto & v : subblocks) {
        sub_blocks.push_back(v);
    }
    return true;
}

bool xtable_block_t::extract_one_sub_block(uint32_t entity_id, const std::string & extend_cert, const std::string & extend_data, xobject_ptr_t<xvblock_t> & sub_block) {
    sub_block = xlighttable_build_t::unpack_one_unit_from_table(this, entity_id, extend_cert, extend_data);
    return sub_block != nullptr ? true : false;
}

const std::vector<xlightunit_tx_info_ptr_t> & xtable_block_t::get_txs() const {
    return get_lightunit_body().get_txs();
}

const xlightunit_body_t & xtable_block_t::get_lightunit_body() const {
    try_load_body();
    // xassert(!m_cache_body.is_empty());
    return m_cache_body;
}

void xtable_block_t::load_body() const {
    base::xvinentity_t* primary_input_entity = get_input()->get_primary_entity();
    const std::vector<base::xvaction_t> & actions = primary_input_entity->get_actions();
    for (auto & action : actions) {
        xtransaction_ptr_t raw_tx = query_raw_transaction(action.get_org_tx_hash());
        xlightunit_tx_info_ptr_t txinfo = std::make_shared<xlightunit_tx_info_t>(action, raw_tx.get());
        m_cache_body.add_tx_info(txinfo);
    }
}

void xtable_block_t::try_load_body() const {
    std::call_once(m_once_load_flag, [this] () {
        load_body();
    });
}

bool xtable_block_t::extract_sub_txs(std::vector<base::xvtxindex_ptr> & sub_txs) {
    base::xvinentity_t* primary_input_entity = get_input()->get_primary_entity();
    const std::vector<base::xvaction_t> & actions = primary_input_entity->get_actions();
    xdbg("wish action size:%zu", actions.size());
    for (auto & action : actions) {
        if (!action.get_org_tx_hash().empty()) {
            xtransaction_ptr_t raw_tx = query_raw_transaction(action.get_org_tx_hash());
            xdbg("wish unpack tx_hash:%s, account: %s, height:%llu, type:%d", data::to_hex_str(action.get_org_tx_hash()).c_str(), get_account().c_str(), get_height(), action.get_org_tx_action_id());
            base::xvtxindex_ptr tx_index = make_object_ptr<base::xvtxindex_t>(*this, dynamic_cast<xdataunit_t*>(raw_tx.get()), action.get_org_tx_hash(), static_cast<enum_transaction_subtype>(action.get_org_tx_action_id()));
            sub_txs.push_back(tx_index);
        }
    }

    return true;
}

NS_END2
