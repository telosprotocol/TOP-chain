// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xblock.h"
#include "xdata/xmemcheck_dbg.h"
#include "xdata/xfullunit.h"
#include "xdata/xlightunit.h"
#include "xdata/xemptyblock.h"
#include "xdata/xtableblock.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xrootblock.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xtransaction_v1.h"
#include "xdata/xtransaction_v2.h"
#include "xvledger/xtxreceipt.h"
#include "xvledger/xvpropertyprove.h"

#include "xbase/xhash.h"
#include "xbase/xobject.h"
#include "xbase/xutl.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xmetrics/xmetrics.h"
#include "xbasic/xdbg.h"
#include "xvledger/xvblockbuild.h"

#include <cinttypes>
#include <string>
NS_BEG2(top, data)

std::map<std::string, std::string>      xblock_t::m_empty_map;
std::vector<xlightunit_tx_info_ptr_t>   xblock_t::m_empty_txs;
uint256_t                               xblock_t::m_empty_uint256;
std::string                             xblock_t::m_empty_string;
std::vector<xobject_ptr_t<xblock_t>>    xblock_t::m_empty_blocks;

xblock_consensus_para_t::xblock_consensus_para_t(const std::string & _account, uint64_t _clock, uint64_t _viewid, uint32_t _viewtoken, uint64_t _proposal_height)
: m_account(_account), m_clock(_clock), m_viewtoken(_viewtoken), m_viewid(_viewid), m_proposal_height(_proposal_height) {
    set_empty_xip2(m_validator);
    set_empty_xip2(m_auditor);
    char local_param_buf[128];
    xprintf(local_param_buf,sizeof(local_param_buf),
        "{%s,height=%" PRIu64 ",viewid=%" PRIu64 ",viewtoken=%u,clock=%" PRIu64 "}",
        _account.c_str(), _proposal_height, _viewid, _viewtoken, _clock);
    m_dump_str = std::string(local_param_buf);
}

xblock_consensus_para_t::xblock_consensus_para_t(const xvip2_t & validator, base::xvblock_t* prev_block) {
    uint32_t viewtoken = prev_block->get_viewtoken() + 1;
    if (viewtoken == 0) {
        viewtoken++;
    }
    uint64_t viewid = prev_block->get_viewid() + 1;
    uint64_t drand_height = 0;
    xvip2_t auditor_xip;
    set_empty_xip2(auditor_xip);
    set_common_consensus_para(prev_block->get_clock() + 1, validator, auditor_xip, viewid, viewtoken, drand_height);
    m_account = prev_block->get_account();
    m_proposal_height = prev_block->get_height() + 1;
}
void xblock_consensus_para_t::set_xip(const xvip2_t & _validator_xip, const xvip2_t & _auditor_xip) {
    m_validator = _validator_xip;
    m_auditor = _auditor_xip;
}

void xblock_consensus_para_t::set_drand_block(base::xvblock_t* _drand_block) {
    m_drand_block = xblock_t::raw_vblock_to_object_ptr(_drand_block);
    m_drand_height = _drand_block->get_height();
}
void xblock_consensus_para_t::set_latest_blocks(const base::xblock_mptrs & latest_blocks) {
    m_account = latest_blocks.get_latest_cert_block()->get_account();
    m_proposal_height = latest_blocks.get_latest_cert_block()->get_height() + 1;
    m_latest_cert_block = xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_cert_block());
    m_latest_locked_block = xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_locked_block());
    m_latest_committed_block = xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_committed_block());
}
void xblock_consensus_para_t::set_common_consensus_para(uint64_t clock,
                                                        const xvip2_t & validator,
                                                        const xvip2_t & auditor,
                                                        uint64_t viewid,
                                                        uint32_t viewtoken,
                                                        uint64_t drand_height) {
    m_clock = clock;
    m_validator = validator;
    m_auditor = auditor;
    m_viewid = viewid;
    m_viewtoken = viewtoken;
    m_drand_height = drand_height;
}

