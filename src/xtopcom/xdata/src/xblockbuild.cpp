// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xdata/xblockbuild.h"
#include "xdata/xrootblock.h"
#include "xdata/xemptyblock.h"
#include "xdata/xlightunit.h"
#include "xdata/xfullunit.h"
#include "xdata/xtableblock.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xblockaction.h"
#include "xvledger/xventity.h"
#include "xvledger/xvaction.h"
#include "xvledger/xvcontract.h"

NS_BEG2(top, data)

XINLINE_CONSTEXPR char const * BLD_URI_LIGHT_TABLE      = "b_lt//"; //xvcontract_t::create_contract_uri(b_lt, {}, 0)
XINLINE_CONSTEXPR char const * BLD_URI_FULL_TABLE       = "b_ft//"; //xvcontract_t::create_contract_uri(b_ft, {}, 0)
XINLINE_CONSTEXPR char const * BLD_URI_LIGHT_UNIT       = "b_lu//"; //xvcontract_t::create_contract_uri(b_lu, {}, 0)
XINLINE_CONSTEXPR char const * BLD_URI_FULL_UNIT        = "b_fu//"; //xvcontract_t::create_contract_uri(b_fu, {}, 0)
XINLINE_CONSTEXPR char const * BLD_URI_ROOT_BLOCK       = "b_rb//"; //xvcontract_t::create_contract_uri(b_rb, {}, 0)

base::xvaction_t make_block_build_action(const std::string & target_uri, const std::map<std::string, std::string> & action_result = {}) {
    std::string caller_addr;  // empty means version0, no caller addr
    // std::string contract_addr;
    // std::string contract_name; // empty means version0, default contract
    // uint32_t    contract_version = 0;
    // std::string target_uri = xvcontract_t::create_contract_uri(contract_addr, contract_name, contract_version);
    std::string method_name = "b";
    std::string tx_hash;

    base::xvaction_t _tx_action(tx_hash, caller_addr, target_uri, method_name);
    if (!action_result.empty()) {
        base::xvalue_t _action_result(action_result);
        _tx_action.copy_result(_action_result);
    }
    xassert(!_tx_action.get_method_uri().empty());
    return _tx_action;
}

base::xvaction_t make_table_block_action_with_table_prop_prove(const std::string & target_uri, uint32_t block_version, const std::map<std::string, std::string> & property_hashs, base::xtable_shortid_t tableid, uint64_t height) {
    if (base::xvblock_fork_t::is_block_match_version(block_version, base::enum_xvblock_fork_version_table_prop_prove)) {
        // xassert(!para.get_property_hashs().empty());  // XTODO maybe empty for only self txs
        xassert(height > 0);
        xtableblock_action_t _action(target_uri, property_hashs, tableid, height);       
        return static_cast<base::xvaction_t>(_action);
    } else {
        base::xvaction_t _action = make_block_build_action(target_uri);
        return _action;
    }
}

std::string get_header_extra(const xlightunit_block_para_t & bodypara) {
    base::xvtxkey_vec_t txs;
    for (auto & tx : bodypara.get_input_txs()) {
        base::xvtxkey_t tx_key(tx->get_tx_hash(), tx->get_tx_subtype());
        txs.push_back(tx_key);
        xdbg("xlightunit_build_t::get_header_extra tx hash:%s, type:%s", tx->get_digest_hex_str().c_str(), tx->get_tx_subtype_str().c_str());
    }
    std::string str;
    txs.serialize_to_string(str);

    base::xvheader_extra he;
    he.insert(base::HEADER_KEY_TXS, str);
    std::string he_str;
    he.serialize_to_string(he_str);
    return he_str;
}

