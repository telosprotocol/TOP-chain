// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xrootblock.h"

#include "xvledger/xvblock.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xdata_common.h"
#include "xdata/xdatautil.h"
#include "xdata/xnative_contract_address.h"
#include "xconfig/xpredefined_configurations.h"

#include <stdexcept>
#include <string>

NS_BEG2(top, data)

REG_CLS(xrootblock_input_t);
REG_CLS(xrootblock_t);

int32_t xrootblock_input_t::do_write(base::xstream_t &stream) {
    KEEP_SIZE();
    SERIALIZE_FIELD_BT(m_account_balances);
    SERIALIZE_FIELD_BT(m_geneis_funds_accounts);
    SERIALIZE_FIELD_BT(m_tcc_accounts);
    SERIALIZE_CONTAINER(m_genesis_nodes) {
        item.serialize(stream);
    }
    return CALC_LEN();
}
int32_t xrootblock_input_t::do_read(base::xstream_t &stream) {
    KEEP_SIZE();
    DESERIALIZE_FIELD_BT(m_account_balances);
    DESERIALIZE_FIELD_BT(m_geneis_funds_accounts);
    DESERIALIZE_FIELD_BT(m_tcc_accounts);
    DESERIALIZE_CONTAINER(m_genesis_nodes) {
        node_info_t item;
        item.deserialize(stream);
        m_genesis_nodes.push_back(item);
    }
    return CALC_LEN();
}

bool xrootblock_input_t::set_account_balances(std::map<std::string, uint64_t> const& balances) {
    m_account_balances = balances;
    return true;
}

bool xrootblock_input_t::set_genesis_funds_accounts(std::vector<std::string> const& accounts) {
    m_geneis_funds_accounts = accounts;
    return true;
}

bool xrootblock_input_t::set_genesis_tcc_accounts(std::vector<std::string> const& accounts) {
    m_tcc_accounts = accounts;
    return true;
}

bool xrootblock_input_t::set_genesis_nodes(const std::vector<node_info_t> & nodes) {
    m_genesis_nodes = nodes;
    return true;
}

const uint64_t xrootblock_input_t::get_account_balance(const std::string& account_addr) const {
    auto entry = m_account_balances.find(account_addr);
    if (entry != m_account_balances.end()) {
        return entry->second;
    } else {
        return 0;
    }
}

// std::string xrootblock_input_t::body_dump() const {
//     std::stringstream ss;
//     ss << "{";
//     if (!m_account_balances.empty()) {
//         ss << "balance_count=" << m_account_balances.size() << ":";
//         for (auto & v : m_account_balances) {
//             ss << v.first << " ";
//             ss << v.second << ";";
//         }
//     }
//     ss << "}";
//     return ss.str();
// }

std::once_flag xrootblock_t::m_flag;
xrootblock_t* xrootblock_t::m_instance = nullptr;

xblockbody_para_t xrootblock_t::get_blockbody_from_para(const xrootblock_para_t & para) {
    xblockbody_para_t blockbody;
    xobject_ptr_t<xrootblock_input_t> input = make_object_ptr<xrootblock_input_t>();
    input->set_account_balances(para.m_account_balances);
    input->set_genesis_funds_accounts(para.m_geneis_funds_accounts);
    input->set_genesis_tcc_accounts(para.m_tcc_accounts);
    input->set_genesis_nodes(para.m_genesis_nodes);
    blockbody.add_input_entity(input);
    xobject_ptr_t<xdummy_entity_t> output = make_object_ptr<xdummy_entity_t>();
    blockbody.add_output_entity(output);
    blockbody.create_default_input_output();
    return blockbody;
}
bool xrootblock_t::init(const xrootblock_para_t & para) {
    std::call_once(m_flag, [&] () {
        xblock_para_t block_para;
        std::string _chain_name = XGET_CONFIG(chain_name);
        block_para.chainid     = top::config::to_chainid(_chain_name);
        block_para.block_level = base::enum_xvblock_level_chain;  // every chain has a root block
        block_para.block_class = base::enum_xvblock_class_light;
        block_para.block_type  = base::enum_xvblock_type_boot;
        block_para.account     = get_rootblock_address();
        block_para.height      = 0;
        block_para.last_block_hash = "";
        block_para.justify_block_hash = "";
        block_para.last_full_block_hash = "";
        block_para.last_full_block_height = 0;

        xblockbody_para_t blockbody = xrootblock_t::get_blockbody_from_para(para);
        base::xauto_ptr<base::xvheader_t> _blockheader = xblockheader_t::create_blockheader(block_para);
        base::xauto_ptr<xblockcert_t> _blockcert = xblockcert_t::create_blockcert(block_para.account, 0, base::enum_xconsensus_flag_commit_cert, 0, 0);
        m_instance = new xrootblock_t(*_blockheader, *_blockcert, blockbody.get_input(), blockbody.get_output());
        xkinfo("root-block info. block=%s", m_instance->dump().c_str());
        return true;
    });

    return true;
}

