// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xrootblock.h"

#include "xvledger/xvblock.h"
#include "xvledger/xvstate.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xdata_common.h"
#include "xdata/xdatautil.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xblocktool.h"
#include "xconfig/xpredefined_configurations.h"

#include <stdexcept>
#include <string>

NS_BEG2(top, data)

xrootblock_input_t::xrootblock_input_t()
:base::xdataunit_t(base::xdataunit_t::enum_xdata_type_undefine) {

}
xrootblock_input_t::~xrootblock_input_t() {

}

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
xrootblock_input_t* m_rootblock_input = nullptr;
xrelay_block* m_relay_genesis_block = nullptr;

bool xrootblock_t::init(const xrootblock_para_t & para) {
    std::call_once(m_flag, [&] () {
        xvblock_t::register_object(base::xcontext_t::instance());  // rootblock init before blockstore
        xblock_t::register_object(base::xcontext_t::instance());
        std::string _chain_name = XGET_CONFIG(chain_name);
        base::enum_xchain_id chainid = top::config::to_chainid(_chain_name);

        base::xvblock_t* _rootblock = xblocktool_t::create_genesis_root_block(chainid, get_rootblock_address(), para);
        m_instance = dynamic_cast<xrootblock_t*>(_rootblock);
        xkinfo("root-block info. block=%s", m_instance->dump().c_str());
        auto const root_hash = XGET_CONFIG(root_hash);
        if (!root_hash.empty() && root_hash != base::xstring_utl::to_hex(_rootblock->get_block_hash())) {
            xerror("xrootblock_t::init chainid: %d, standard root hash: %s, init root hash: %s, error!", chainid, root_hash.c_str(), base::xstring_utl::to_hex(_rootblock->get_block_hash()).c_str());
            return false;
        }

        xobject_ptr_t<base::xvbstate_t> bstate = make_object_ptr<base::xvbstate_t>(*_rootblock->get_header());
        std::string binlog = _rootblock->get_binlog();
        if(binlog.empty())
        {
            xerror("xrootblock_t::init,invalid binlog");
            return false;
        }
        if(false == bstate->apply_changes_of_binlog(binlog))
        {
            xerror("xrootblock_t::init,invalid binlog");
            return false;
        }
        auto propobj = bstate->load_string_var(xrootblock_t::ROOT_BLOCK_PROPERTY_NAME);
        if (propobj == nullptr) {
            xerror("xrootblock_t::init,load string fail");
            return false;
        }
        std::string property_str = propobj->query();
        if (property_str.empty()) {
            xerror("xrootblock_t::init, string null");
            return false;
        }
        m_rootblock_input = new xrootblock_input_t();
        m_rootblock_input->serialize_from_string(property_str);

        m_relay_genesis_block = xblocktool_t::create_genesis_relay_block(para);
        if (m_relay_genesis_block == nullptr) {
            xerror("xrootblock_t::init, create relay_genesis_block failed");
            return false;
        }
        
        return true;
    });

    return true;
}

xrootblock_t::xrootblock_t(base::xvheader_t & header, base::xvqcert_t & cert, base::xvinput_t* input, base::xvoutput_t* output)
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

xrootblock_input_t* xrootblock_t::get_rootblock_input() const {
    return m_rootblock_input;
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
#if !defined(XENABLE_TESTS)
    xassert(!m_instance->get_rootblock_input()->get_seed_nodes().empty());
#endif
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

 xrelay_block* xrootblock_t::get_genesis_relay_block() {
    return m_relay_genesis_block;
}

NS_END2