base::xvaction_t make_action(const xcons_transaction_ptr_t & tx) {
    std::string caller_addr;  // empty means version0, no caller addr
    std::string contract_addr = tx->get_account_addr();
    std::string contract_name; // empty means version0, default contract
    uint32_t    contract_version = 0;
    std::string target_uri = base::xvcontract_t::create_contract_uri(contract_addr, contract_name, contract_version);
    std::string method_name = xtransaction_t::transaction_type_to_string(tx->get_tx_type());
    if (tx->is_recv_tx()) {  // set origin tx source addr for recv tx, confirm tx need know source addr without origin tx
        caller_addr = tx->get_source_addr();
    }

    base::xvalue_t _action_result(tx->get_tx_execute_state().get_map_para());  // how to set result
    base::xvaction_t _tx_action(tx->get_tx_hash(), caller_addr, target_uri, method_name);
    _tx_action.set_org_tx_action_id(tx->get_tx_subtype());
    _tx_action.copy_result(_action_result);
    return _tx_action;
}

xlightunit_tx_info_ptr_t build_tx_info(const xcons_transaction_ptr_t & tx) {
    base::xvaction_t _action = data::make_action(tx);
    xlightunit_tx_info_ptr_t txinfo = std::make_shared<xlightunit_tx_info_t>(_action, tx->get_transaction());
    return txinfo;
}

bool xlightunit_build_t::should_build_unit_opt(const uint64_t clock, const uint64_t height) {
    // genesis block never fork
    if (height > 0 && base::xvblock_fork_t::instance().is_forked(clock)) {
        return true;
    }
    return false;
}

xlightunit_build_t::xlightunit_build_t(const std::string & account, const xlightunit_block_para_t & bodypara) {
    base::xbbuild_para_t build_para(xrootblock_t::get_rootblock_chainid(), account, base::enum_xvblock_level_unit, base::enum_xvblock_class_light, xrootblock_t::get_rootblock_hash());
    if (should_build_unit_opt(build_para.get_clock(), build_para.get_height())) {
        build_para.set_extra_data(get_header_extra(bodypara));
    }
    init_header_qcert(build_para);
    build_block_body(bodypara);
}

xlightunit_build_t::xlightunit_build_t(base::xvblock_t* prev_block, const xlightunit_block_para_t & bodypara, const xblock_consensus_para_t & para) {
    base::xbbuild_para_t build_para(prev_block, base::enum_xvblock_class_light, base::enum_xvblock_type_txs);
    build_para.set_unit_cert_para(para.get_clock(), para.get_viewtoken(), para.get_viewid(), para.get_validator(), para.get_auditor(),
                                  para.get_drand_height(), para.get_parent_height(), para.get_justify_cert_hash());
    if (should_build_unit_opt(build_para.get_clock(), build_para.get_height())) {
        build_para.set_extra_data(get_header_extra(bodypara));
    }
    init_header_qcert(build_para);
    build_block_body(bodypara);
}
xlightunit_build_t::xlightunit_build_t(base::xvheader_t* header, base::xvinput_t* input, base::xvoutput_t* output)
: base::xvblockmaker_t(header, input, output) {

}

bool xlightunit_build_t::build_block_body(const xlightunit_block_para_t & para) {
    // #1 set input entitys and resources
    std::vector<base::xvaction_t> input_actions;
    if (base::xvblock_fork_t::is_block_older_version(get_header()->get_block_version(), base::enum_xvblock_fork_version_unit_opt)) {
        xdbg("block version:%d, height:%llu, account:%s", get_header()->get_block_version(), get_header()->get_height(), get_header()->get_account().c_str());
        for (auto & tx : para.get_input_txs()) {
            base::xvaction_t _action = make_action(tx);
            input_actions.push_back(_action);
        }
        set_input_entity(input_actions);

        for (auto & tx : para.get_input_txs()) {
            // confirm tx no need take origintx
            if (tx->is_self_tx() || tx->is_send_tx()) {
                std::string origintx_bin;
                tx->get_transaction()->serialize_to_string(origintx_bin);
                std::string origintx_hash = tx->get_tx_hash();
                set_input_resource(origintx_hash, origintx_bin);
            }
        }

        // key_name_unconfirm_tx_count will be unusable after block 2.0.0
        uint32_t unconfirm_tx_num = para.get_account_unconfirm_sendtx_num();
        std::string unconfirm_tx_num_str = base::xstring_utl::tostring(unconfirm_tx_num);
        set_output_entity(base::xvoutentity_t::key_name_unconfirm_tx_count(), unconfirm_tx_num_str);
    } else {
        xdbg("block version:%d, height:%llu, account:%s", get_header()->get_block_version(), get_header()->get_height(), get_header()->get_account().c_str());
        base::xvaction_t _action = make_block_build_action(BLD_URI_LIGHT_UNIT);
        input_actions.push_back(_action);
        set_input_entity(input_actions);
    }

    // #2 set output entitys and resources
    set_output_full_state(para.get_fullstate_bin());
    set_output_binlog(para.get_property_binlog());
    return true;
}

