// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbase/xobject_ptr.h"
#include "xbase/xhash.h"
#include "xbase/xatom.h"
#include "xvledger/xvtransaction.h"
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
    if (m_balance_change != 0) {
        ss << "balance:" << m_balance_change;
    }
    if (m_pledge_balance_change.tgas != 0) {
        ss << ",ptgas:" << m_pledge_balance_change.tgas;
    }
    if (m_pledge_balance_change.vote != 0) {
        ss << ",pvote:" << m_pledge_balance_change.vote;
    }
    if (m_pledge_balance_change.disk != 0) {
        ss << ",pdisk:" << m_pledge_balance_change.disk;
    }
    if (m_lock_balance_change != 0) {
        ss << ",lbalance:" << m_lock_balance_change;
    }
    if (m_lock_tgas_change != 0) {
        ss << ",ltgas:" << m_lock_tgas_change;
    }
    if (m_unvote_num_change != 0) {
        ss << ",unvote:" << m_unvote_num_change;
    }
    if (!m_props.empty()) {
        ss << ",up:";
        for (auto & v : m_props) {
            ss << ",k:" << v.first;
            ss << ",v:" << base::xhash32_t::digest(v.second);
        }
    }
    if (!m_contract_txs.empty()) {
        ss << ",ctxs:" << m_contract_txs.size();
        for (auto & v : m_contract_txs) {
            ss << ",tx:" << base::xhash32_t::digest(v->get_digest_hex_str());
        }
    }
    auto & ps = m_native_property.get_properties();
    if (!ps.empty()) {
        ss << ",np:";
#if 0
        base::xstream_t stream(base::xcontext_t::instance());
        m_native_property.serialize_to(stream);
        ss << " size:" << stream.size();
#endif
        for (auto iter = ps.begin(); iter != ps.end(); iter++) {
            auto obj_type = iter->second->get_obj_type();
            ss << " "   << iter->first;
            if (obj_type == base::xstring_t::enum_obj_type) {
                ss << ",s:"  << base::xhash32_t::digest(m_native_property.string_get(iter->first)->get());
            } else if (obj_type == base::xstrdeque_t::enum_obj_type) {
                auto ds = m_native_property.deque_get(iter->first)->get_deque();
                for (auto & d : ds) {
                    ss << ",d:"  << base::xhash32_t::digest(d);
                }
            } else if (obj_type == base::xstrmap_t::enum_obj_type) {
                auto ms = m_native_property.map_get(iter->first)->get_map();
                for (auto & m : ms) {
                    ss << ",m:"  << m.first << " " << base::xhash32_t::digest(m.second);
                }
            }
            ss << ";";
        }
    }
    if (m_prop_log != nullptr) {
#if 0
        base::xstream_t stream(base::xcontext_t::instance());
        m_prop_log->serialize_to(stream);
        ss << " size:" << stream.size();
        ss << ",binlog:" << base::xhash32_t::digest(m_prop_log->get_hash_hex_str());
#endif
    }
    ss << "}";
    return ss.str();
}

xlightunit_block_para_t::~xlightunit_block_para_t() {
    m_raw_txs.clear();
    m_tx_result.m_contract_txs.clear();
}

