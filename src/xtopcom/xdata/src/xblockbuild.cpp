// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xdata/xblockbuild.h"
#include "xdata/xrootblock.h"
#include "xdata/xblockbody.h"
#include "xdata/xemptyblock.h"
#include "xdata/xlightunit.h"
#include "xdata/xfullunit.h"
#include "xdata/xtableblock.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xblock_resources.h"
#include "xdata/xnative_contract_address.h"

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
: xvblockbuild_t(header, input, output) {
}

bool xlightunit_build_t::build_block_body(const xlightunit_block_para_t & para) {
    xblockbody_para_t blockbody;
    for (auto & tx : para.get_input_txs()) {
        enum_xunit_tx_exec_status last_action_status = enum_xunit_tx_exec_status_success;
        uint64_t last_tx_clock = 0;
        if (tx->is_recv_tx() || tx->is_confirm_tx()) {
            last_action_status = tx->get_receipt()->get_tx_info()->get_tx_exec_state().get_tx_exec_status();
            last_tx_clock = tx->get_clock();
        }
        xlightunit_input_entity_ptr_t ientity = make_object_ptr<xlightunit_input_entity_t>(tx->get_tx_subtype(),
                                                                                        tx->get_transaction(),
                                                                                        false,
                                                                                        last_action_status,
                                                                                        last_tx_clock);


        blockbody.add_input_entity(ientity);
        // if (tx->is_self_tx() || tx->is_send_tx()) {  // send tx save origin tx in lightunit
        //     xobject_ptr_t<xresource_origintx_t> origintx = make_object_ptr<xresource_origintx_t>(tx->get_transaction());
        //     std::string reskey = ientity->get_origintx_resource_key();
        //     std::string resvalue;
        //     origintx->serialize_to_string(resvalue);
        //     blockbody.add_input_resource(reskey, resvalue);
        // }
    }
    for (auto & tx : para.get_contract_create_txs()) {
        xassert(tx->get_tx_subtype() == enum_transaction_subtype_send);
        enum_xunit_tx_exec_status last_action_status = enum_xunit_tx_exec_status_success;
        uint64_t last_tx_clock = 0;
        xlightunit_input_entity_ptr_t ientity = make_object_ptr<xlightunit_input_entity_t>(tx->get_tx_subtype(),
                                                                                           tx->get_transaction(),
                                                                                           true,
                                                                                           last_action_status,
                                                                                           last_tx_clock);
        blockbody.add_input_entity(ientity);
        // if (tx->is_self_tx() || tx->is_send_tx()) {  // send tx save origin tx in lightunit
        //     xobject_ptr_t<xresource_origintx_t> origintx = make_object_ptr<xresource_origintx_t>(tx->get_transaction());
        //     std::string reskey = ientity->get_origintx_resource_key();
        //     std::string resvalue;
        //     origintx->serialize_to_string(resvalue);
        //     blockbody.add_input_resource(reskey, resvalue);
        // }
    }

    for (auto & tx : para.get_input_txs()) {
        xobject_ptr_t<xlightunit_output_entity_t> tx_info = make_object_ptr<xlightunit_output_entity_t>(tx->get_tx_subtype(), tx->get_transaction(), tx->get_tx_execute_state());
        blockbody.add_output_entity(tx_info);
    }
    for (auto & tx : para.get_contract_create_txs()) {
        xobject_ptr_t<xlightunit_output_entity_t> tx_info = make_object_ptr<xlightunit_output_entity_t>(tx->get_tx_subtype(), tx->get_transaction(), tx->get_tx_execute_state());
        blockbody.add_output_entity(tx_info);
    }

    xlightunit_output_resource_ptr_t txout_resource = make_object_ptr<xlightunit_output_resource_t>(para);
    std::string out_resource_str;
    txout_resource->serialize_to_string(out_resource_str);
    blockbody.add_output_resource(xlightunit_output_resource_t::name(), out_resource_str);

    // TODO(jimmy) delete property binlog from xlightunit_output_resource_t
    blockbody.add_output_resource(base::xvoutput_t::res_binlog_key_name(), para.get_property_binlog());

    init_input(blockbody.get_input_entitys(), blockbody.get_input_resource());
    init_output(blockbody.get_output_entitys(), blockbody.get_output_resource());

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
: xvblockbuild_t(header) {
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
: xvblockbuild_t(header, input, output) {
}

bool xfullunit_build_t::build_block_body(const xfullunit_block_para_t & para) {
    xblockbody_para_t blockbody;
    xobject_ptr_t<xfullunit_input_t> input = make_object_ptr<xfullunit_input_t>(para.m_first_unit_height, para.m_first_unit_hash);
    xobject_ptr_t<xfullunit_output_t> output = make_object_ptr<xfullunit_output_t>(para.m_property_snapshot);
    blockbody.add_input_entity(input);
    blockbody.add_output_entity(output);

    // TODO(jimmy) delete output snapshot, output entity record snapshot root hash
    blockbody.add_output_resource(base::xvoutput_t::res_binlog_key_name(), para.m_property_snapshot);

    init_input(blockbody.get_input_entitys(), blockbody.get_input_resource());
    init_output(blockbody.get_output_entitys(), blockbody.get_output_resource());
    return true;
}

base::xauto_ptr<base::xvblock_t> xfullunit_build_t::create_new_block() {
    return new xfullunit_block_t(*get_header(), *get_qcert(), get_input(), get_output());
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
    xblockbody_para_t blockbody;
    uint16_t index = 0;
    uint16_t count = (uint16_t)para.get_account_units().size();
    for (uint16_t index = 0; index < count; index++) {
        const auto & unit = para.get_account_units()[index];
        xobject_ptr_t<xtableblock_input_entity_t> input_entity = make_object_ptr<xtableblock_input_entity_t>(unit.get());
        xobject_ptr_t<xtableblock_output_entity_t> output_entity = make_object_ptr<xtableblock_output_entity_t>(unit.get());
        blockbody.add_input_entity(input_entity);
        blockbody.add_output_entity(output_entity);

        xassert(unit->get_block_hash().empty());
        base::xauto_ptr<xresource_unit_input_t> input_res = new xresource_unit_input_t(unit.get());
        std::string input_res_key = std::to_string(index);
        std::string input_res_value;
        input_res->serialize_to_string(input_res_value);
        blockbody.add_input_resource(input_res_key, input_res_value);

        base::xauto_ptr<xresource_unit_output_t> output_res = new xresource_unit_output_t(unit.get());
        std::string output_res_key = std::to_string(index);
        std::string output_res_value;
        output_res->serialize_to_string(output_res_value);
        blockbody.add_output_resource(output_res_key, output_res_value);

        // base::xauto_ptr<xresource_wholeblock_t> whileblock_res = new xresource_wholeblock_t(unit.get());
        // std::string key = input_entity->get_unit_resource_key();
        // std::string res_value;
        // whileblock_res->serialize_to_string(res_value);
        // blockbody.add_input_resource(key, res_value);
    }

    std::string property_binlog = para.get_property_binlog();
    blockbody.add_output_resource(base::xvoutput_t::res_binlog_key_name(), property_binlog);  // bl = binlog

    init_input(blockbody.get_input_entitys(), blockbody.get_input_resource());
    init_output(blockbody.get_output_entitys(), blockbody.get_output_resource());
    return true;
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
    xblockbody_para_t blockbody;

    xobject_ptr_t<xdummy_entity_t> input = make_object_ptr<xdummy_entity_t>();
    blockbody.add_input_entity(input);

    const xstatistics_data_t & statistics_data = para.get_block_statistics_data();
    auto const & serialized_data = statistics_data.serialize_based_on<base::xstream_t>();
    // TODO(jimmy) where define all resouces key name
    blockbody.add_output_resource(xfull_tableblock_t::RESOURCE_NODE_SIGN_STATISTICS, {std::begin(serialized_data), std::end(serialized_data) });

    std::string snapshot_hash = para.get_snapshot_hash();
    xobject_ptr_t<xfulltable_output_entity_t> output = make_object_ptr<xfulltable_output_entity_t>(snapshot_hash);
    blockbody.add_output_entity(output);

    // TODO(jimmy) should put to primary entity
    blockbody.add_output_resource(base::xvoutput_t::res_binlog_hash_key_name(), snapshot_hash);  // TODO(jimmy) bl = binlog

    init_input(blockbody.get_input_entitys(), blockbody.get_input_resource());
    init_output(blockbody.get_output_entitys(), blockbody.get_output_resource());
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
    xblockbody_para_t blockbody;
    xobject_ptr_t<xrootblock_input_t> input = make_object_ptr<xrootblock_input_t>();
    input->set_account_balances(para.m_account_balances);
    input->set_genesis_funds_accounts(para.m_geneis_funds_accounts);
    input->set_genesis_tcc_accounts(para.m_tcc_accounts);
    input->set_genesis_nodes(para.m_genesis_nodes);
    blockbody.add_input_entity(input);
    xobject_ptr_t<xdummy_entity_t> output = make_object_ptr<xdummy_entity_t>();
    blockbody.add_output_entity(output);

    init_input(blockbody.get_input_entitys(), blockbody.get_input_resource());
    init_output(blockbody.get_output_entitys(), blockbody.get_output_resource());
    return true;
}

base::xauto_ptr<base::xvblock_t> xrootblock_build_t::create_new_block() {
    return new xrootblock_t(*get_header(), *get_qcert(), get_input(), get_output());
}

NS_END2