base::xauto_ptr<base::xvblock_t> xlightunit_build_t::create_new_block() {
    return new xlightunit_block_t(*get_header(), *get_qcert(), get_input(), get_output());
}


xemptyblock_build_t::xemptyblock_build_t(const std::string & account) {
    base::enum_xvblock_level _level = get_block_level_from_account(account);
    base::xbbuild_para_t build_para(xrootblock_t::get_rootblock_chainid(), account, _level, base::enum_xvblock_class_nil, xrootblock_t::get_rootblock_hash());
    init_header_qcert(build_para);
}
xemptyblock_build_t::xemptyblock_build_t(const std::string & tc_account, uint64_t _tc_height) {
    xassert(_tc_height != 0);
    base::enum_xvblock_level _level = get_block_level_from_account(tc_account);
    base::enum_xvblock_type _type = get_block_type_from_empty_block(tc_account);
    base::xbbuild_para_t build_para(xrootblock_t::get_rootblock_chainid(), tc_account, _tc_height, base::enum_xvblock_class_nil, _level, _type, xrootblock_t::get_rootblock_hash(), xrootblock_t::get_rootblock_hash());
    xvip2_t _validator;
    set_empty_xip2(_validator);  // must change for block later
    build_para.set_basic_cert_para(_tc_height, -1, _tc_height, _validator);
    init_header_qcert(build_para);
}

xemptyblock_build_t::xemptyblock_build_t(base::xvblock_t* prev_block, const xblock_consensus_para_t & para) {
    base::enum_xvblock_type _type = get_block_type_from_empty_block(prev_block->get_account());
    base::xbbuild_para_t build_para(prev_block, base::enum_xvblock_class_nil, _type);
    if (prev_block->get_block_level() == base::enum_xvblock_level_unit) {
        build_para.set_unit_cert_para(para.get_clock(), para.get_viewtoken(), para.get_viewid(), para.get_validator(), para.get_auditor(),
                                    para.get_drand_height(), para.get_parent_height(), para.get_justify_cert_hash());
    } else if (prev_block->get_block_level() == base::enum_xvblock_level_table) {
        build_para.set_table_cert_para(para.get_clock(), para.get_viewtoken(), para.get_viewid(), para.get_validator(), para.get_auditor(),
                                    para.get_drand_height(), para.get_justify_cert_hash());
    } else {
        build_para.set_basic_cert_para(para.get_clock(), para.get_viewtoken(), para.get_viewid(), para.get_validator());
    }
    init_header_qcert(build_para);
}

xemptyblock_build_t::xemptyblock_build_t(base::xvblock_t* prev_block) {
    base::xbbuild_para_t build_para(prev_block, base::enum_xvblock_class_nil, base::enum_xvblock_type_general);
    // set for test
    build_para.set_basic_cert_para(prev_block->get_clock()+1, prev_block->get_viewtoken()+1, prev_block->get_viewid(), xvip2_t{(uint64_t)1, (uint64_t)1});
    init_header_qcert(build_para);
}
xemptyblock_build_t::xemptyblock_build_t(base::xvheader_t* header)
: base::xvblockmaker_t(header) {
}

base::xauto_ptr<base::xvblock_t> xemptyblock_build_t::create_new_block() {
    return new xemptyblock_t(*get_header(), *get_qcert());
}

