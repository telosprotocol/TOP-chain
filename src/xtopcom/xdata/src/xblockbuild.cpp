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
#include "xvledger/xventity.h"
#include "xvledger/xvaction.h"

NS_BEG2(top, data)

xlightunit_build_t::xlightunit_build_t(const std::string & account, const xlightunit_block_para_t & bodypara) {
    base::xbbuild_para_t build_para(xrootblock_t::get_rootblock_chainid(), account, base::enum_xvblock_level_unit, base::enum_xvblock_class_light, xrootblock_t::get_rootblock_hash());
    init_header_qcert(build_para);
    build_block_body(bodypara);
}

xlightunit_build_t::xlightunit_build_t(base::xvblock_t* prev_block, const xlightunit_block_para_t & bodypara, const xblock_consensus_para_t & para) {
    base::xbbuild_para_t build_para(prev_block, base::enum_xvblock_class_light, base::enum_xvblock_type_txs);
    build_para.set_unit_cert_para(para.get_clock(), para.get_viewtoken(), para.get_viewid(), para.get_validator(), para.get_auditor(),
                                  para.get_drand_height(), para.get_parent_height(), para.get_justify_cert_hash());
    init_header_qcert(build_para);
    build_block_body(bodypara);
}
xlightunit_build_t::xlightunit_build_t(base::xvheader_t* header, base::xvinput_t* input, base::xvoutput_t* output)
: base::xvblockmaker_t(header, input, output) {

}

base::xvaction_t xlightunit_build_t::make_action(const xcons_transaction_ptr_t & tx) {
    std::string caller_addr;  // TODO(jimmy)
    std::string target_uri;  // TODO(jimmy)
    std::string method_uri = "v1";  // TODO(jimmy)

    if (tx->is_self_tx() || tx->is_send_tx() || tx->is_confirm_tx()) {
        caller_addr = tx->get_source_addr();
        target_uri = tx->get_target_addr();
    } else {
        caller_addr = tx->get_target_addr();
        target_uri = tx->get_source_addr();
    }
    base::xvalue_t _action_result(tx->get_tx_execute_state().get_map_para());  // how to set result
    base::xvaction_t _tx_action(tx->get_tx_hash(), caller_addr, target_uri, method_uri);
    xassert(tx->get_tx_subtype() > base::enum_transaction_subtype_invalid);
    _tx_action.set_org_tx_action_id(tx->get_tx_subtype());
    _tx_action.copy_result(_action_result);
    return _tx_action;
}

bool xlightunit_build_t::build_block_body(const xlightunit_block_para_t & para) {
    // #1 set input entitys with actions
    std::vector<base::xvaction_t> input_actions;
    for (auto & tx : para.get_input_txs()) {
        base::xvaction_t _action = make_action(tx);
        input_actions.push_back(_action);
        xassert(_action.get_org_tx_action_id() > 0);
    }
    set_input_entity(input_actions);

    // #2 set input resources
    for (auto & tx : para.get_input_txs()) {
        // TODO(jimmy) confirm tx no need take origintx
        if (tx->is_self_tx() || tx->is_send_tx() || tx->is_recv_tx()) {
            std::string origintx_bin;
            tx->get_transaction()->serialize_to_string(origintx_bin);
            std::string origintx_hash = tx->get_transaction()->get_digest_str();
            set_input_resource(origintx_hash, origintx_bin);
        }
    }
    uint32_t unconfirm_tx_num = para.get_account_unconfirm_sendtx_num();
    std::string unconfirm_tx_num_str = base::xstring_utl::tostring(unconfirm_tx_num);
    set_input_resource(xlightunit_block_t::unconfirm_tx_num_name(), unconfirm_tx_num_str); // TODO(jimmy) put unconfirm tx flag into out entity

    // #3 set output entitys with state bin and binlog bin
    set_output_entity(para.get_fullstate_bin(), para.get_property_binlog());

    // #4 set output resources with binlog_bin
    set_output_resource_binlog(para.get_property_binlog());
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
    init_header_qcert(build_para);
    build_block_body(bodypara);
}
xfullunit_build_t::xfullunit_build_t(base::xvheader_t* header, base::xvinput_t* input, base::xvoutput_t* output)
: base::xvblockmaker_t(header, input, output) {

}

