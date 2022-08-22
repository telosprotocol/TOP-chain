// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xblock_cs_para.h"

#include <cinttypes>
NS_BEG2(top, data)

xblock_consensus_para_t::xblock_consensus_para_t(const std::string & _account, uint64_t _clock, uint64_t _viewid, uint32_t _viewtoken, uint64_t _proposal_height, uint64_t _gmtime)
: m_account(_account), m_clock(_clock), m_viewtoken(_viewtoken), m_viewid(_viewid), m_proposal_height(_proposal_height), m_gmtime(_gmtime) {
    set_empty_xip2(m_validator);
    set_empty_xip2(m_auditor);
    char local_param_buf[128];
    xprintf(local_param_buf,sizeof(local_param_buf),
        "{%s,height=%" PRIu64 ",viewid=%" PRIu64 ",viewtoken=%u,clock=%" PRIu64 ",gmt=%" PRIu64 "}",
        _account.c_str(), _proposal_height, _viewid, _viewtoken, _clock, _gmtime);
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
    set_latest_blocks(xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_cert_block()),
                      xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_locked_block()),
                      xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_committed_block()));
}

void xblock_consensus_para_t::set_latest_blocks(const xblock_ptr_t & certblock, const xblock_ptr_t & lock_block, const xblock_ptr_t & commit_block) {
    m_proposal_height = certblock->get_height() + 1;
    m_latest_cert_block = certblock;
    m_latest_locked_block = lock_block;
    m_latest_committed_block = commit_block;

    set_justify_cert_hash(lock_block->get_input_root_hash());
    set_parent_height(0);
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
                                                            uint64_t total_lock_tgas_token_property_height) {
    m_drand_height = drand_height;
    m_random_seed = random_seed;
    m_total_lock_tgas_token = total_lock_tgas_token;
    m_total_lock_tgas_token_property_height = total_lock_tgas_token_property_height;
}

void xblock_consensus_para_t::set_table_state(xtablestate_ptr_t const& cert_state, xtablestate_ptr_t const& commit_state) {
    m_cert_tablestate = cert_state;
    m_commit_tablestate = commit_state;
}

xvip2_t xblock_consensus_para_t::get_leader_xip() const {
    if (m_auditor.high_addr != 0 && m_auditor.low_addr != 0 && get_node_id_from_xip2(m_auditor) != 0x3FF) {
        return m_auditor;
    }
    return m_validator;
}

NS_END2