xfullunit_build_t::xfullunit_build_t(base::xvblock_t* prev_block, const xfullunit_block_para_t & bodypara, const xblock_consensus_para_t & para) {
    base::xbbuild_para_t build_para(prev_block, base::enum_xvblock_class_full, base::enum_xvblock_type_general);
    build_para.set_unit_cert_para(para.get_clock(), para.get_viewtoken(), para.get_viewid(), para.get_validator(), para.get_auditor(),
                                    para.get_drand_height(), para.get_parent_height(), para.get_justify_cert_hash());

    if (xlightunit_build_t::should_build_unit_opt(build_para.get_clock(), build_para.get_height())) {
        build_para.set_extra_data(get_header_extra(bodypara));
    }
    
    init_header_qcert(build_para);
    build_block_body(bodypara);
}

xfullunit_build_t::xfullunit_build_t(base::xvheader_t* header, base::xvinput_t* input, base::xvoutput_t* output)
: base::xvblockmaker_t(header, input, output) {

}

bool xfullunit_build_t::build_block_body(const xfullunit_block_para_t & para) {
    // #1 set input entitys and resources
    base::xvaction_t _action = make_block_build_action(BLD_URI_FULL_UNIT);
    set_input_entity(_action);

    xdbg("fullunit block version:%d, height:%llu, account:%s", get_header()->get_block_version(), get_header()->get_height(), get_header()->get_account().c_str());
    // #2 set output entitys and resources
    std::string full_state_bin = para.get_fullstate_bin();
    set_output_full_state(full_state_bin);

    return true;
}

base::xauto_ptr<base::xvblock_t> xfullunit_build_t::create_new_block() {
    return new xfullunit_block_t(*get_header(), *get_qcert(), get_input(), get_output());
}


xlighttable_build_t::xlighttable_build_t(base::xvblock_t* prev_block, const xtable_block_para_t & bodypara, const xblock_consensus_para_t & para) {
    base::xbbuild_para_t build_para(prev_block, base::enum_xvblock_class_light, base::enum_xvblock_type_batch);
    xassert(!bodypara.get_extra_data().empty());
    build_para.set_extra_data(bodypara.get_extra_data()); // only light-table need extra data
    build_para.set_table_cert_para(para.get_clock(), para.get_viewtoken(), para.get_viewid(), para.get_validator(), para.get_auditor(),
                                    para.get_drand_height(), para.get_justify_cert_hash());
    base::xvaccount_t _vaccount(prev_block->get_account());
    init_header_qcert(build_para);
    build_block_body(bodypara, _vaccount, prev_block->get_height() + 1);
}

bool xlighttable_build_t::build_block_body(const xtable_block_para_t & para, const base::xvaccount_t & account, uint64_t height) {
    // #1 set input entitys and resources
    base::xvaction_t _action = make_table_block_action_with_table_prop_prove(BLD_URI_LIGHT_TABLE, get_header()->get_block_version(), para.get_property_hashs(), account.get_short_table_id(), height);
    xdbg("xlighttable_build_t::build_block_body. account=%s,height=%ld,version=%ld,tx size:%zu", account.get_account().c_str(), height, get_header()->get_block_version(), para.get_txs().size());
    if (base::xvblock_fork_t::is_block_older_version(get_header()->get_block_version(), base::enum_xvblock_fork_version_unit_opt)) {
        set_input_entity(_action);
    } else {
        std::vector<base::xvaction_t> input_actions;
        input_actions.push_back(_action);
        for(auto & tx : para.get_txs()) {
            input_actions.push_back(*tx.get());
        }
        set_input_entity(input_actions);

        for (auto & tx : para.get_txs()) {
            // confirm tx no need take origintx
            if (tx->is_self_tx() || tx->is_send_tx()) {
                std::string origintx_bin;
                tx->get_raw_tx()->serialize_to_string(origintx_bin);
                std::string origintx_hash = tx->get_tx_hash();
                set_input_resource(origintx_hash, origintx_bin);
            }
        }
    }

    std::vector<xobject_ptr_t<base::xvblock_t>> batch_units;
    for (auto & v : para.get_account_units()) {
        batch_units.push_back(v);
    }
    set_batch_units(batch_units);

    // #2 set output entitys and resources
    std::string binlog = para.get_property_binlog();
    set_output_binlog(binlog);
    std::string full_state = para.get_fullstate_bin();
    set_output_full_state(full_state);
    std::string tgas_balance_change = base::xstring_utl::tostring(para.get_tgas_balance_change());
    set_output_entity(base::xvoutentity_t::key_name_tgas_pledge_change(), tgas_balance_change);
    return true;
}