void xblock_consensus_para_t::set_tableblock_consensus_para(uint64_t drand_height,
                                                            const std::string & random_seed,
                                                            uint64_t total_lock_tgas_token,
                                                            const std::string & extra_data) {
    m_drand_height = drand_height;
    m_random_seed = random_seed;
    m_total_lock_tgas_token = total_lock_tgas_token;
    m_extra_data = extra_data;
}

int32_t xblockheader_extra_data_t::do_write(base::xstream_t & stream) const {
    KEEP_SIZE();
    stream << m_paras;
    return CALC_LEN();
}

int32_t xblockheader_extra_data_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    stream >> m_paras;
    return CALC_LEN();
}

int32_t xblockheader_extra_data_t::deserialize_from_string(const std::string & extra_data) {
    base::xstream_t _stream(base::xcontext_t::instance(), (uint8_t*)extra_data.data(), (uint32_t)extra_data.size());
    return serialize_from(_stream);
}

int32_t xblockheader_extra_data_t::serialize_to_string(std::string & extra_data) {
    if (!m_paras.empty()) {
        base::xautostream_t<1024> _stream(base::xcontext_t::instance());
        int32_t ret = serialize_to(_stream);
        extra_data.clear();
        extra_data.assign((const char *)_stream.data(), _stream.size());
        return ret;
    }
    return 0;
}

uint64_t xblockheader_extra_data_t::get_tgas_total_lock_amount_property_height() const {
    auto iter = m_paras.find(enum_extra_data_type_tgas_total_lock_amount_property_height);
    if (iter == m_paras.end()) {
        return 0;
    } else {
        return base::xstring_utl::touint64(iter->second);
    }
}

void xblockheader_extra_data_t::set_tgas_total_lock_amount_property_height(uint64_t height) {
    std::string height_str = base::xstring_utl::tostring(height);
    m_paras[enum_extra_data_type_tgas_total_lock_amount_property_height] = height_str;
}

xobject_ptr_t<xblock_t> xblock_t::raw_vblock_to_object_ptr(base::xvblock_t* vblock) {
    xblock_ptr_t object_ptr;
    xblock_t* block_ptr = dynamic_cast<xblock_t*>(vblock);
    xassert(block_ptr != nullptr);
    block_ptr->add_ref();
    object_ptr.attach(block_ptr);
    return object_ptr;
}

void  xblock_t::txs_to_receiptids(const std::vector<xlightunit_tx_info_ptr_t> & txs_info, base::xreceiptid_check_t & receiptid_check) {
    for (auto & tx : txs_info) {
        if (tx->is_send_tx()) {
            uint64_t sendid = tx->get_receipt_id();
            base::xtable_shortid_t tableid = tx->get_receipt_id_peer_tableid();
            receiptid_check.set_sendid(tableid, sendid);
        } else if (tx->is_recv_tx()) {
            uint64_t recvid = tx->get_receipt_id();
            base::xtable_shortid_t tableid = tx->get_receipt_id_peer_tableid();
            receiptid_check.set_recvid(tableid, recvid);
        } else if (tx->is_confirm_tx()) {
            uint64_t confirmid = tx->get_receipt_id();
            base::xtable_shortid_t tableid = tx->get_receipt_id_peer_tableid();
            receiptid_check.set_confirmid(tableid, confirmid);
        }
    }
}

