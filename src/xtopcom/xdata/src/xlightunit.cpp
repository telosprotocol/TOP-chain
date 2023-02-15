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
#include "xdata/xblockextract.h"
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
            jv["used_gas"] = static_cast<xJson::UInt64>(tx->get_used_tgas());
            jv["used_tx_deposit"] = static_cast<xJson::UInt64>(tx->get_used_deposit());
            jv["used_disk"] = static_cast<xJson::UInt64>(tx->get_used_disk());
            jv["tx_exec_status"] = xtransaction_t::tx_exec_status_to_str(tx->get_tx_exec_status());  // 1: success, 2: fail
            xJson::Value jtx;
            jtx["0x" + tx->get_tx_hex_hash()] = jv;
            ji["txs"].append(jtx);
        }
    } else {
        // auto tx_vec = get_txkeys();
        // for (auto & tx : tx_vec) {
        //     xJson::Value jv;
        //     jv["tx_consensus_phase"] = tx.get_tx_subtype_str();
        //     xJson::Value jtx;
        //     jtx["0x" + tx.get_tx_hex_hash()] = jv;
        //     ji["txs"].append(jtx);
        // }
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
        // auto tx_vec = get_txkeys();
        // xJson::Value txs;
        // for (auto & tx : tx_vec) {
        //     xJson::Value jv;
        //     jv["tx_consensus_phase"] = tx.get_tx_subtype_str();
        //     jv["tx_hash"] = "0x" + tx.get_tx_hex_hash();
        //     txs["txs"].append(jv);
        // }
        // root["lightunit"] = txs;
    }
}

void xlightunit_block_t::parse_to_json(xJson::Value & root, const std::string & rpc_version) {
    if (rpc_version == RPC_VERSION_V1) {
        parse_to_json_v1(root);
    } else {
        parse_to_json_v2(root);
    }
}

const std::vector<xlightunit_action_ptr_t> xlightunit_block_t::get_txs() const {
    auto _txactions = data::xblockextract_t::unpack_txactions((base::xvblock_t*)this);
    std::vector<xlightunit_action_ptr_t> txactions;
    for (auto & action : _txactions) {
        xlightunit_action_ptr_t txaction = std::make_shared<xlightunit_action_t>(action);
        txactions.push_back(txaction);
    }
    return txactions;
}

NS_END2
