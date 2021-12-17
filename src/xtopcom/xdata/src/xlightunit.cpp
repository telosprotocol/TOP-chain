// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbase/xobject_ptr.h"
#include "xbase/xhash.h"
#include "xbase/xatom.h"
#include "xvledger/xvblockbuild.h"
#include "xdata/xdatautil.h"
#include "xdata/xlightunit.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xaction_parse.h"
#include "xdata/xrootblock.h"
#include "xdata/xblockbuild.h"
#include "xvledger/xvledger.h"

NS_BEG2(top, data)

xlightunit_block_para_t::~xlightunit_block_para_t() {

}

void xlightunit_block_para_t::set_one_input_tx(const xtransaction_ptr_t & tx) {
    xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(tx.get());
    m_raw_txs.push_back(cons_tx);
}

void xlightunit_block_para_t::set_one_input_tx(const xcons_transaction_ptr_t & input_tx) {
    m_raw_txs.push_back(input_tx);
}

void xlightunit_block_para_t::set_input_txs(const std::vector<xcons_transaction_ptr_t> & input_txs) {
    xassert(m_raw_txs.empty());
    m_raw_txs = input_txs;
    m_succ_txs.insert(m_succ_txs.end(), m_raw_txs.begin(), m_raw_txs.end());
}

void xlightunit_block_para_t::set_unchange_txs(const std::vector<xcons_transaction_ptr_t> & unchange_txs) {
    xassert(m_unchange_txs.empty());
    m_unchange_txs = unchange_txs;
    m_succ_txs.insert(m_succ_txs.end(), m_unchange_txs.begin(), m_unchange_txs.end());
}

std::vector<xcons_transaction_ptr_t> const & xlightunit_block_para_t::get_succ_txs() const {
    return m_succ_txs;
}

xlightunit_block_t::xlightunit_block_t()
: xblock_t((enum_xdata_type)object_type_value) {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_block_lightunit, 1);
}

xlightunit_block_t::xlightunit_block_t(base::xvheader_t & header, base::xvqcert_t & cert, base::xvinput_t* input, base::xvoutput_t* output)
: xblock_t(header, cert, input, output, (enum_xdata_type)object_type_value) {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_block_lightunit, 1);
}

xlightunit_block_t::~xlightunit_block_t() {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_block_lightunit, -1);
}

base::xobject_t * xlightunit_block_t::create_object(int type) {
    (void)type;
    return new xlightunit_block_t;
}

void * xlightunit_block_t::query_interface(const int32_t _enum_xobject_type_) {
    if (object_type_value == _enum_xobject_type_)
        return this;
    return xvblock_t::query_interface(_enum_xobject_type_);
}

void xlightunit_block_t::parse_to_json_v1(xJson::Value & root) {
    xJson::Value ji;
    if (base::xvblock_fork_t::is_block_older_version(get_block_version(), base::enum_xvblock_fork_version_unit_opt)) {
        auto txs = get_txs();
        for (auto & tx : txs) {
            xJson::Value jv;
            jv["tx_consensus_phase"] = tx->get_tx_subtype_str();
            jv["send_tx_lock_gas"] = static_cast<unsigned long long>(tx->get_send_tx_lock_tgas());
            jv["used_gas"] = tx->get_used_tgas();
            jv["used_tx_deposit"] = tx->get_used_deposit();
            jv["used_disk"] = tx->get_used_disk();
            jv["tx_exec_status"] = xtransaction_t::tx_exec_status_to_str(tx->get_tx_exec_status());  // 1: success, 2: fail
            xJson::Value jtx;
            jtx["0x" + tx->get_tx_hex_hash()] = jv;
            ji["txs"].append(jtx);
        }
    } else {
        auto tx_vec = get_txkeys();
        for (auto & tx : tx_vec) {
            xJson::Value jv;
            jv["tx_consensus_phase"] = tx.get_tx_subtype_str();
            xJson::Value jtx;
            jtx["0x" + tx.get_tx_hex_hash()] = jv;
            ji["txs"].append(jtx);
        }
    }
    root["lightunit"]["lightunit_input"] = ji;
}

