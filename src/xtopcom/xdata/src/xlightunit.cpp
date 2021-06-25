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
    base::xvoutentity_t* outentity = get_output()->get_primary_entity();
    if (outentity != nullptr) {
        std::string unconfirm_tx_num_str = outentity->query_value(base::xvoutentity_t::key_name_unconfirm_tx_count());
        if (!unconfirm_tx_num_str.empty()) {
            return base::xstring_utl::touint32(unconfirm_tx_num_str);
        }
    }
    xassert(false);
    return 0;
}

NS_END2
