// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbase/xobject_ptr.h"
#include "xbase/xhash.h"
#include "xbase/xatom.h"
#include "xvledger/xvtxindex.h"
#include "xvledger/xvblockbuild.h"
#include "xdata/xdatautil.h"
#include "xdata/xlightunit.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xaction_parse.h"
#include "xdata/xrootblock.h"

NS_BEG2(top, data)

REG_CLS(xlightunit_block_t);


std::string xtransaction_result_t::dump() const {
    std::stringstream ss;
    ss << "{";
    if (!m_contract_txs.empty()) {
        ss << ",ctxs:" << m_contract_txs.size();
        for (auto & v : m_contract_txs) {
            ss << ",tx:" << base::xhash32_t::digest(v->get_digest_hex_str());
        }
    }
    ss << "}";
    return ss.str();
}

xlightunit_block_para_t::~xlightunit_block_para_t() {
    m_raw_txs.clear();
    m_tx_result.m_contract_txs.clear();
}

void xlightunit_block_para_t::set_transaction_result(const xtransaction_result_t & result) {
    m_tx_result = result;
    // set_balance_change(result.m_balance_change);
    // const xpledge_balance_change & pledge_change = result.m_pledge_balance_change;
    // set_pledge_balance_change(pledge_change.tgas, pledge_change.disk, pledge_change.vote);
    // set_unvote_num_change(result.m_unvote_num_change);
    // set_contract_txs(result.m_contract_txs);
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
}

void xlightunit_block_para_t::set_contract_txs(const std::vector<xcons_transaction_ptr_t> & contract_txs) {
    xassert(m_tx_result.m_contract_txs.empty());
    m_tx_result.m_contract_txs = contract_txs;
}

xlightunit_block_t::xlightunit_block_t()
: xblock_t((enum_xdata_type)object_type_value) {

}

xlightunit_block_t::xlightunit_block_t(base::xvheader_t & header, base::xvqcert_t & cert, base::xvinput_t* input, base::xvoutput_t* output)
: xblock_t(header, cert, input, output, (enum_xdata_type)object_type_value) {

}

