// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xunit_service/xpreproposal_packer.h"

#include "xBFT/xconsevent.h"
#include "xbase/xutl.h"
#include "xbasic/xhex.h"
#include "xdata/xblockextract.h"
#include "xsafebox/safebox_proxy.h"
#include "xverifier/xverifier_utl.h"
#include "xblockmaker/xblockmaker_error.h"

#include <cinttypes>
NS_BEG2(top, xunit_service)

xpreproposal_packer::xpreproposal_packer(base::xtable_index_t & tableid,
                                         const std::string & account_id,
                                         std::shared_ptr<xcons_service_para_face> const & para,
                                         std::shared_ptr<xblock_maker_face> const & block_maker,
                                         base::xcontext_t & _context,
                                         uint32_t target_thread_id)
  : xbatch_packer(tableid, account_id, para, block_maker, _context, target_thread_id) {
    xunit_info("xpreproposal_packer::xpreproposal_packer,create,this=%p,account=%s", this, account_id.c_str());
}

xpreproposal_packer::~xpreproposal_packer() {
    xunit_info("xpreproposal_packer::~xpreproposal_packer,destory,this=%p, account=%s", this, get_account().c_str());
}

bool xpreproposal_packer::close(bool force_async) {
    xbatch_packer::close(force_async);
    xunit_dbg("xpreproposal_packer::close, this=%p,refcount=%d, account=%s", this, get_refcount(), get_account().c_str());
    return true;
}

bool xpreproposal_packer::proc_preproposal(const xvip2_t & leader_xip,  uint64_t height, uint64_t viewid, uint64_t clock, uint32_t viewtoken, const std::string & msgdata) {
    xpreproposal_msg_t preproposal_msg;
    if (preproposal_msg.serialize_from_string(msgdata) <= 0) {
        xerror("xpreproposal_packer::proc_preproposal preproposal_msg serialize from fail");
        return false;
    }

    if (!connect_to_checkpoint()) {
        return false;
    }

    data::xblock_consensus_para_t cs_para(get_account(), clock, viewid, viewtoken, height, preproposal_msg.get_gmtime());
    xdbg("xpreproposal_packer::proc_preproposal cs_para:%s", cs_para.dump().c_str());
    cs_para.set_tgas_height(preproposal_msg.get_total_lock_tgas_token_height());
    cs_para.set_xip(preproposal_msg.get_validator_xip(), preproposal_msg.get_auditor_xip());
    cs_para.set_drand_height(preproposal_msg.get_drand_height());

    auto local_block = get_proposal_maker()->make_proposal_backup(
        cs_para, preproposal_msg.get_last_block_hash(), preproposal_msg.get_justify_cert_hash(), preproposal_msg.get_input_txs(), preproposal_msg.get_receiptid_state_proves());
    if (local_block == nullptr) {
        xwarn("xpreproposal_packer::proc_preproposal make proposal fail. cs_para:%s", cs_para.dump().c_str());
        return false;
    }
    xinfo("xpreproposal_packer::proc_preproposal make proposal success. cs_para:%s", cs_para.dump().c_str());
    m_preproposal_block = local_block;
    return true;
}

int xpreproposal_packer::veriry_proposal_by_preproposal_block(base::xvblock_t * proposal_block) {
    if (m_preproposal_block == nullptr) {
        return blockmaker::xblockmaker_error_proposal_no_preproposal;
    }

    auto ret = get_proposal_maker()->verify_proposal(proposal_block, m_preproposal_block.get());
    if (ret != xsuccess) {
        m_preproposal_block = nullptr;
    }
    xinfo("xpreproposal_packer::veriry_proposal_by_preproposal_block ret:%d,proposal_block:%s", ret, proposal_block->dump().c_str());
    return ret;
}

void xpreproposal_packer::clear_for_new_view() {
    m_msg_state = enum_received_msg_nil;
    m_preproposal_block = nullptr;
}

void xpreproposal_packer::send_preproposal(const data::xblock_consensus_para_t & cs_para,
                                           const std::vector<data::xcons_transaction_ptr_t> & txs,
                                           const std::vector<base::xvproperty_prove_ptr_t> & receiptid_state_proves) {
    xvip2_t local_xip = get_xip2_addr();
    auto to_xip = local_xip;
    set_node_id_to_xip2(to_xip, common::xbroadcast_slot_id_value);

    xinfo("xpreproposal_packer::send_preproposal tps_key cs_para=%s", cs_para.dump().c_str());

    xpreproposal_msg_t msg(cs_para, txs, receiptid_state_proves);
    std::string msg_content;
    msg.serialize_to_string(msg_content);

    base::xbftpdu_t packet;
    packet.set_block_chainid(cs_para.get_latest_cert_block()->get_chainid());
    packet.set_block_account(get_account());
    packet.set_block_height(cs_para.get_latest_cert_block()->get_height() + 1);
    packet.set_block_clock(cs_para.get_clock());
    packet.set_block_viewid(cs_para.get_viewid());
    packet.set_block_viewtoken(cs_para.get_viewtoken());
    packet.reset_message(xconsensus::enum_consensus_msg_type_preproposal, get_default_msg_ttl(), msg_content, 0, local_xip.low_addr, to_xip.low_addr);

    send_out(local_xip, to_xip, packet, 0, 0);
}

xunit_service::xpreproposal_send_cb xpreproposal_packer::get_preproposal_send_cb() {
    return std::bind(&xpreproposal_packer::send_preproposal,
                     this,
                     std::placeholders::_1,
                     std::placeholders::_2,
                     std::placeholders::_3);
}