bool xfullunit_build_t::build_block_body(const xfullunit_block_para_t & para) {
    // #1 set input entitys with actions
    set_input_entity({});
    // #2 set input resources
    // #3 set output entitys with state bin and binlog bin
    std::string full_state_bin = para.m_property_snapshot;
    std::string binlog_bin;  //fullunit has empty binlog
    set_output_entity(full_state_bin, binlog_bin);
    // #4 set output resources
    set_output_resource_state(para.m_property_snapshot);
    return true;
}

base::xauto_ptr<base::xvblock_t> xfullunit_build_t::create_new_block() {
    base::xauto_ptr<base::xvblock_t> _block = new xfullunit_block_t(*get_header(), *get_qcert(), get_input(), get_output());
    xassert(_block->get_output()->get_binlog_hash().empty());
    xassert(!_block->get_output()->get_state_hash().empty());
    return _block;
}


xlighttable_build_t::xlighttable_build_t(base::xvblock_t* prev_block, const xtable_block_para_t & bodypara, const xblock_consensus_para_t & para) {
    base::xbbuild_para_t build_para(prev_block, base::enum_xvblock_class_light, base::enum_xvblock_type_batch);
    xassert(!bodypara.get_extra_data().empty());
    build_para.set_extra_data(bodypara.get_extra_data()); // TODO(jimmy) only light-table need extra data
    build_para.set_table_cert_para(para.get_clock(), para.get_viewtoken(), para.get_viewid(), para.get_validator(), para.get_auditor(),
                                    para.get_drand_height(), para.get_justify_cert_hash());
    init_header_qcert(build_para);
    build_block_body(bodypara);
}

bool xlighttable_build_t::build_block_body(const xtable_block_para_t & para) {
    std::vector<xobject_ptr_t<base::xvblock_t>> batch_units;
    for (auto & v : para.get_account_units()) {
        batch_units.push_back(v);
    }
    // #1 set input entitys with actions,lighttable has no actions
    std::vector<base::xvaction_t> _vactions;
    set_input_entity(_vactions);

    // #2 set input resources
    set_batch_units(batch_units);

    // #3 set output entitys with state bin and binlog bin
    std::string full_state_bin;  // TODO(jimmy) lighttable has no fullstate hash
    std::string binlog_bin = para.get_property_binlog();
    set_output_entity(full_state_bin, binlog_bin);
    // #4 set output resources
    set_output_resource_binlog(binlog_bin);
    return true;
}

