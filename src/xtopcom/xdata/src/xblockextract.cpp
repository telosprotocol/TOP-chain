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

void xblockextract_t::loop_top_txactions(base::xvblock_t* _block, std::function<void(const base::xvaction_t & _action)> _func) {
    if (_block->get_block_class() == base::enum_xvblock_class_nil) {
        return;
    }

    std::error_code ec;
    auto input_object = _block->load_input(ec);
    if (nullptr == input_object) {
        return;
    }
    auto & all_entitys = input_object->get_entitys();
    for (auto & entity : all_entitys) {
        // it must be xinentitys
        base::xvinentity_t* _inentity = dynamic_cast<base::xvinentity_t*>(entity);
        assert(_inentity != nullptr);
        auto & all_actions = _inentity->get_actions();
        for (auto & action : all_actions) {
            if (action.get_org_tx_hash().empty()) {
                continue;
            }
            _func(action);
        }
    }

    return;
}

void xblockextract_t::loop_eth_txactions(base::xvblock_t* _block, std::function<void(const base::xvaction_t & _action)> _func) {
    if (_block->get_block_class() == base::enum_xvblock_class_nil) {
        return;
    }

    std::error_code ec;
    auto input_object = _block->load_input(ec);
    if (nullptr == input_object) {
        return;
    }
    base::xvinentity_t* primary_input_entity = input_object->get_primary_entity();
    xassert(primary_input_entity != nullptr);
    if (!primary_input_entity->get_extend_data().empty()) {
        xtable_primary_inentity_extend_t _extend;
        int32_t ret = _extend.serialize_from_string(primary_input_entity->get_extend_data());
        if (ret <= 0) {
            xassert(false);
            return;
        }
        base::xvactions_t _ethreceipt_actions = _extend.get_txactions();
        for (auto & action : _ethreceipt_actions.get_actions()) {
            if (action.get_org_tx_hash().empty()) {
                xassert(false);
                continue;
            }
            _func(action);
        }
    }

    return;
}

void xblockextract_t::loop_all_txactions(base::xvblock_t* _block, std::function<void(const base::xvaction_t & _action)> _func) {
    if (_block->get_block_class() == base::enum_xvblock_class_nil) {
        return;
    }

    auto excontainer = _block->get_excontainer();
    if (excontainer != nullptr) {
        auto actions = excontainer->get_input_actions();
        for (auto & action : *actions) {
            if (action.get_org_tx_hash().empty()) {
                continue;
            }
            _func(action);
        }
        return;
    }

    loop_top_txactions(_block, _func);
    loop_eth_txactions(_block, _func);
    return;
}

void xblockextract_t::extract_sub_txs(base::xvblock_t* _block, std::vector<base::xvtxindex_ptr> & sub_txs) {
    if (_block->get_block_class() == base::enum_xvblock_class_nil) {
        return;
    }

    auto f = [&sub_txs,&_block](const base::xvaction_t & action) { 
        base::xvtxindex_ptr tx_index = make_object_ptr<base::xvtxindex_t>(*_block, action.get_org_tx_hash(), (base::enum_transaction_subtype)action.get_org_tx_action_id());
        sub_txs.push_back(tx_index);
    };
    loop_all_txactions(_block, f);
    xdbg("xblockextract_t::extract_sub_txs block=%s,count=%zu",_block->dump().c_str(),sub_txs.size());
    return;
}

uint32_t xblockextract_t::get_txactions_count(base::xvblock_t* _block) {
    if (_block->get_block_class() == base::enum_xvblock_class_nil) {
        return 0;
    }
    uint32_t count = 0;
    auto f = [&count](const base::xvaction_t & _action) { 
        count++; 
    };
    loop_all_txactions(_block, f);
    xdbg("xblockextract_t::get_txactions_count block=%s,count=%d",_block->dump().c_str(),count);
    return count;
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

    auto f = [&txactions](const base::xvaction_t & action) { 
        xlightunit_action_t txaction(action);
        txactions.push_back(txaction);
    };
    loop_all_txactions(_block, f);
    xdbg("xblockextract_t::unpack_txactions block=%s,count=%zu",_block->dump().c_str(),txactions.size());
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
    auto f = [&txactions](const base::xvaction_t & action) { 
        xlightunit_action_t txaction(action);
        txactions.push_back(txaction);
    };
    loop_eth_txactions(_block, f);
    return txactions;
}