void xlightunit_block_para_t::set_transaction_result(const xtransaction_result_t & result) {
    set_balance_change(result.m_balance_change);
    const xpledge_balance_change & pledge_change = result.m_pledge_balance_change;
    set_pledge_balance_change(pledge_change.tgas, pledge_change.disk, pledge_change.vote);
    set_lock_change(result.m_lock_balance_change, result.m_lock_tgas_change);
    set_unvote_num_change(result.m_unvote_num_change);
    set_propertys_change(result.m_props);
    set_contract_txs(result.m_contract_txs);
    set_native_property(result.m_native_property);
    set_property_log(result.m_prop_log);
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

void xlightunit_block_para_t::set_property_log(const xproperty_log_ptr_t & binlog) {
    m_tx_result.m_prop_log = binlog;
}

void xlightunit_block_para_t::set_pledge_balance_change(int64_t tgas_change, int64_t disk_change, int64_t vote_change) {
    m_tx_result.m_pledge_balance_change.tgas = tgas_change;
    m_tx_result.m_pledge_balance_change.disk = disk_change;
    m_tx_result.m_pledge_balance_change.vote = vote_change;
}

void xlightunit_block_para_t::set_lock_change(int64_t deposit_change, int64_t tgas_change) {
    m_tx_result.m_lock_balance_change = deposit_change;
    m_tx_result.m_lock_tgas_change = tgas_change;
}


xlightunit_output_resource_t::xlightunit_output_resource_t(const xlightunit_block_para_t & para) {
    // calc unconfirm sendtx num
    uint32_t send_tx_num = 0;
    uint32_t confirm_tx_num = 0;
    for (auto & tx : para.get_input_txs()) {
        if (tx->is_send_tx()) {
            send_tx_num++;
        } else if (tx->is_confirm_tx()) {
            confirm_tx_num++;
        }
    }
    for (auto & tx : para.get_contract_create_txs()) {
        xassert(tx->is_send_tx());
        send_tx_num++;
    }
    uint32_t account_prev_unconfirm_num = para.get_account_unconfirm_sendtx_num();
    m_unconfirm_sendtx_num = account_prev_unconfirm_num - confirm_tx_num;
    xassert(account_prev_unconfirm_num >= confirm_tx_num);
    m_prev_sendtx_confirmed_flag = m_unconfirm_sendtx_num == 0 ? 1 : 0;
    m_unconfirm_sendtx_num += send_tx_num;

    m_property_hash = para.get_props_hash();
    m_native_property.add_property(para.get_native_property());

    m_balance_change = para.get_balance_change();
    m_pledge_balance_change.tgas = para.get_pledge_balance_change_tgas();
    m_pledge_balance_change.disk = para.get_pledge_balance_change_disk();
    m_pledge_balance_change.vote = para.get_pledge_balance_change_vote();
    m_lock_balance_change = para.get_lock_change_balance();
    m_lock_tgas_change = para.get_lock_change_tgas();
    m_unvote_num_change = para.get_unvote_num_change();

    m_binlog = para.get_property_log();
}

int32_t xlightunit_output_resource_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    SERIALIZE_FIELD_BT(m_balance_change);
    SERIALIZE_FIELD_BT(m_pledge_balance_change.tgas);
    SERIALIZE_FIELD_BT(m_pledge_balance_change.disk);
    SERIALIZE_FIELD_BT(m_pledge_balance_change.vote);
    SERIALIZE_FIELD_BT(m_lock_balance_change);
    SERIALIZE_FIELD_BT(m_lock_tgas_change);
    SERIALIZE_FIELD_BT(m_unvote_num_change);
    m_native_property.serialize_to(stream);
    SERIALIZE_FIELD_BT(m_property_hash);
    SERIALIZE_FIELD_BT(m_unconfirm_sendtx_num);
    SERIALIZE_FIELD_BT(m_prev_sendtx_confirmed_flag);
    SERIALIZE_PTR(m_binlog) {
        m_binlog->serialize_to(stream);
    }
    return CALC_LEN();
}
int32_t xlightunit_output_resource_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    DESERIALIZE_FIELD_BT(m_balance_change);
    DESERIALIZE_FIELD_BT(m_pledge_balance_change.tgas);
    DESERIALIZE_FIELD_BT(m_pledge_balance_change.disk);
    DESERIALIZE_FIELD_BT(m_pledge_balance_change.vote);
    DESERIALIZE_FIELD_BT(m_lock_balance_change);
    DESERIALIZE_FIELD_BT(m_lock_tgas_change);
    DESERIALIZE_FIELD_BT(m_unvote_num_change);
    m_native_property.serialize_from(stream);
    MAP_DESERIALIZE_SIMPLE(stream, m_property_hash);
    DESERIALIZE_FIELD_BT(m_unconfirm_sendtx_num);
    DESERIALIZE_FIELD_BT(m_prev_sendtx_confirmed_flag);
    DESERIALIZE_PTR(m_binlog) {
        xaccount_binlog_t* _data = dynamic_cast<xaccount_binlog_t*>(base::xdataobj_t::read_from(stream));
        xassert(_data != nullptr);
        m_binlog.attach(_data);
    }
    return CALC_LEN();
}

std::string xlightunit_output_resource_t::get_property_hash(const std::string & prop_name) const {
    auto iter = m_property_hash.find(prop_name);
    if (iter != m_property_hash.end()) {
        return iter->second;
    }
    return {};
}

