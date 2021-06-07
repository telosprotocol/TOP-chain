// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbase/xobject_ptr.h"
#include "xbase/xhash.h"
#include "xbase/xatom.h"
#include "xvledger/xvtxindex.h"
#include "xdata/xdatautil.h"
#include "xdata/xlightunit.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xaction_parse.h"
#include "xdata/xrootblock.h"
#include "xdata/xblock_resources.h"

NS_BEG2(top, data)

REG_CLS(xlightunit_block_t);
REG_CLS(xlightunit_output_resource_t);


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

xlightunit_output_resource_t::xlightunit_output_resource_t(const xlightunit_block_para_t & para) {
    m_unconfirm_sendtx_num = para.get_account_unconfirm_sendtx_num();
    m_property_binlog = para.get_property_binlog();
}

int32_t xlightunit_output_resource_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    SERIALIZE_FIELD_BT(m_unconfirm_sendtx_num);
    stream << m_property_binlog;  // TODO(jimmy)
    return CALC_LEN();
}
int32_t xlightunit_output_resource_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    DESERIALIZE_FIELD_BT(m_unconfirm_sendtx_num);
    stream >> m_property_binlog;  // TODO(jimmy)
    return CALC_LEN();
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

int64_t xlightunit_block_t::get_burn_balance_change() const {
    int64_t burn_balance = 0;
    const auto & output_entitys = get_output()->get_entitys();
    for (const auto & entity : output_entitys) {
        xlightunit_output_entity_t* output_tx = dynamic_cast<xlightunit_output_entity_t*>(entity);
        if (output_tx->is_self_tx() || output_tx->is_confirm_tx()) {
            burn_balance += output_tx->get_tx_exec_state().get_used_deposit();
        }

        burn_balance += output_tx->get_tx_exec_state().get_beacon_service_fee();
        burn_balance += output_tx->get_tx_exec_state().get_self_burn_balance();
    }
    return burn_balance;
}

std::string xlightunit_block_t::dump_body() const {
    std::stringstream ss;
    ss << "{";
    ss << "input=" << get_input()->dump();
    ss << "output=" << get_output()->dump();
    ss << "}";
    return ss.str();
}

xcons_transaction_ptr_t xlightunit_block_t::create_txreceipt(const xtransaction_t* tx, xlightunit_output_entity_t* txinfo) {
    if (get_cert()->get_extend_cert().empty() || get_cert()->get_extend_data().empty()) {
        xerror("xlightunit_block_t::create_txreceipts failed for create receipt without parent cert. unit=%s, tx=", dump().c_str(), tx->dump().c_str());
        return nullptr;
    }

    const std::string leaf = txinfo->query_value("merkle-tree-leaf");
    base::xmerkle_path_256_t path;
    bool ret = calc_output_merkle_path(leaf, path);
    if (!ret) {
        xwarn("xtable_block_t::create_txreceipt calc_output_merkle_path fail, tx=%s", tx->dump().c_str());
        return nullptr;
    }
    xtx_receipt_ptr_t txreceipt = make_object_ptr<xtx_receipt_t>(txinfo, path, get_cert());
    xcons_transaction_ptr_t contx = make_object_ptr<xcons_transaction_t>((xtransaction_t*)tx, txreceipt);
    contx->set_unit_height(get_height());
    return contx;
}



xcons_transaction_ptr_t xlightunit_block_t::create_one_txreceipt(const xtransaction_t* tx) {
    const auto & output_entitys = get_output()->get_entitys();
    for (size_t index = 0; index < output_entitys.size(); index++) {
        xlightunit_output_entity_t* output_entity = dynamic_cast<xlightunit_output_entity_t*>(output_entitys[index]);
        xassert(output_entity != nullptr);
        if (output_entity->get_tx_hash() == tx->get_digest_str()) {
            return create_txreceipt(tx, output_entity);
        }
    }
    xerror("xlightunit_block_t::create_one_txreceipt fail find tx in unit.tx=%s,unit=%s", base::xstring_utl::to_hex(tx->get_digest_str()).c_str(), dump().c_str());
    return nullptr;
}