void  xblock_t::register_object(base::xcontext_t & _context) {
#ifdef DEBUG    // should only call once
    static int32_t static_registered_flag = 0;
    ++static_registered_flag;
    xassert(static_registered_flag == 1);
#endif

    auto lambda_new_rootblock= [](const int type)->xobject_t*{
        return new xrootblock_t();
    };
    auto lambda_new_fullunit= [](const int type)->xobject_t*{
        return new xfullunit_block_t();
    };
    auto lambda_new_lightunit= [](const int type)->xobject_t*{
        return new xlightunit_block_t();
    };
    auto lambda_new_cons_transaction = [](const int type)->xobject_t*{
        return new xcons_transaction_t();
    };
    auto lambda_new_lighttable = [](const int type)->xobject_t*{
        return new xtable_block_t();
    };
    auto lambda_new_fulltable = [](const int type)->xobject_t*{
        return new xfull_tableblock_t();
    };
    auto lambda_new_emptyblock = [](const int type)->xobject_t*{
        return new xemptyblock_t();
    };
    auto lambda_new_txreceipt = [](const int type)->xobject_t*{
        return new base::xtx_receipt_t();
    };    
    auto lambda_new_transactionv1 = [](const int type)->xobject_t*{
        return new xtransaction_v1_t();
    };    
    auto lambda_new_transactionv2 = [](const int type)->xobject_t*{
        return new xtransaction_v2_t();
    };      
    auto lambda_new_property_prove = [](const int type)->xobject_t*{
        return new base::xvproperty_prove_t();
    };
    base::xcontext_t::register_xobject2(_context,(base::enum_xobject_type)xrootblock_t::get_object_type(),lambda_new_rootblock);
    base::xcontext_t::register_xobject2(_context,(base::enum_xobject_type)xfullunit_block_t::get_object_type(),lambda_new_fullunit);
    base::xcontext_t::register_xobject2(_context,(base::enum_xobject_type)xlightunit_block_t::get_object_type(),lambda_new_lightunit);
    base::xcontext_t::register_xobject2(_context,(base::enum_xobject_type)xcons_transaction_t::get_object_type(),lambda_new_cons_transaction);
    base::xcontext_t::register_xobject2(_context,(base::enum_xobject_type)xtable_block_t::get_object_type(),lambda_new_lighttable);
    base::xcontext_t::register_xobject2(_context,(base::enum_xobject_type)xfull_tableblock_t::get_object_type(),lambda_new_fulltable);    
    base::xcontext_t::register_xobject2(_context,(base::enum_xobject_type)xemptyblock_t::get_object_type(),lambda_new_emptyblock);
    base::xcontext_t::register_xobject2(_context,(base::enum_xobject_type)base::xtx_receipt_t::get_object_type(),lambda_new_txreceipt);
    base::xcontext_t::register_xobject2(_context,(base::enum_xobject_type)xtransaction_v1_t::get_object_type(),lambda_new_transactionv1);
    base::xcontext_t::register_xobject2(_context,(base::enum_xobject_type)xtransaction_v2_t::get_object_type(),lambda_new_transactionv2);
    base::xcontext_t::register_xobject2(_context,(base::enum_xobject_type)base::xvproperty_prove_t::get_object_type(),lambda_new_property_prove);
    xkinfo("xblock_t::register_object,finish");    
}

xblock_t::xblock_t(base::xvheader_t & header, base::xvqcert_t & cert, enum_xdata_type type)
    : base::xvblock_t(header, cert, nullptr, nullptr, type) {
    MEMCHECK_ADD_TRACE(this, "block_create");
}

xblock_t::xblock_t(enum_xdata_type type) : base::xvblock_t(type) {
    MEMCHECK_ADD_TRACE(this, "block_create");
}

xblock_t::xblock_t(base::xvheader_t & header, base::xvqcert_t & cert, base::xvinput_t* input, base::xvoutput_t* output, enum_xdata_type type)
  : base::xvblock_t(header, cert, input, output, type) {
    MEMCHECK_ADD_TRACE(this, "block_create");
}

xblock_t::~xblock_t() {
    xdbg("xblock_t::~xblock_t %s", dump().c_str());
    MEMCHECK_REMOVE_TRACE(this);
}

#ifdef XENABLE_PSTACK  // tracking memory
int32_t xblock_t::add_ref() {
    MEMCHECK_ADD_TRACE(this, "block_add");
    return base::xvblock_t::add_ref();
}
int32_t xblock_t::release_ref() {
    MEMCHECK_ADD_TRACE(this, "block_release");
    xdbgassert(get_refcount() > 0);
    return base::xvblock_t::release_ref();
}
#endif