// TODO(jimmy) move to xvblockbuild
std::vector<xobject_ptr_t<base::xvblock_t>> xlighttable_build_t::unpack_units_from_table(const base::xvblock_t* _tableblock) {
    std::vector<xobject_ptr_t<base::xvblock_t>> _batch_units;

    const std::vector<base::xventity_t*> & _table_inentitys = _tableblock->get_input()->get_entitys();
    const std::vector<base::xventity_t*> & _table_outentitys = _tableblock->get_output()->get_entitys();
    xassert(_table_inentitys.size() > 1);
    xassert(_table_inentitys.size() == _table_outentitys.size());
    uint32_t entitys_count = _table_inentitys.size();
    for (uint32_t index = 1; index < entitys_count; index++) {  // unit entity from index#1
        base::xvinentity_t* _table_unit_inentity = dynamic_cast<base::xvinentity_t*>(_table_inentitys[index]);
        xassert(_table_unit_inentity != nullptr);
        base::xvoutentity_t* _table_unit_outentity = dynamic_cast<base::xvoutentity_t*>(_table_outentitys[index]);
        xassert(_table_unit_outentity != nullptr);

        base::xtable_inentity_extend_t extend;
        extend.serialize_from_string(_table_unit_inentity->get_extend_data());

        base::xtable_unit_resource_ptr_t _unit_res = base::xvtableblock_maker_t::query_unit_resource(_tableblock, index);
        xassert(_unit_res != nullptr);
        const xobject_ptr_t<base::xvheader_t> & _unit_header = extend.get_unit_header();

        std::shared_ptr<base::xvblockmaker_t> vbmaker = nullptr;
        if (_unit_header->get_block_class() == base::enum_xvblock_class_nil) {
            vbmaker = std::make_shared<xemptyblock_build_t>(_unit_header.get());
        } else {
            // unit input entity can be created by the actions of table unit input entity
            base::xauto_ptr<base::xvinentity_t> _unit_inentity = new base::xvinentity_t(_table_unit_inentity->get_actions());
            std::vector<base::xventity_t*> _unit_inentitys;
            _unit_inentitys.push_back(_unit_inentity.get());
            xobject_ptr_t<base::xvinput_t> _unit_input = make_object_ptr<base::xvinput_t>(_unit_inentitys, _unit_res->get_unit_input_resources());

            // unit output entity is same with table unit output entity
            base::xauto_ptr<base::xventity_t> _unit_outentity = new base::xvoutentity_t(*_table_unit_outentity);
            std::vector<base::xventity_t*> _unit_outentitys;
            _unit_outentitys.push_back(_unit_outentity.get());
            xobject_ptr_t<base::xvoutput_t> _unit_output = make_object_ptr<base::xvoutput_t>(_unit_outentitys, _unit_res->get_unit_output_resources());

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
        _batch_units.push_back(_unit);
    }

    base::xvtableblock_maker_t::units_set_parent_cert(_batch_units, _tableblock);
    return _batch_units;
}

base::xauto_ptr<base::xvblock_t> xlighttable_build_t::create_new_block() {
    // TODO(jimmy) use xvblock_t directly
    return new xtable_block_t(*get_header(), *get_qcert(), get_input(), get_output());
}

xfulltable_build_t::xfulltable_build_t(base::xvblock_t* prev_block, const xfulltable_block_para_t & bodypara, const xblock_consensus_para_t & para) {
    base::xbbuild_para_t build_para(prev_block, base::enum_xvblock_class_full, base::enum_xvblock_type_general);
    build_para.set_table_cert_para(para.get_clock(), para.get_viewtoken(), para.get_viewid(), para.get_validator(), para.get_auditor(),
                                    para.get_drand_height(), para.get_justify_cert_hash());
    init_header_qcert(build_para);
    build_block_body(bodypara);
}

bool xfulltable_build_t::build_block_body(const xfulltable_block_para_t & para) {
    // #1 set input entitys with actions
    set_input_entity({});
    // #2 set input resources
    // #3 set output entitys with state bin and binlog bin
    std::string full_state_bin = para.get_snapshot();
    std::string binlog_bin;  //fullunit has empty binlog
    set_output_entity(full_state_bin, binlog_bin);
    // #4 set output resources
    set_output_resource_state(full_state_bin);

    const xstatistics_data_t & statistics_data = para.get_block_statistics_data();
    auto const & serialized_data = statistics_data.serialize_based_on<base::xstream_t>();
    // TODO(jimmy) where define all resouces key name
    set_input_resource(xfull_tableblock_t::RESOURCE_NODE_SIGN_STATISTICS, {std::begin(serialized_data), std::end(serialized_data) });

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
    // #1 set input entitys with actions
    set_input_entity({});
    // #2 set input resources
    xobject_ptr_t<xrootblock_input_t> input = make_object_ptr<xrootblock_input_t>();
    input->set_account_balances(para.m_account_balances);
    input->set_genesis_funds_accounts(para.m_geneis_funds_accounts);
    input->set_genesis_tcc_accounts(para.m_tcc_accounts);
    input->set_genesis_nodes(para.m_genesis_nodes);

    std::string resource_bin;
    input->serialize_to_string(resource_bin);
    set_input_resource(xrootblock_t::root_resource_name, resource_bin);
    // #3 set output entitys with state bin and binlog bin
    std::string full_state_bin;
    std::string binlog_bin;  //fullunit has empty binlog
    set_output_entity(full_state_bin, binlog_bin);
    // #4 set output resources

    return true;
}

base::xauto_ptr<base::xvblock_t> xrootblock_build_t::create_new_block() {
    return new xrootblock_t(*get_header(), *get_qcert(), get_input(), get_output());
}

NS_END2