xrootblock_t::xrootblock_t(base::xvheader_t & header, xblockcert_t & cert, const xinput_ptr_t & input, const xoutput_ptr_t & output)
: xblock_t(header, cert, input, output, (enum_xdata_type)object_type_value) {

}

xrootblock_t::xrootblock_t() : xblock_t((enum_xdata_type)object_type_value) { }

xrootblock_t::~xrootblock_t() { }

base::xobject_t * xrootblock_t::create_object(int type) {
    (void)type;
    return new xrootblock_t;
}

void * xrootblock_t::query_interface(const int32_t _enum_xobject_type_) {
    if (object_type_value == _enum_xobject_type_)
        return this;
    return xvblock_t::query_interface(_enum_xobject_type_);
}

void xrootblock_t::dump_block_data(xJson::Value & json) const {
    json["chain_id"] = get_rootblock_chainid();
    json["chain_name"] = XGET_CONFIG(chain_name);
    json["rootblock_address"] = get_rootblock_address();
    auto genesis_accounts = get_seed_nodes();
    for (auto info : genesis_accounts) {
        xJson::Value accounts_info;
        accounts_info["node_account_address"] = info.m_account.value();
        accounts_info["node_public_key"] = info.m_publickey.to_string();
        json["genesis_accounts"].append(accounts_info);
    }
}

base::xvblock_t* xrootblock_t::get_rootblock() {
    xassert(m_instance != nullptr);
    return m_instance;
}

std::string xrootblock_t::get_rootblock_address() {
    return genesis_root_addr_main_chain;
}

uint64_t xrootblock_t::get_initial_balance(const std::string& account_addr) {
    xassert(m_instance != nullptr);
    if (m_instance != nullptr) {
        return m_instance->get_rootblock_input()->get_account_balance(account_addr);
    }
    return 0;
}

const std::map<std::string, uint64_t>& xrootblock_t::get_account_balances() {
    xassert(m_instance != nullptr);
    return m_instance->get_rootblock_input()->get_account_balances();
}

const std::vector<std::string>& xrootblock_t::get_tcc_initial_committee_addr() {
    xassert(m_instance != nullptr);
    return m_instance->get_rootblock_input()->get_tcc_accounts();
}

const std::vector<node_info_t> & xrootblock_t::get_seed_nodes() {
    xassert(m_instance != nullptr);
    xassert(!m_instance->get_rootblock_input()->get_seed_nodes().empty());
    return m_instance->get_rootblock_input()->get_seed_nodes();
}

std::map<std::string, uint64_t> xrootblock_t::get_all_genesis_accounts() {
    xassert(m_instance != nullptr);
    std::map<std::string, uint64_t> accounts = m_instance->get_rootblock_input()->get_account_balances();
    const auto & seed_nodes = m_instance->get_rootblock_input()->get_seed_nodes();
    for (auto & v : seed_nodes) {
        accounts[v.m_account.value()] = 0;
    }
    return accounts;
}

bool xrootblock_t::is_seed_node(const std::string & account) {
    const std::vector<node_info_t> & nodes = get_seed_nodes();
    for (auto & v : nodes) {
        if (v.m_account.value() == account) {
            return true;
        }
    }
    return false;
}

const std::string xrootblock_t::get_rootblock_hash() {
    xassert(m_instance != nullptr);
    return m_instance->get_block_hash();
}

base::enum_xchain_id xrootblock_t::get_rootblock_chainid() {
    xassert(m_instance != nullptr);
    return static_cast<base::enum_xchain_id>(m_instance->get_chainid());
}

void xrootblock_t::get_rootblock_data(xJson::Value & json) {
    xassert(m_instance != nullptr);
    if (m_instance != nullptr) {
        m_instance->dump_block_data(json);
    }
    return;
}

NS_END2