void xlightunit_block_t::parse_to_json_v2(xJson::Value & root) {
    if (base::xvblock_fork_t::is_block_older_version(get_block_version(), base::enum_xvblock_fork_version_unit_opt)) {
        auto txs = get_txs();
        for (auto & tx : txs) {
            xJson::Value jv;
            jv["tx_consensus_phase"] = tx->get_tx_subtype_str();
            jv["tx_hash"] = "0x" + tx->get_tx_hex_hash();
            root["lightunit"]["txs"].append(jv);
        }
    } else {
        auto tx_vec = get_txkeys();
        for (auto & tx : tx_vec) {
            xJson::Value jv;
            jv["tx_consensus_phase"] = tx.get_tx_subtype_str();
            jv["tx_hash"] = "0x" + tx.get_tx_hex_hash();
            root["lightunit"]["txs"].append(jv);
        }
    }
}

void xlightunit_block_t::parse_to_json(xJson::Value & root, const std::string & rpc_version) {
    if (rpc_version == RPC_VERSION_V1) {
        parse_to_json_v1(root);
    } else {
        parse_to_json_v2(root);
    }
}

std::string xlightunit_block_t::dump_body() const {
    std::stringstream ss;
    ss << "{";
    ss << "input=" << get_input()->dump();
    ss << "output=" << get_output()->dump();
    ss << "}";
    return ss.str();
}

const std::vector<xlightunit_tx_info_ptr_t> xlightunit_block_t::get_txs() const {
    return get_lightunit_body().get_txs();
}

bool xlightunit_block_t::extract_sub_txs(std::vector<base::xvtxindex_ptr> & sub_txs) {
     if (get_input() != nullptr) {
        base::xvinentity_t* primary_input_entity = get_input()->get_primary_entity();
        if (primary_input_entity != nullptr) {
            const std::vector<base::xvaction_t> & actions = primary_input_entity->get_actions();
            for (auto & action : actions) {
                if (!action.get_org_tx_hash().empty()) {
                    enum_transaction_subtype _actionid = (enum_transaction_subtype)action.get_org_tx_action_id();
                    base::xvtxindex_ptr tx_index = make_object_ptr<base::xvtxindex_t>(*this, nullptr, action.get_org_tx_hash(), _actionid);
                    sub_txs.push_back(tx_index);
                }
            }
        }
    }
    return true;
}

const xlightunit_body_t & xlightunit_block_t::get_lightunit_body() const {
    try_load_body();
    // xassert(!m_cache_body.is_empty());
    return m_cache_body;
}

void xlightunit_block_t::load_body() const {
    if (get_input() == nullptr) {
        return;
    }
    base::xvinentity_t* primary_input_entity = get_input()->get_primary_entity();
    if (primary_input_entity == nullptr) {
        return;
    }
    const std::vector<base::xvaction_t> & actions = primary_input_entity->get_actions();
    for (auto & action : actions) {
        if (action.get_org_tx_hash().empty()) {
            continue;
        }
        xtransaction_ptr_t raw_tx = query_raw_transaction(action.get_org_tx_hash());
        xlightunit_tx_info_ptr_t txinfo = std::make_shared<xlightunit_tx_info_t>(action, raw_tx.get());
        m_cache_body.add_tx_info(txinfo);
    }
}

void xlightunit_block_t::try_load_body() const {
    std::call_once(m_once_load_flag, [this] () {
        load_body();
    });
}

uint32_t xlightunit_block_t::get_unconfirm_sendtx_num() const {
    base::xvoutentity_t* outentity = get_output()->get_primary_entity();
    if (outentity != nullptr) {
        std::string unconfirm_tx_num_str = outentity->query_value(base::xvoutentity_t::key_name_unconfirm_tx_count());
        if (!unconfirm_tx_num_str.empty()) {
            return base::xstring_utl::touint32(unconfirm_tx_num_str);
        }
    }
    // key_name_unconfirm_tx_count will be unusable after block 2.0.0
    return 0;
}

NS_END2