xlightunit_action_ptr_t xblockextract_t::unpack_one_txaction(base::xvblock_t* _block, std::string const& txhash) {
    if (_block->get_block_class() == base::enum_xvblock_class_nil) {
        return nullptr;
    }

    data::xlightunit_action_ptr_t txaction_ptr = nullptr;

    auto f = [&txaction_ptr, &txhash](const base::xvaction_t & action) { 
        if (action.get_org_tx_hash() == txhash) {
            txaction_ptr = std::make_shared<xlightunit_action_t>(action); // TODO(jimmy) return stop 
        }
    };
    loop_all_txactions(_block, f);
    if (nullptr == txaction_ptr) {
        xerror("xblockextract_t::unpack_one_txaction fail-find tx.block=%s,tx=%s", _block->dump().c_str(), base::xstring_utl::to_hex(txhash).c_str());
    }

    return txaction_ptr;
}

void xblockextract_t::unpack_ethheader(base::xvblock_t* _block, xeth_header_t & ethheader, std::error_code & ec) {
    if (_block->is_genesis_block()) {
        uint64_t gas_limit = XGET_ONCHAIN_GOVERNANCE_PARAMETER(block_gas_limit);
        ethheader.set_gaslimit(gas_limit);
        return;
    }

    xassert(_block->get_block_level() == base::enum_xvblock_level_table);
    base::xtableheader_extra_t header_extra;
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

evm_common::xh256_t xblockextract_t::get_state_root(base::xvblock_t * block, std::error_code & ec) {
    assert(!ec);

    if (block->get_height() == 0 || !base::xvblock_fork_t::is_block_match_version(block->get_block_version(), base::enum_xvblock_fork_version_5_0_0)) {
        xdbg("xblockextract_t::get_state_root block is old version or height = 0 block:%s", block->dump().c_str());
        return evm_common::xh256_t{};
    }

    data::xeth_header_t ethheader;
    unpack_ethheader(block, ethheader, ec);
    if (ec) {
        return evm_common::xh256_t{};
    }

    return ethheader.get_state_root();
}

evm_common::xh256_t xblockextract_t::get_state_root_from_block(base::xvblock_t * block) {
    std::error_code ec;
    auto const & state_root = get_state_root(block, ec);
    if (ec) {  // should not happen
        xerror("xblockextract_t::get_state_root_from_block get state root fail. block:%s", block->dump().c_str());
        return evm_common::xh256_t{};
    }

    return state_root;
}

xtransaction_ptr_t xblockextract_t::unpack_raw_tx(base::xvblock_t* _block, std::string const& txhash, std::error_code & ec) {
    std::string orgtx_bin = _block->query_input_resource(txhash);
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

    std::string relayblock_resource = _block->query_output_resource(base::xvoutput_t::RESOURCE_RELAY_BLOCK);
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

void xblockextract_t::get_tableheader_extra_from_block(base::xvblock_t* _block, base::xtableheader_extra_t &header_extra, std::error_code & ec) {
    assert(!ec);

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

    // if (!_block->is_body_and_offdata_ready(false)) {
    //     ec = common::error::xerrc_t::invalid_block;
    //     xerror("xblockextract_t::unpack_subblocks input and output should ready. %s", _block->dump().c_str());
    //     return;
    // }

    auto excontainer = _block->get_excontainer();
    if (excontainer != nullptr) {
        excontainer->extract_sub_blocks(sublocks);
        xdbg_info("xblockextract_t::unpack_subblocks from container.block=%s,sublocks=%zu",_block->dump().c_str(),sublocks.size());
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