xblockbody_para_t xlightunit_block_t::get_blockbody_from_para(const xlightunit_block_para_t & para) {
    xblockbody_para_t blockbody;
    for (auto & tx : para.get_input_txs()) {
        enum_xunit_tx_exec_status last_action_status = enum_xunit_tx_exec_status_success;
        uint64_t last_tx_clock = 0;
        if (tx->is_recv_tx() || tx->is_confirm_tx()) {
            last_action_status = tx->get_receipt()->get_tx_info()->get_tx_exec_state().get_tx_exec_status();
            last_tx_clock = tx->get_clock();
        }
        xlightunit_input_entity_ptr_t ientity = make_object_ptr<xlightunit_input_entity_t>(tx->get_tx_subtype(),
                                                                                        tx->get_transaction(),
                                                                                        false,
                                                                                        last_action_status,
                                                                                        last_tx_clock);


        blockbody.add_input_entity(ientity);
        // if (tx->is_self_tx() || tx->is_send_tx()) {  // send tx save origin tx in lightunit
        //     xobject_ptr_t<xresource_origintx_t> origintx = make_object_ptr<xresource_origintx_t>(tx->get_transaction());
        //     std::string reskey = ientity->get_origintx_resource_key();
        //     std::string resvalue;
        //     origintx->serialize_to_string(resvalue);
        //     blockbody.add_input_resource(reskey, resvalue);
        // }
    }
    for (auto & tx : para.get_contract_create_txs()) {
        xassert(tx->get_tx_subtype() == enum_transaction_subtype_send);
        enum_xunit_tx_exec_status last_action_status = enum_xunit_tx_exec_status_success;
        uint64_t last_tx_clock = 0;
        xlightunit_input_entity_ptr_t ientity = make_object_ptr<xlightunit_input_entity_t>(tx->get_tx_subtype(),
                                                                                           tx->get_transaction(),
                                                                                           true,
                                                                                           last_action_status,
                                                                                           last_tx_clock);
        blockbody.add_input_entity(ientity);
        // if (tx->is_self_tx() || tx->is_send_tx()) {  // send tx save origin tx in lightunit
        //     xobject_ptr_t<xresource_origintx_t> origintx = make_object_ptr<xresource_origintx_t>(tx->get_transaction());
        //     std::string reskey = ientity->get_origintx_resource_key();
        //     std::string resvalue;
        //     origintx->serialize_to_string(resvalue);
        //     blockbody.add_input_resource(reskey, resvalue);
        // }
    }

    for (auto & tx : para.get_input_txs()) {
        xobject_ptr_t<xlightunit_output_entity_t> tx_info = make_object_ptr<xlightunit_output_entity_t>(tx->get_tx_subtype(), tx->get_transaction(), tx->get_tx_execute_state());
        blockbody.add_output_entity(tx_info);
    }
    for (auto & tx : para.get_contract_create_txs()) {
        xobject_ptr_t<xlightunit_output_entity_t> tx_info = make_object_ptr<xlightunit_output_entity_t>(tx->get_tx_subtype(), tx->get_transaction(), tx->get_tx_execute_state());
        blockbody.add_output_entity(tx_info);
    }

    xlightunit_output_resource_ptr_t txout_resource = make_object_ptr<xlightunit_output_resource_t>(para);
    std::string out_resource_str;
    txout_resource->serialize_to_string(out_resource_str);
    blockbody.add_output_resource(xlightunit_output_resource_t::name(), out_resource_str);

    blockbody.create_default_input_output();
    return blockbody;
}

base::xvblock_t * xlightunit_block_t::create_lightunit(const std::string & account,
                                                       uint64_t height,
                                                       std::string last_block_hash,
                                                       std::string justify_block_hash,
                                                       uint64_t viewid,
                                                       uint64_t clock,
                                                       const std::string & last_full_block_hash,
                                                       uint64_t last_full_block_height,
                                                       const xlightunit_block_para_t & para) {
    xassert(!para.get_input_txs().empty());
    xblockbody_para_t blockbody = xlightunit_block_t::get_blockbody_from_para(para);
    base::xauto_ptr<base::xvheader_t> _blockheader = xblockheader_t::create_lightunit_header(account, height, last_block_hash, justify_block_hash, last_full_block_hash, last_full_block_height);
    base::xauto_ptr<xblockcert_t> _blockcert = xblockcert_t::create_blockcert(account, height, base::enum_xconsensus_flag_extend_cert, viewid, clock);
    xlightunit_block_t * lightunit = new xlightunit_block_t(*_blockheader, *_blockcert, blockbody.get_input(), blockbody.get_output());

    return lightunit;
}