base::xauto_ptr<base::xvinput_t> xlighttable_build_t::make_unit_input_from_table(const base::xvblock_t* _tableblock, const base::xtable_inentity_extend_t & extend, base::xvinentity_t* _table_unit_inentity) {
    const std::vector<base::xvaction_t> &  input_actions = _table_unit_inentity->get_actions();
    // make unit input entity by table input entity
    base::xauto_ptr<base::xvinentity_t> _unit_inentity = new base::xvinentity_t(input_actions);
    std::vector<base::xventity_t*> _unit_inentitys;
    _unit_inentitys.push_back(_unit_inentity.get());

    // query unit input resource from table and make unit resource
    base::xauto_ptr<base::xstrmap_t> _strmap = new base::xstrmap_t();
    for (auto & action : input_actions) {
        if (!action.get_org_tx_hash().empty()) {
            std::string _org_tx = _tableblock->get_input()->query_resource(action.get_org_tx_hash());
            if (_org_tx.empty()) {
                // confirm tx not has origin tx
                base::enum_transaction_subtype _subtype = (base::enum_transaction_subtype)action.get_org_tx_action_id();
                if (_subtype == base::enum_transaction_subtype_send) { // sendtx must has origin tx
                    xassert(false);
                    return nullptr;
                }
            }
            _strmap->set(action.get_org_tx_hash(), _org_tx);
        }
    }

    base::xauto_ptr<base::xvinput_t> _unit_input = make_object_ptr<base::xvinput_t>(_unit_inentitys, *_strmap.get());
    return _unit_input;
}

base::xauto_ptr<base::xvoutput_t> xlighttable_build_t::make_unit_output_from_table(const base::xvblock_t* _tableblock, const base::xtable_inentity_extend_t & extend, base::xvoutentity_t* _table_unit_outentity) {
    // make unit output entity by table output entity
    base::xauto_ptr<base::xvoutentity_t> _unit_outentity = new base::xvoutentity_t(*_table_unit_outentity);
    std::vector<base::xventity_t*> _unit_outentitys;
    _unit_outentitys.push_back(_unit_outentity.get());

    // query unit input resource from table and make unit resource
    base::xauto_ptr<base::xstrmap_t> _strmap = new base::xstrmap_t();
    if (extend.get_unit_header()->get_block_class() == base::enum_xvblock_class_light) {
        std::string _binlog = _tableblock->get_output()->query_resource(_unit_outentity->get_binlog_hash());
        if (!_binlog.empty()) {
            _strmap->set(_unit_outentity->get_binlog_hash(), _binlog);
        } else {
            xassert(false);
            return nullptr;  // lightunit must has binlog
        }
    }
    if (extend.get_unit_header()->get_block_class() == base::enum_xvblock_class_full) {
        std::string _state = _tableblock->get_output()->query_resource(extend.get_unit_output_root_hash());
        if (!_state.empty()) {
            _strmap->set(extend.get_unit_output_root_hash(), _state);
        } else {
            xassert(false);
            return nullptr;  // fullunit must has fullstate
        }
    }
    xobject_ptr_t<base::xvoutput_t> _unit_output = make_object_ptr<base::xvoutput_t>(_unit_outentitys, *_strmap.get());
    _unit_output->set_root_hash(extend.get_unit_output_root_hash());
    return _unit_output;
}

