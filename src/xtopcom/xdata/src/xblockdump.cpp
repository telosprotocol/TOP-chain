// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <iostream>

#include "xbasic/xhex.h"
#include "xdata/xblockdump.h"
#include "xdata/xblockextract.h"

NS_BEG2(top, data)

void  xblockdump_t::dump_action(std::string const& prefix_str, base::xvaction_t const& action) {
    base::xstream_t _stream(base::xcontext_t::instance());
    action.serialize_to(_stream);

    std::cout << prefix_str << "action_caller:" << action.get_caller() 
    << " uri:" << action.get_contract_uri() 
    << " txhash:" << top::to_hex_prefixed(action.get_org_tx_hash()) 
    << " id:" << (uint32_t)action.get_org_tx_action_id() 
    << " size:" << _stream.size() << std::endl;
}

void  xblockdump_t::dump_input_entity(std::string const& prefix_str, base::xvinentity_t* entity) {
    base::xstream_t _stream(base::xcontext_t::instance());
    entity->serialize_to(_stream);

    base::xvinentity_t* _inentity = dynamic_cast<base::xvinentity_t*>(entity);
    std::cout << prefix_str << "actions_count:" << _inentity->get_actions().size() 
    << " entity_size:" << _stream.size() 
    << " extend_data_size:" << _inentity->get_extend_data().size() << std::endl;
}

void  xblockdump_t::dump_output_entity(std::string const& prefix_str, base::xvoutentity_t* entity) {
    base::xstream_t _stream(base::xcontext_t::instance());
    entity->serialize_to(_stream);

    std::cout << prefix_str << "values_count:" << entity->get_values().size() << " entity_size:" << _stream.size() << std::endl;
    for (auto & v : entity->get_values()) {
        std::cout << prefix_str << prefix_str << "key:" << v.first << " value:" << top::to_hex_prefixed(v.second) << std::endl;
    }
}

void  xblockdump_t::dump_input_entitys(base::xvblock_t* block) {
    const std::vector<base::xventity_t*> & _entitys = block->get_input()->get_entitys();
    uint32_t _input_entitys_count = _entitys.size();
    std::cout << "input_entitys_count:" << _input_entitys_count << std::endl;
    for (uint32_t index = 0; index < _input_entitys_count; index++) {
        base::xvinentity_t* _inentity = dynamic_cast<base::xvinentity_t*>(_entitys[index]);
        dump_input_entity("--", _inentity);
        for (auto & action : _inentity->get_actions()) {
            dump_action("----", action);
        }
    }
}

void  xblockdump_t::dump_output_entitys(base::xvblock_t* block) {
    const std::vector<base::xventity_t*> & _entitys = block->get_output()->get_entitys();
    uint32_t _input_entitys_count = _entitys.size();
    std::cout << "output_entitys_count:" << _input_entitys_count << std::endl;
    for (uint32_t index = 0; index < _input_entitys_count; index++) {
        base::xvoutentity_t* _inentity = dynamic_cast<base::xvoutentity_t*>(_entitys[index]);
        dump_output_entity("--", _inentity);
    }
}

void  xblockdump_t::dump_input_resources(base::xvblock_t* block) {
    auto resources = block->get_input()->get_resources();
    const std::map<std::string, std::string>& _map = resources->get_map();
    std::cout << "input_resources_count:" << _map.size() << std::endl;
    for (auto & v : _map) {
        std::cout << "--" << "key:" << top::to_hex_prefixed(v.first) << " value_size:" << v.second.size() << std::endl;
    }
}

void  xblockdump_t::dump_output_resources(base::xvblock_t* block) {
    auto resources = block->get_output()->get_resources();
    const std::map<std::string, std::string>& _map = resources->get_map();
    std::cout << "output_resources_count:" << _map.size() << std::endl;
    for (auto & v : _map) {
        std::cout << "--" << "key:" << top::to_hex_prefixed(v.first) << " value_size:" << v.second.size() << std::endl;
    }    
}

void  xblockdump_t::dump_object(base::xvblock_t* block) {
    std::string block_object_bin;
    block->serialize_to_string(block_object_bin);
    std::cout << "block_size:" << block_object_bin.size() + block->get_input()->get_resources_data().size() + block->get_output()->get_resources_data().size()
    << " object_size:" << block_object_bin.size()
    << " input_size:" << block->get_input()->get_resources_data().size()
    << " output_size:" << block->get_output()->get_resources_data().size() << std::endl;
} 

void  xblockdump_t::dump_unitblock(base::xvblock_t* block) {
    std::cout << "--unit:" << block->dump() << std::endl;
    std::cout << "--binlog_hash:" << top::to_hex_prefixed(block->get_binlog_hash()) 
    << " state_hash:" << top::to_hex_prefixed(block->get_fullstate_hash()) << std::endl;
    // dump_output_entitys(block);
}

void  xblockdump_t::dump_tableblock(base::xvblock_t* block) {
    std::cout << "block:" << block->dump() << std::endl;
    dump_object(block);
    dump_input_entitys(block);
    dump_output_entitys(block);
    dump_input_resources(block);
    dump_output_resources(block);

    std::error_code ec;
    std::vector<xobject_ptr_t<base::xvblock_t>> subblocks;
    data::xblockextract_t::unpack_subblocks(block, subblocks, ec);

    std::cout << "unit_count:" << subblocks.size() << std::endl;
    for (auto & subblock : subblocks) {
        dump_unitblock(subblock.get());
    }
}


NS_END2