base::xvblock_t* xlightunit_block_t::create_next_lightunit(const xinput_ptr_t & input,
                                                      const xoutput_ptr_t & output,
                                                      base::xvblock_t* prev_block) {
    base::xauto_ptr<base::xvheader_t> _blockheader = xblockheader_t::create_next_blockheader(prev_block, base::enum_xvblock_class_light);
    base::xauto_ptr<xblockcert_t> _blockcert = xblockcert_t::create_blockcert(prev_block->get_account(), _blockheader->get_height(),
        base::enum_xconsensus_flag_extend_cert, prev_block->get_viewid() + 1, prev_block->get_clock() + 1);
    xlightunit_block_t* lightunit = new xlightunit_block_t(*_blockheader, *_blockcert, input, output);
    return lightunit;
}

base::xvblock_t* xlightunit_block_t::create_genesis_lightunit(const std::string & account,
                                                              const xtransaction_ptr_t & genesis_tx,
                                                              const xtransaction_result_t & result) {
    xassert(account == genesis_tx->get_target_addr());

    xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(genesis_tx.get());

    xlightunit_block_para_t para;
    para.set_one_input_tx(cons_tx);
    para.set_transaction_result(result);
    return create_lightunit(account, 0, xrootblock_t::get_rootblock_hash(), std::string(), 0, 0, std::string(), 0, para);
}

base::xvblock_t* xlightunit_block_t::create_next_lightunit(const xlightunit_block_para_t & para, base::xvblock_t* prev_block) {
    if (prev_block->is_genesis_block() || prev_block->get_block_class() == base::enum_xvblock_class_full) {
        return create_lightunit(prev_block->get_account(), prev_block->get_height() + 1,
            prev_block->get_block_hash(), std::string(), prev_block->get_viewid() + 1, prev_block->get_clock() + 1,
            prev_block->get_block_hash(), prev_block->get_height(), para);
    } else {
        return create_lightunit(prev_block->get_account(), prev_block->get_height() + 1,
            prev_block->get_block_hash(), std::string(), prev_block->get_viewid() + 1, prev_block->get_clock() + 1,
            prev_block->get_last_full_block_hash(), prev_block->get_last_full_block_height(), para);
    }
}
base::xvblock_t* xlightunit_block_t::create_next_lightunit(const xlightunit_block_para_t & para, xblockchain2_t* chain) {
    return create_lightunit(chain->get_account(), chain->get_chain_height() + 1,
        chain->get_last_block_hash(), std::string(), 1, 1,
        chain->get_last_full_unit_hash(), chain->get_last_full_unit_height(), para);  // viewid and clock should be replaced by consensus
}

xlightunit_block_t::xlightunit_block_t()
: xblock_t((enum_xdata_type)object_type_value) {

}

xlightunit_block_t::xlightunit_block_t(base::xvheader_t & header, xblockcert_t & cert)
: xblock_t(header, cert, (enum_xdata_type)object_type_value) {
}
// xlightunit_block_t::xlightunit_block_t(base::xvheader_t & header, xblockcert_t & cert, const std::string & input, const std::string & output)
// : xblock_t(header, cert, input, output, (enum_xdata_type)object_type_value) {

// }

xlightunit_block_t::xlightunit_block_t(base::xvheader_t & header, xblockcert_t & cert, const xinput_ptr_t & input, const xoutput_ptr_t & output)
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

bool xlightunit_block_t::get_send_trans_info(uint64_t & lastest_number, uint256_t & lastest_hash) const {
    const std::vector<xlightunit_tx_info_ptr_t> & txs = get_txs();
    lastest_number = 0;  // reset first
    for (auto & tx : txs) {
        if (tx->is_self_tx() || tx->is_send_tx()) {
            if (tx->get_raw_tx()->get_tx_nonce() > lastest_number) {
                lastest_number = tx->get_raw_tx()->get_tx_nonce();
                lastest_hash = tx->get_raw_tx()->digest();
            }
        }
    }
    return lastest_number != 0;  // tx nonce increase from 1
}

bool xlightunit_block_t::get_recv_trans_info(uint64_t & total_number, uint256_t & latest_hash) const {
    total_number = 0;  // reset first
    const std::vector<xlightunit_tx_info_ptr_t> & txs = get_txs();
    for (auto & tx : txs) {
        if (tx->is_recv_tx()) {
            total_number++;
            latest_hash = tx->get_raw_tx()->digest();
        }
    }
    return total_number != 0;
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
    xmerkle_path_256_t path;
    bool ret = calc_output_merkle_path(leaf, path);
    if (!ret) {
        xwarn("xtable_block_t::create_txreceipt calc_output_merkle_path fail, tx=%s", tx->dump().c_str());
        return nullptr;
    }
    xtx_receipt_ptr_t txreceipt = make_object_ptr<xtx_receipt_t>(txinfo, path, get_blockcert());
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