void xlightunit_block_t::create_send_txreceipts(std::vector<xcons_transaction_ptr_t> & sendtx_receipts) {
    const auto & output_entitys = get_output()->get_entitys();
    const auto & input_entitys = get_input()->get_entitys();
    for (size_t index = 0; index < input_entitys.size(); index++) {
        xlightunit_input_entity_t* input_entity = dynamic_cast<xlightunit_input_entity_t*>(input_entitys[index]);
        xassert(input_entity != nullptr);
        if (input_entity->is_send_tx()) {
            const xtransaction_ptr_t & raw_tx = input_entity->get_raw_tx();
            xassert(raw_tx != nullptr);
            xlightunit_output_entity_t* output_entity = dynamic_cast<xlightunit_output_entity_t*>(output_entitys[index]);
            xassert(output_entity != nullptr);
            xcons_transaction_ptr_t cons_tx = create_txreceipt(raw_tx.get(), output_entity);
            sendtx_receipts.push_back(cons_tx);
        }
    }
}
void xlightunit_block_t::create_txreceipts(std::vector<xcons_transaction_ptr_t> & sendtx_receipts, std::vector<xcons_transaction_ptr_t> & recvtx_receipts) {
    const auto & output_entitys = get_output()->get_entitys();
    const auto & input_entitys = get_input()->get_entitys();
    for (size_t index = 0; index < input_entitys.size(); index++) {
        xlightunit_input_entity_t* input_entity = dynamic_cast<xlightunit_input_entity_t*>(input_entitys[index]);
        xassert(input_entity != nullptr);

        if (input_entity->is_send_tx()) {
            const xtransaction_ptr_t & raw_tx = input_entity->get_raw_tx();
            xassert(raw_tx != nullptr);
            xlightunit_output_entity_t* output_entity = dynamic_cast<xlightunit_output_entity_t*>(output_entitys[index]);
            xassert(output_entity != nullptr);
            xcons_transaction_ptr_t cons_tx = create_txreceipt(raw_tx.get(), output_entity);
            sendtx_receipts.push_back(cons_tx);
        } else if (input_entity->is_recv_tx()) {
            const xtransaction_ptr_t & raw_tx = input_entity->get_raw_tx();
            xassert(raw_tx != nullptr);
            xlightunit_output_entity_t* output_entity = dynamic_cast<xlightunit_output_entity_t*>(output_entitys[index]);
            xassert(output_entity != nullptr);
            xcons_transaction_ptr_t cons_tx = create_txreceipt(raw_tx.get(), output_entity);
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

const xlightunit_output_resource_ptr_t & xlightunit_block_t::get_tx_output_resource() const {
    return get_lightunit_body().get_txout_resource();
}

const xlightunit_body_t & xlightunit_block_t::get_lightunit_body() const {
    try_load_body();
    xassert(!m_cache_body.is_empty());
    return m_cache_body;
}

void xlightunit_block_t::load_body() const {
    const auto & output_entitys = get_output()->get_entitys();
    const auto & input_entitys = get_input()->get_entitys();
    xassert(output_entitys.size() > 0);
    xassert(input_entitys.size() > 0);
    xassert(input_entitys.size() == output_entitys.size());
    for (size_t index = 0; index < input_entitys.size(); index++) {
        xlightunit_input_entity_t* input_entity = dynamic_cast<xlightunit_input_entity_t*>(input_entitys[index]);
        xlightunit_output_entity_t* output_entity = dynamic_cast<xlightunit_output_entity_t*>(output_entitys[index]);
        xlightunit_tx_info_ptr_t txinfo = std::make_shared<xlightunit_tx_info_t>(input_entity->get_tx_key(),
                                                                                input_entity->get_raw_tx().get(),
                                                                                input_entity->get_input_tx_propertys(),
                                                                                output_entity->get_tx_exec_state());
        m_cache_body.add_tx_info(txinfo);
    }

    std::string tx_output_resource = get_output()->query_resource(xlightunit_output_resource_t::name());
    xassert(!tx_output_resource.empty());
    base::xstream_t _stream(base::xcontext_t::instance(), (uint8_t*)tx_output_resource.data(), (uint32_t)tx_output_resource.size());
    xlightunit_output_resource_t* txout_resource = dynamic_cast<xlightunit_output_resource_t*>(base::xdataunit_t::read_from(_stream));
    xassert(txout_resource != nullptr);

    xlightunit_output_resource_ptr_t txout_resource_ptr;
    txout_resource_ptr.attach(txout_resource);
    m_cache_body.add_lightunit_output_resource(txout_resource_ptr);
}

void xlightunit_block_t::try_load_body() const {
    std::call_once(m_once_load_flag, [this] () {
        load_body();
    });
}

NS_END2