xobject_ptr_t<base::xvblock_t> xlighttable_build_t::unpack_one_unit_from_table(const base::xvblock_t* _tableblock, uint32_t entity_id, const std::string & extend_cert, const std::string & extend_data) {
    XMETRICS_GAUGE(metrics::data_table_unpack_one_unit, 1);
    const std::vector<base::xventity_t*> & _table_inentitys = _tableblock->get_input()->get_entitys();
    const std::vector<base::xventity_t*> & _table_outentitys = _tableblock->get_output()->get_entitys();
    if (_table_inentitys.size() <= entity_id || entity_id == 0 || _table_inentitys.size() != _table_outentitys.size()) {
        xassert(false);
        return nullptr;
    }
    base::xvinentity_t* _table_unit_inentity = dynamic_cast<base::xvinentity_t*>(_table_inentitys[entity_id]);
    base::xvoutentity_t* _table_unit_outentity = dynamic_cast<base::xvoutentity_t*>(_table_outentitys[entity_id]);
    if (_table_unit_inentity == nullptr || _table_unit_outentity == nullptr) {
        xassert(false);
        return nullptr;
    }

    base::xtable_inentity_extend_t extend;
    extend.serialize_from_string(_table_unit_inentity->get_extend_data());
    const xobject_ptr_t<base::xvheader_t> & _unit_header = extend.get_unit_header();

    std::shared_ptr<base::xvblockmaker_t> vbmaker = nullptr;
    if (_unit_header->get_block_class() == base::enum_xvblock_class_nil) {
        vbmaker = std::make_shared<xemptyblock_build_t>(_unit_header.get());
    } else {
        base::xauto_ptr<base::xvinput_t> _unit_input = make_unit_input_from_table(_tableblock, extend, _table_unit_inentity);
        base::xauto_ptr<base::xvoutput_t> _unit_output = make_unit_output_from_table(_tableblock, extend, _table_unit_outentity);
        if (_unit_input == nullptr || _unit_output == nullptr) {
            xassert(false);
            return nullptr;
        }
        if (_unit_header->get_block_class() == base::enum_xvblock_class_light) {
            vbmaker = std::make_shared<xlightunit_build_t>(_unit_header.get(), _unit_input.get(), _unit_output.get());
        } else if (_unit_header->get_block_class() == base::enum_xvblock_class_full) {
            vbmaker = std::make_shared<xfullunit_build_t>(_unit_header.get(), _unit_input.get(), _unit_output.get());
        }
    }

    base::xbbuild_para_t build_para;
    base::xvqcert_t* tablecert = _tableblock->get_cert();
    build_para.set_unit_cert_para(tablecert->get_clock(), tablecert->get_viewtoken(), tablecert->get_viewid(), tablecert->get_validator(),
                                tablecert->get_auditor(), tablecert->get_drand_height(), _tableblock->get_height(), extend.get_unit_justify_hash());
    vbmaker->init_qcert(build_para);
    xobject_ptr_t<base::xvblock_t> _unit = vbmaker->build_new_block();
    xassert(_unit != nullptr);
    _unit->set_parent_block(_tableblock->get_account(), entity_id);
    _unit->get_cert()->set_parent_height(_tableblock->get_height());
    _unit->get_cert()->set_parent_viewid(_tableblock->get_viewid());

    _unit->set_extend_cert(extend_cert);
    _unit->set_extend_data(extend_data);

    _unit->set_block_flag(base::enum_xvblock_flag_authenticated);
    xassert(!_unit->get_block_hash().empty());
    return _unit;
}

