// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xdata/xblockextract.h"
#include "xdata/xblockbuild.h"
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

    if (_block->get_header()->get_extra_data().empty()) {
        ec = common::error::xerrc_t::invalid_block;
        xerror("xblockextract_t::unpack_ethheader fail-extra data empty.block=%s",_block->dump().c_str());
        return;
    }

    const std::string & extra_data = _block->get_header()->get_extra_data();
    data::xtableheader_extra_t blockheader_extradata;
    int32_t ret = blockheader_extradata.deserialize_from_string(extra_data);
    if (ret <= 0) {
        ec = common::error::xerrc_t::invalid_block;
        xerror("xblockextract_t::unpack_ethheader fail-extra data invalid.block=%s",_block->dump().c_str());
        return;
    }
    std::string eth_header_str = blockheader_extradata.get_ethheader();
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

void xblockextract_t::unpack_relayblock(base::xvblock_t* _block, bool include_sig, xrelay_block & relayblock, std::error_code & ec) {
    auto & header_extra = _block->get_header()->get_extra_data();
    xdbg("unpack_relayblock, header_extra: %d, %s", header_extra.size(), top::HexEncode(header_extra).c_str());
    //if (_block->get_height() == 0
    if (header_extra.empty()) {
        ec = common::error::xerrc_t::invalid_block;
        xerror("xblockextract_t::unpack_relayblock parameters invalid.");
        return;
    }

    data::xtableheader_extra_t last_header_extra;
    auto ret = last_header_extra.deserialize_from_string(header_extra);
    if (ret <= 0) {
        ec = common::error::xerrc_t::invalid_block;
        xerror("xblockextract_t::unpack_relayblock header extra data deserialize fail.");
        return;
    }
    auto relay_block_data_str = last_header_extra.get_relay_block_data();

    relayblock.decodeBytes(to_bytes(relay_block_data_str), ec);
    if (ec) {
        xwarn("xblockextract_t::unpack_relayblock decodeBytes error %s; err msg %s", ec.category().name(), ec.message().c_str());
        return;
    }

    if (include_sig) {
        std::string sign_list = _block->get_cert()->get_extend_data();
        if (sign_list.empty()) {
            ec = common::error::xerrc_t::invalid_block;
            xwarn("xblockextract_t::unpack_relayblock extend data empty.");
            return;
        }
        std::vector<xrelay_signature_node_t> signature_nodes;
        base::xstream_t stream{base::xcontext_t::instance(), (uint8_t *)sign_list.data(), static_cast<uint32_t>(sign_list.size())};
        uint16_t size = 0;
        stream >> size;
        for (uint16_t i = 0; i < size; i++) {
            std::string sign_str;
            stream >> sign_str;

            if (sign_str == "") {
                xrelay_signature_node_t signature;
                signature_nodes.push_back(signature);
                continue;
            }
            if (sign_str.size() != 65) {
                ec = common::error::xerrc_t::invalid_block;
                xerror("xblockextract_t::unpack_relayblock extend data empty.");
                return;
            }

            xrelay_signature_node_t signature{sign_str};
            signature_nodes.push_back(signature);
        }
        relayblock.add_signature_nodes(signature_nodes);
    }
}

NS_END2