int32_t xblock_t::full_block_serialize_to(base::xstream_t & stream) {
    if (!is_input_ready(true) || !is_output_ready(true)) {
        xerror("xblock_t::full_block_serialize_to not valid block.%s", dump().c_str());
        return -1;
    }

    KEEP_SIZE();
    std::string block_object_bin;
    this->serialize_to_string(block_object_bin);
    stream << block_object_bin;
    if (get_header()->get_block_class() != base::enum_xvblock_class_nil) {
        stream << get_input()->get_resources_data();
        stream << get_output()->get_resources_data();
    }

    xdbg("xblock_t::full_block_serialize_to,succ.block=%s,object_bin=%ld,ir=%ld,or=%ld",
        dump().c_str(), base::xhash64_t::digest(block_object_bin), base::xhash64_t::digest(get_input()->get_resources_data()), base::xhash64_t::digest(get_output()->get_resources_data()));
    return CALC_LEN();
}

base::xvblock_t* xblock_t::full_block_read_from(base::xstream_t & stream) {
    xassert(stream.size() > 0);
    std::string block_object_bin;
    stream >> block_object_bin;
    base::xvblock_t* new_block = base::xvblock_t::create_block_object(block_object_bin);
    if (new_block == nullptr) {
        xerror("xblock_t::full_block_read_from not valid block.object_bin=%ld", base::xhash64_t::digest(block_object_bin));
        return nullptr;
    }
    xassert(new_block != nullptr);
    if (new_block != nullptr && new_block->get_header()->get_block_class() != base::enum_xvblock_class_nil) {
        std::string _input_content;
        std::string _output_content;
        stream >> _input_content;
        stream >> _output_content;
        if (false == new_block->set_input_resources(_input_content)) {
            xerror("xblock_t::full_block_read_from set_input_resources fail.block=%s,ir=%ld",new_block->dump().c_str(),base::xhash64_t::digest(_input_content));
            new_block->release_ref();
            return nullptr;
        }
        if (false == new_block->set_output_resources(_output_content)) {
            xerror("xblock_t::full_block_read_from set_output_resources failblock=%s,or=%ld",new_block->dump().c_str(),base::xhash64_t::digest(_output_content));
            new_block->release_ref();
            return nullptr;
        }
    }
    return new_block;
}

std::string xblock_t::dump_header(base::xvheader_t* header) {
    std::stringstream ss;
    ss << "{";
    ss << header->get_account();
    ss << ",height=" << header->get_height();
    ss << ",level:" << header->get_block_level();
    ss << ",class=" << header->get_block_class();
    ss << ",type=" << header->get_block_type();
    ss << ",features:" << header->get_block_features();
    ss << ",version:" << header->get_block_version();
    ss << ",weight:" << header->get_weight();
    if (!header->get_comments().empty()) {
        ss << ",comments:"  << header->get_comments();
    }
    if (!header->get_extra_data().empty()) {
        ss << ",extra:"     << base::xhash64_t::digest(header->get_extra_data());
    }
    ss << ",last_h:" << base::xhash64_t::digest(header->get_last_block_hash());
    ss << ",input_h=" << base::xhash64_t::digest(header->get_input_hash());
    ss << ",output_h=" << base::xhash64_t::digest(header->get_output_hash());
    ss << "}";
    return ss.str();
}

std::string xblock_t::dump_header() const {
    std::stringstream ss;
    ss << "{";
    ss << get_header()->get_account();
    ss << ",height=" << get_header()->get_height();
    ss << ",level:" << get_header()->get_block_level();
    ss << ",class=" << get_header()->get_block_class();
    ss << ",type=" << get_header()->get_block_type();
    ss << ",features:" << get_header()->get_block_features();
    ss << ",version:" << get_header()->get_block_version();
    ss << ",weight:" << get_header()->get_weight();
    if (!get_header()->get_comments().empty()) {
        ss << ",comments:"  << get_header()->get_comments();
    }
    ss << ",extra:"     << base::xhash64_t::digest(get_header()->get_extra_data());
    ss << ",last_h:" << base::xhash64_t::digest(get_header()->get_last_block_hash());
    ss << ",input_h=" << base::xhash64_t::digest(get_header()->get_input_hash());
    ss << ",output_h=" << base::xhash64_t::digest(get_header()->get_output_hash());
    if (get_block_class() != base::enum_xvblock_class_nil) {
        std::string     _input_bin;
        std::string     _output_bin;
        get_input()->serialize_to_string(_input_bin);
        get_output()->serialize_to_string(_output_bin);
        ss << ",i_=" << base::xhash64_t::digest(_input_bin);
        ss << ",ic_=" << get_input()->get_entitys().size();
        ss << ",o_=" << base::xhash64_t::digest(_output_bin);
        ss << ",oc_=" << get_output()->get_entitys().size();
    }
    ss << "}";
    return ss.str();
}

