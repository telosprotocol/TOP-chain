// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xdata/xblockextract.h"
#include "xdata/xblockbuild.h"
#include "xbase/xutl.h"

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


NS_END2