xlightunit_block_t::~xlightunit_block_t() {

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

std::string xlightunit_block_t::dump_body() const {
    std::stringstream ss;
    ss << "{";
    ss << "input=" << get_input()->dump();
    ss << "output=" << get_output()->dump();
    ss << "}";
    return ss.str();
}

xcons_transaction_ptr_t xlightunit_block_t::create_txreceipt(const xtransaction_t* tx, const base::xvaction_t & action) {
    if (get_cert()->get_extend_cert().empty() || get_cert()->get_extend_data().empty()) {
        xerror("xlightunit_block_t::create_txreceipts failed for create receipt without parent cert. unit=%s, tx=", dump().c_str(), tx->dump().c_str());
        return nullptr;
    }

    std::string leaf;
    action.serialize_to(leaf);
    base::xmerkle_path_256_t path;
    bool ret = base::xvblockmaker_t::calc_input_merkle_path(get_input(), leaf, path);
    if (!ret) {
        xwarn("xtable_block_t::create_txreceipt fail-calc merkle path, tx=%s", tx->dump().c_str());
        return nullptr;
    }
    base::xtx_receipt_ptr_t txreceipt = make_object_ptr<base::xtx_receipt_t>(action, path, get_cert());
    xcons_transaction_ptr_t contx = make_object_ptr<xcons_transaction_t>((xtransaction_t*)tx, txreceipt);
    contx->set_unit_height(get_height());
    return contx;
}



xcons_transaction_ptr_t xlightunit_block_t::create_one_txreceipt(const xtransaction_t* tx) {
    base::xvinentity_t* primary_input_entity = get_input()->get_primary_entity();
    const std::vector<base::xvaction_t> & actions = primary_input_entity->get_actions();
    for (auto & action : actions) {
        if (action.get_org_tx_hash() == tx->get_digest_str()) {
            return create_txreceipt(tx, action);
        }
    }
    xerror("xlightunit_block_t::create_one_txreceipt fail find tx in unit.tx=%s,unit=%s", base::xstring_utl::to_hex(tx->get_digest_str()).c_str(), dump().c_str());
    return nullptr;
}

xtransaction_ptr_t  xlightunit_block_t::query_raw_transaction(const std::string & txhash) const {
    std::string value = get_input()->query_resource(txhash);
    if (!value.empty()) {
        xtransaction_ptr_t raw_tx = make_object_ptr<xtransaction_t>();
        raw_tx->serialize_from_string(value);
        return raw_tx;
    }
    return nullptr;
}

void xlightunit_block_t::create_send_txreceipts(std::vector<xcons_transaction_ptr_t> & sendtx_receipts) {
    base::xvinentity_t* primary_input_entity = get_input()->get_primary_entity();
    const std::vector<base::xvaction_t> & actions = primary_input_entity->get_actions();
    for (auto & action : actions) {
        // TODO(jimmy) sendtx need create txreceipt
        if ( (base::enum_transaction_subtype)action.get_org_tx_action_id() == base::enum_transaction_subtype_send ) {
            xtransaction_ptr_t raw_tx = query_raw_transaction(action.get_org_tx_hash());
            xassert(raw_tx != nullptr);
            xcons_transaction_ptr_t cons_tx = create_txreceipt(raw_tx.get(), action);
            sendtx_receipts.push_back(cons_tx);
        }
    }
}
void xlightunit_block_t::create_txreceipts(std::vector<xcons_transaction_ptr_t> & sendtx_receipts, std::vector<xcons_transaction_ptr_t> & recvtx_receipts) {
    base::xvinentity_t* primary_input_entity = get_input()->get_primary_entity();
    const std::vector<base::xvaction_t> & actions = primary_input_entity->get_actions();
    for (auto & action : actions) {
        if ((base::enum_transaction_subtype)action.get_org_tx_action_id() == base::enum_transaction_subtype_send) {
            xtransaction_ptr_t raw_tx = query_raw_transaction(action.get_org_tx_hash());
            xassert(raw_tx != nullptr);
            xcons_transaction_ptr_t cons_tx = create_txreceipt(raw_tx.get(), action);
            sendtx_receipts.push_back(cons_tx);
        } else if ((base::enum_transaction_subtype)action.get_org_tx_action_id() == base::enum_transaction_subtype_recv) {
            xtransaction_ptr_t raw_tx = query_raw_transaction(action.get_org_tx_hash());
            xassert(raw_tx != nullptr);
            xcons_transaction_ptr_t cons_tx = create_txreceipt(raw_tx.get(), action);
            recvtx_receipts.push_back(cons_tx);
        }
    }
}

const std::vector<xlightunit_tx_info_ptr_t> & xlightunit_block_t::get_txs() const {
    return get_lightunit_body().get_txs();
}

bool xlightunit_block_t::extract_sub_txs(std::vector<base::xvtxindex_ptr> & sub_txs) {
    const std::vector<xlightunit_tx_info_ptr_t> & txs_info = get_txs();
    xassert(!txs_info.empty());
    for (auto & tx : txs_info) {
        base::xvtxindex_ptr tx_index = make_object_ptr<base::xvtxindex_t>(*this, tx->get_raw_tx().get(), tx->get_tx_hash(), tx->get_tx_subtype());
        sub_txs.push_back(tx_index);
    }
    return true;
}

const xlightunit_body_t & xlightunit_block_t::get_lightunit_body() const {
    try_load_body();
    xassert(!m_cache_body.is_empty());
    return m_cache_body;
}

void xlightunit_block_t::load_body() const {
    base::xvinentity_t* primary_input_entity = get_input()->get_primary_entity();
    const std::vector<base::xvaction_t> & actions = primary_input_entity->get_actions();
    for (auto & action : actions) {
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
    // TODO(jimmy)
    std::string unconfirm_tx_num_str = get_input()->query_resource(xlightunit_block_t::unconfirm_tx_num_name());
    if (!unconfirm_tx_num_str.empty()) {
        return base::xstring_utl::touint32(unconfirm_tx_num_str);
    }
    xassert(false);
    return 0;
}

NS_END2