std::string xblock_t::dump_cert(base::xvqcert_t * qcert) const {
    std::stringstream ss;
    ss << "{";
    ss << ",nonce="     << qcert->get_nonce();
    ss << ",expire="    << qcert->get_expired();
    ss << ",drand="     << qcert->get_drand_height();
    ss << ",parent_h="  << qcert->get_parent_block_height();
    ss << " validator=" << qcert->get_validator().low_addr;
    ss << " auditor="   << qcert->get_auditor().low_addr;
    ss << " sign_h="    << base::xhash64_t::digest(qcert->get_hash_to_sign());
    ss << " header_h="   << base::xhash64_t::digest(qcert->get_header_hash());
    ss << " input_r="   << base::xhash64_t::digest(qcert->get_input_root_hash());
    ss << " output_r="  << base::xhash64_t::digest(qcert->get_output_root_hash());
    ss << " justify_r="  << base::xhash64_t::digest(qcert->get_justify_cert_hash());
    ss << " verify_th=" << qcert->get_validator_threshold();
    ss << " audit_th="  << qcert->get_auditor_threshold();
    ss << " verify_sig="<< qcert->get_verify_signature().size();
    ss << " audit_sig=" << qcert->get_audit_signature().size();
    ss << " cons_type=" << qcert->get_consensus_type();
    ss << " cons_th=" << qcert->get_consensus_threshold();
    ss << " cons_flag=" << qcert->get_consensus_flags();
    ss << " key_type=" << qcert->get_crypto_key_type();
    ss << " sign_type=" << qcert->get_crypto_sign_type();
    ss << " hash_type=" << qcert->get_crypto_hash_type();
    ss << " extend=" << base::xhash64_t::digest(qcert->get_extend_data());
    ss << "," << base::xhash64_t::digest(qcert->get_extend_cert());

    ss << "}";

    return ss.str();
}

std::string xblock_t::dump_cert() const {
    std::stringstream ss;
    ss << "cert:" << dump_cert(get_cert());
    return ss.str();
}

std::string xblock_t::dump_body() const {
    std::stringstream ss;
    if (get_input() != nullptr) {
        ss << get_input()->dump();
    }
    if (get_output() != nullptr) {
        ss << get_output()->dump();
    }
    return ss.str();
}

std::string xblock_t::get_block_hash_hex_str() const {
    return to_hex_str(get_block_hash());
}

xlightunit_tx_info_ptr_t xblock_t::get_tx_info(const std::string & txhash) const {
    const auto & txs = get_txs();
    for (auto & tx : txs) {
        if (tx->get_tx_hash() == txhash) {
            return tx;
        }
    }
    return nullptr;
}

xtransaction_ptr_t  xblock_t::query_raw_transaction(const std::string & txhash) const {
    std::string value = get_input()->query_resource(txhash);
    if (!value.empty()) {
        xtransaction_ptr_t raw_tx;
        xtransaction_t::set_tx_by_serialized_data(raw_tx, value);
        return raw_tx;
    }
    return nullptr;
}

uint32_t  xblock_t::query_tx_size(const std::string & txhash) const {
    std::string value = get_input()->query_resource(txhash);
    return value.size();
}

std::vector<base::xvtxkey_t> xblock_t::get_txkeys() const {
    auto & extra_str = get_header()->get_extra_data();
    if (extra_str.empty()) {
        return {};
    }
    base::xvheader_extra he;
    he.serialize_from_string(extra_str);

    auto txs_str = he.get_val(base::HEADER_KEY_TXS);
    if (txs_str.empty()) {
        xdbg("empty header, account:%s", get_account().c_str());
        return std::vector<base::xvtxkey_t>{};
    }

    base::xvtxkey_vec_t txs;
    txs.serialize_from_string(txs_str);
    return txs.get_txkeys();
}

NS_END2