std::vector<xobject_ptr_t<base::xvblock_t>> xlighttable_build_t::unpack_units_from_table(const base::xvblock_t* _tableblock) {
    XMETRICS_GAUGE(metrics::data_table_unpack_units, 1);
#ifdef DEBUG    
    if (!_tableblock->is_input_ready(true)
        || !_tableblock->is_output_ready(true)) {
        xerror("xlighttable_build_t::unpack_units_from_table not ready block. block=%s", _tableblock->dump().c_str());
        return {};
    }
#endif
    base::xvaccount_t _vtable_addr(_tableblock->get_account());
    std::vector<xobject_ptr_t<base::xvblock_t>> _batch_units;

    const std::vector<base::xventity_t*> & _table_inentitys = _tableblock->get_input()->get_entitys();
    const std::vector<base::xventity_t*> & _table_outentitys = _tableblock->get_output()->get_entitys();
    if (_table_inentitys.size() <= 0 || _table_inentitys.size() != _table_outentitys.size()) {
        xassert(false);
        return {};
    }
    uint32_t entitys_count = _table_inentitys.size();
    for (uint32_t index = 1; index < entitys_count; index++) {  // unit entity from index#1
        base::xvinentity_t* _table_unit_inentity = dynamic_cast<base::xvinentity_t*>(_table_inentitys[index]);
        base::xvoutentity_t* _table_unit_outentity = dynamic_cast<base::xvoutentity_t*>(_table_outentitys[index]);
        if (_table_unit_inentity == nullptr || _table_unit_outentity == nullptr) {
            xassert(false);
            return {};
        }

        base::xtable_inentity_extend_t extend;
        extend.serialize_from_string(_table_unit_inentity->get_extend_data());
        const xobject_ptr_t<base::xvheader_t> & _unit_header = extend.get_unit_header();

        std::shared_ptr<base::xvblockmaker_t> vbmaker = nullptr;
        if (_unit_header->get_block_class() == base::enum_xvblock_class_nil) {
            vbmaker = std::make_shared<xemptyblock_build_t>(_unit_header.get());
        } else {
            base::xauto_ptr<base::xvinput_t> _unit_input = make_unit_input_from_table(_tableblock, extend, _table_unit_inentity);
            base::xauto_ptr<base::xvoutput_t> _unit_output = make_unit_output_from_table(_tableblock, extend, _table_unit_outentity);
            if (_unit_input == nullptr || _unit_output == nullptr) {
                xassert(false);
                return {};
            }
            if (_unit_header->get_block_class() == base::enum_xvblock_class_light) {
                vbmaker = std::make_shared<xlightunit_build_t>(_unit_header.get(), _unit_input.get(), _unit_output.get());
            } else if (_unit_header->get_block_class() == base::enum_xvblock_class_full) {
                vbmaker = std::make_shared<xfullunit_build_t>(_unit_header.get(), _unit_input.get(), _unit_output.get());
            }
        }

        base::xbbuild_para_t build_para;
        base::xvqcert_t* tablecert = _tableblock->get_cert();
        build_para.set_unit_cert_para(tablecert->get_clock(), tablecert->get_viewtoken(), tablecert->get_viewid(), tablecert->get_validator(),
                                    tablecert->get_auditor(), tablecert->get_drand_height(), _tableblock->get_height(), extend.get_unit_justify_hash());
        vbmaker->init_qcert(build_para);
        xobject_ptr_t<base::xvblock_t> _unit = vbmaker->build_new_block();
        xassert(_unit != nullptr);
        _unit->set_parent_block(_vtable_addr.get_account(), index);
        _unit->get_cert()->set_parent_height(_tableblock->get_height());
        _unit->get_cert()->set_parent_viewid(_tableblock->get_viewid());

        _batch_units.push_back(_unit);
    }

    base::xvtableblock_maker_t::units_set_parent_cert(_batch_units, _tableblock);
#ifdef DEBUG
    for (auto & v : _batch_units) {
        xassert(!v->get_cert()->get_extend_cert().empty());
        xassert(!v->get_cert()->get_extend_data().empty());
    }
#endif
    return _batch_units;
}

base::xauto_ptr<base::xvblock_t> xlighttable_build_t::create_new_block() {
    return new xtable_block_t(*get_header(), *get_qcert(), get_input(), get_output());
}

xfulltable_build_t::xfulltable_build_t(base::xvblock_t* prev_block, const xfulltable_block_para_t & bodypara, const xblock_consensus_para_t & para) {
    base::xbbuild_para_t build_para(prev_block, base::enum_xvblock_class_full, base::enum_xvblock_type_general);
    build_para.set_table_cert_para(para.get_clock(), para.get_viewtoken(), para.get_viewid(), para.get_validator(), para.get_auditor(),
                                    para.get_drand_height(), para.get_justify_cert_hash());
    base::xvaccount_t _vaccount(prev_block->get_account());
    init_header_qcert(build_para);
    build_block_body(bodypara, _vaccount, prev_block->get_height() + 1);
}