bool xpreproposal_packer::process_msg(const xvip2_t & from_addr, const xvip2_t & to_addr, const base::xcspdu_t & packet, int32_t cur_thread_id, uint64_t timenow_ms) {
    auto type = packet.get_msg_type();

    if (type == xconsensus::enum_consensus_msg_type_preproposal) {
        if (m_msg_state != enum_received_msg_nil) {
            xdbg("xpreproposal_packer::process_msg already recv msg,account=%s,m_msg_state=%d", get_account().c_str(), m_msg_state);
            return true;
        }
        m_msg_state = enum_received_msg_preproposal_first;
    } else if (type == xconsensus::enum_consensus_msg_type_proposal || type == xconsensus::enum_consensus_msg_type_proposal_v2) {
        if (m_msg_state == enum_received_msg_nil) {
            m_msg_state = enum_received_msg_proposal_first;
        }
    }
    return xcsaccount_t::recv_in(from_addr, to_addr, packet, cur_thread_id, timenow_ms);
}

xpreproposal_msg_t::xpreproposal_msg_t(const data::xblock_consensus_para_t & cs_para,
                                       const std::vector<data::xcons_transaction_ptr_t> & txs,
                                       const std::vector<base::xvproperty_prove_ptr_t> & receiptid_state_proves)
  : m_last_block_hash(cs_para.get_latest_cert_block()->get_block_hash())
  , m_justify_cert_hash(cs_para.get_latest_locked_block()->get_input_root_hash())
  , m_gmtime(cs_para.get_gmtime())
  , m_drand_height(cs_para.get_drand_height())
  , m_total_lock_tgas_token_height(cs_para.get_tgas_height())
  , m_auditor_xip(cs_para.get_auditor())
  , m_validator_xip(cs_para.get_validator())
  , m_input_txs(txs)
  , m_receiptid_state_proves(receiptid_state_proves) {
}

int32_t xpreproposal_msg_t::serialize_to_string(std::string & _str) const {
    base::xstream_t _raw_stream(base::xcontext_t::instance());
    int32_t ret = do_write(_raw_stream);
    _str.assign((const char *)_raw_stream.data(), _raw_stream.size());
    return ret;
}

int32_t xpreproposal_msg_t::serialize_from_string(const std::string & _str) {
    base::xstream_t _stream(base::xcontext_t::instance(), (uint8_t *)_str.data(), (int32_t)_str.size());
    int32_t ret = do_read(_stream);
    return ret;
}

int32_t xpreproposal_msg_t::do_write(base::xstream_t & stream) const {
    const int32_t begin_size = stream.size();

    stream.write_compact_var(m_version);
    stream << m_last_block_hash;
    stream << m_justify_cert_hash;
    stream.write_compact_var(m_gmtime);
    stream.write_compact_var(m_drand_height);
    stream.write_compact_var(m_total_lock_tgas_token_height);
    stream.write_compact_var(m_auditor_xip.low_addr);
    stream.write_compact_var(m_auditor_xip.high_addr);
    stream.write_compact_var(m_validator_xip.low_addr);
    stream.write_compact_var(m_validator_xip.high_addr);

    uint32_t count = m_input_txs.size();
    stream.write_compact_var(count);
    for (uint32_t i = 0; i < count; i++) {
        m_input_txs[i]->serialize_to(stream);
    }

    uint32_t prove_count = m_receiptid_state_proves.size();
    if (prove_count > 0) {
        stream.write_compact_var(prove_count);
        for (uint32_t i = 0; i < prove_count; i++) {
            m_receiptid_state_proves[i]->serialize_to(stream);
        }
    }

    return (stream.size() - begin_size);
}

int32_t xpreproposal_msg_t::do_read(base::xstream_t & stream) {
    const int32_t begin_size = stream.size();

    stream.read_compact_var(m_version);
    if (m_version != 0) {
        xwarn("xpreproposal_msg_t::do_read version not match:%d,", m_version);
        return -1;
    }
    stream >> m_last_block_hash;
    stream >> m_justify_cert_hash;
    stream.read_compact_var(m_gmtime);
    stream.read_compact_var(m_drand_height);
    stream.read_compact_var(m_total_lock_tgas_token_height);
    stream.read_compact_var(m_auditor_xip.low_addr);
    stream.read_compact_var(m_auditor_xip.high_addr);
    stream.read_compact_var(m_validator_xip.low_addr);
    stream.read_compact_var(m_validator_xip.high_addr);

    uint32_t count;
    stream.read_compact_var(count);
    for (uint32_t i = 0; i < count; i++) {
        data::xcons_transaction_ptr_t tx = make_object_ptr<data::xcons_transaction_t>();
        tx->serialize_from(stream);
        m_input_txs.push_back(tx);
    }

    if (stream.size() > 0) {
        uint32_t prove_count = 0;
        int32_t ret = stream.read_compact_var(prove_count);
        // for compatibility
        if (ret > 0) {
            for (uint32_t i = 0; i < prove_count; i++) {
                base::xvproperty_prove_ptr_t receiptid_state_prove = make_object_ptr<base::xvproperty_prove_t>();
                receiptid_state_prove->serialize_from(stream);
                m_receiptid_state_proves.push_back(receiptid_state_prove);
            }
        }
    }

    return (begin_size - stream.size());
}

NS_END2