bool xfulltable_build_t::build_block_body(const xfulltable_block_para_t & para, const base::xvaccount_t & account, uint64_t height) {
    // #1 set input entitys and resources
    base::xvaction_t _action = make_table_block_action_with_table_prop_prove(BLD_URI_FULL_TABLE, get_header()->get_block_version(), para.get_property_hashs(), account.get_short_table_id(), height);
    set_input_entity(_action);
    xdbg("xfulltable_build_t::build_block_body，account=%s,height=%ld,version=0x%x", account.get_account().c_str(), height, get_header()->get_block_version());
    // #2 set output entitys and resources
    std::string full_state_bin = para.get_snapshot();
    set_output_full_state(full_state_bin);

    std::string tgas_balance_change = base::xstring_utl::tostring(para.get_tgas_balance_change());
    set_output_entity(base::xvoutentity_t::key_name_tgas_pledge_change(), tgas_balance_change);

    const xstatistics_data_t & statistics_data = para.get_block_statistics_data();
    auto const & serialized_data = statistics_data.serialize_based_on<base::xstream_t>();
    std::string serialized_data_str = {std::begin(serialized_data), std::end(serialized_data) };
    set_input_resource(xfull_tableblock_t::RESOURCE_NODE_SIGN_STATISTICS, serialized_data_str);
    return true;
}

base::xauto_ptr<base::xvblock_t> xfulltable_build_t::create_new_block() {
    return new xfull_tableblock_t(*get_header(), *get_qcert(), get_input(), get_output());
}


xrootblock_build_t::xrootblock_build_t(base::enum_xchain_id chainid, const std::string & account, const xrootblock_para_t & bodypara) {
    // rootblock last block hash is empty
    base::xbbuild_para_t build_para(chainid, account, base::enum_xvblock_level_chain, base::enum_xvblock_class_light, std::string());
    init_header_qcert(build_para);
    build_block_body(bodypara);
}

bool xrootblock_build_t::build_block_body(const xrootblock_para_t & para) {
    // #1 set input entitys and resources
    base::xvaction_t _action = make_block_build_action(BLD_URI_ROOT_BLOCK);
    set_input_entity(_action);

    xobject_ptr_t<xrootblock_input_t> input = make_object_ptr<xrootblock_input_t>();
    input->set_account_balances(para.m_account_balances);
    input->set_genesis_funds_accounts(para.m_geneis_funds_accounts);
    input->set_genesis_tcc_accounts(para.m_tcc_accounts);
    input->set_genesis_nodes(para.m_genesis_nodes);
    // TODO(jimmy) use bstate
    std::string resource_bin;
    input->serialize_to_string(resource_bin);
    // set_input_resource(xrootblock_t::root_resource_name, resource_bin);
    // #3 set output entitys and resources

    // std::string binlog = "1"; // TODO(jimmy) just for rules
    // set_output_binlog(binlog);
    // std::string full_state = "2"; // TODO(jimmy) just for rules not same hash with binlog
    // set_output_full_state(full_state);


    xobject_ptr_t<base::xvbstate_t> bstate = make_object_ptr<base::xvbstate_t>(*get_header());
    xobject_ptr_t<base::xvcanvas_t> canvas = make_object_ptr<base::xvcanvas_t>();
    {
        auto propobj = bstate->new_string_var(xrootblock_t::ROOT_BLOCK_PROPERTY_NAME, canvas.get());
        propobj->reset(resource_bin, canvas.get());
    }

    std::string property_binlog;
    canvas->encode(property_binlog);
    std::string fullstate_bin;
    bstate->take_snapshot(fullstate_bin);
    set_output_binlog(property_binlog);
    set_output_full_state(fullstate_bin);
    return true;
}

base::xauto_ptr<base::xvblock_t> xrootblock_build_t::create_new_block() {
    return new xrootblock_t(*get_header(), *get_qcert(), get_input(), get_output());
}

NS_END2
