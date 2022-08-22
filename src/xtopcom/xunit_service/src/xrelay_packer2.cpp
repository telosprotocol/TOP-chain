// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xutl.h"
#include "xBFT/xconsevent.h"
#include "xunit_service/xrelay_packer2.h"
#include "xdata/xblockextract.h"
#include "xverifier/xverifier_utl.h"
#include "xbasic/xhex.h"

#include <cinttypes>
NS_BEG2(top, xunit_service)

xrelay_packer2::xrelay_packer2(observer_ptr<mbus::xmessage_bus_face_t> const   &mb,
                           base::xtable_index_t &                          tableid,
                           const std::string &                             account_id,
                           std::shared_ptr<xcons_service_para_face> const &para,
                           std::shared_ptr<xblock_maker_face> const &      block_maker,
                           base::xcontext_t &                              _context,
                           uint32_t                                        target_thread_id) : xbatch_packer(mb, tableid, account_id, para, block_maker, _context, target_thread_id) {
    xunit_info("xrelay_packer2::xrelay_packer,create,this=%p,account=%s", this, account_id.c_str());
}

xrelay_packer2::~xrelay_packer2() {
    xunit_info("xrelay_packer2::~xrelay_packer,destory,this=%p, account=%s", this, get_account().c_str()); 
}

bool xrelay_packer2::close(bool force_async) {
    xbatch_packer::close(force_async);
    xunit_dbg("xrelay_packer2::close, this=%p,refcount=%d, account=%s", this, get_refcount(),  get_account().c_str());
    return true;
}

bool xrelay_packer2::on_timer_fire(const int32_t thread_id,
                                  const int64_t timer_id,
                                  const int64_t current_time_ms,
                                  const int32_t start_timeout_ms,
                                  int32_t & in_out_cur_interval_ms) {
    // XTODO relay-table should be slowly, no need timely try to make proposal    
    // xbatch_packer::on_timer_fire(thread_id, timer_id, current_time_ms, start_timeout_ms, in_out_cur_interval_ms);                             
    return true;
}

int32_t xrelay_packer2::set_vote_extend_data(base::xvblock_t * proposal_block, const uint256_t & hash, bool is_leader) {
    top::uint256_t hash_0;
    if (hash == hash_0) {
        return xsuccess;
    }

    auto local_xip = get_xip2_addr();
    // get leader xip
    auto leader_xip = proposal_block->get_cert()->get_validator();
    if (get_node_id_from_xip2(leader_xip) == 0x3FF) {
        leader_xip = proposal_block->get_cert()->get_auditor();
    }

    if (get_network_height_from_xip2(leader_xip) != get_network_height_from_xip2(local_xip)) {
        xerror("xrelay_packer2::set_vote_extend_data,fail-unmatch xip. is_leader=%d.block=%s,leader_xip=%s,local_xip=%s,network_height=%ld,%ld",
            is_leader, proposal_block->dump().c_str(), xcons_utl::xip_to_hex(leader_xip).c_str(), xcons_utl::xip_to_hex(local_xip).c_str(),
            get_network_height_from_xip2(leader_xip), get_network_height_from_xip2(local_xip));
        return -1;
    }

    xdbg("xrelay_packer2::set_vote_extend_data is_leader=%d.block=%s,leader_xip=%s,local_xip=%s,network_height=%ld,%ld",
        is_leader, proposal_block->dump().c_str(), xcons_utl::xip_to_hex(leader_xip).c_str(), xcons_utl::xip_to_hex(local_xip).c_str(),
        get_network_height_from_xip2(leader_xip), get_network_height_from_xip2(local_xip));

    auto prikey_str = get_vcertauth()->get_prikey(local_xip);
    uint8_t priv_content[xverifier::PRIKEY_LEN];
    memcpy(priv_content, prikey_str.data(), prikey_str.size());
    top::utl::xecprikey_t ecpriv(priv_content);

    auto signature = ecpriv.sign(hash);
    std::string signature_str = std::string((char *)signature.get_compact_signature(), signature.get_compact_signature_size());
    if (is_leader) {
        top::utl::xecpubkey_t pub_key_obj = ecpriv.get_public_key();
        std::string pubkey_str = std::string((char *)(pub_key_obj.data()), pub_key_obj.size());        
        m_relay_multisign[pubkey_str] = std::make_pair(local_xip, signature_str);
        m_relay_hash = hash;
        xdbg("xrelay_packer2::set_vote_extend_data leader proposal=%s,hash=%s,xip=%s,pubkey=%s",
            proposal_block->dump().c_str(), top::to_hex(top::to_bytes(hash)).c_str(), xcons_utl::xip_to_hex(local_xip).c_str(),base::xstring_utl::base64_encode(pub_key_obj.data(), pub_key_obj.size()).c_str());
    } else {
        proposal_block->set_vote_extend_data(signature_str);
        xdbg("xrelay_packer2::set_vote_extend_data backup proposal=%s,hash=%s,xip=%s",
            proposal_block->dump().c_str(), top::to_hex(top::to_bytes(hash)).c_str(), xcons_utl::xip_to_hex(local_xip).c_str());        
    }
    return xsuccess;
}

void xrelay_packer2::clear_for_new_view() {
    // clear relay multisign for new view.
    m_relay_multisign.clear();
    m_relay_hash = uint256_t();
}

void xrelay_packer2::send_receipts(base::xvblock_t *vblock) {
    // relay chain have no receipts.
}

uint32_t xrelay_packer2::calculate_min_tx_num(bool first_packing) {
    return 0;
}

bool xrelay_packer2::verify_vote_extend_data(base::xvblock_t * proposal_block, const xvip2_t & replica_xip, const std::string & vote_extend_data, std::string & result) {
    if ((proposal_block->get_cert()->get_consensus_flags() & base::enum_xconsensus_flag_extend_vote) == 0) {
        return true;
    }

    if (vote_extend_data.empty()) {
        xerror("xrelay_packer2::verify_vote_extend_data,fail-vote_extend_data empty for proposal=%s", proposal_block->dump().c_str());
        return true;
    }

    if (m_relay_hash == uint256_t()) {
        xerror("xrelay_packer2::verify_vote_extend_data m_relay_hash is 0, proposal=%s", proposal_block->dump().c_str());
    }

    const std::string account_addr_of_node = get_vcertauth()->get_signer(replica_xip);
    utl::xkeyaddress_t key_address(account_addr_of_node);

    utl::xecdsasig_t signature_obj((uint8_t *)vote_extend_data.c_str());

    uint8_t addr_type = 0;
    uint16_t net_id = 0;
    if (false == key_address.get_type_and_netid(addr_type, net_id)) {
        xerror("xrelay_packer2::verify_vote_extend_data get type and netid fail,proposal=%s,node:%s", proposal_block->dump().c_str(), account_addr_of_node.c_str());
        return false;
    }

    uint8_t out_publickey_data[65] = {0};
    if (top::utl::xsecp256k1_t::get_publickey_from_signature(signature_obj, m_relay_hash, out_publickey_data)) {
        top::utl::xecpubkey_t verify_key(out_publickey_data);
        if (verify_key.to_address(addr_type, net_id) != account_addr_of_node) {
            xerror("xrelay_packer2::verify_vote_extend_data adress not feat,proposal=%s,node:%s, verify_key.to_address(addr_type, net_id):%s",
                   proposal_block->dump().c_str(),
                   account_addr_of_node.c_str(),
                   verify_key.to_address(addr_type, net_id).c_str());
            return false;
        }

    } else {
        xerror("xrelay_packer2::verify_vote_extend_data get_publickey_from_signature fail,proposal=%s,node:%s", proposal_block->dump().c_str(), account_addr_of_node.c_str());
        return false;
    }

    result = std::string((char *)(out_publickey_data), 65);
    xdbg("xrelay_packer2::verify_vote_extend_data verify sign succ.proposal_block:%s,signer:%s", proposal_block->dump().c_str(), account_addr_of_node.c_str());
    return true;
}

void xrelay_packer2::add_vote_extend_data(base::xvblock_t * proposal_block, const xvip2_t & replica_xip, const std::string & vote_extend_data, const std::string & result) {
    if ((proposal_block->get_cert()->get_consensus_flags() & base::enum_xconsensus_flag_extend_vote) == 0) {
        return;
    }

    m_relay_multisign[result] = std::make_pair(replica_xip, vote_extend_data);
    xdbg("xrelay_packer2::add_vote_extend_data proposal:%s,xip=%s,pubkey=%s",
        proposal_block->dump().c_str(), xcons_utl::xip_to_hex(replica_xip).c_str(),base::xstring_utl::base64_encode((unsigned char const*)result.data(), result.size()).c_str());
}

bool xrelay_packer2::proc_vote_complate(base::xvblock_t * proposal_block) {
    if ((proposal_block->get_cert()->get_consensus_flags() & base::enum_xconsensus_flag_extend_vote) == 0) {
        return true;
    }

    if (m_relay_multisign.empty()) {
        xerror("xrelay_packer2::proc_vote_complate m_relay_multisign is empty,proposal:%s", proposal_block->dump().c_str());
        return false;
    }

    std::error_code ec;
    std::shared_ptr<data::xrelay_block> relay_block = data::xblockextract_t::unpack_relay_block_from_table(proposal_block, ec);    
    if (ec || nullptr == relay_block) {
        xerror("xrelay_packer2::proc_vote_complate last_relay_block decodeBytes,proposal:%s,error %s; err msg %s",
              proposal_block->dump().c_str(),
              ec.category().name(),
              ec.message().c_str());
        return false;
    }
    uint64_t election_round = m_election_round;

    data::xrelay_signature_group_t siggroup;
    siggroup.signature_epochID = election_round;

    uint32_t num = 0;
    uint32_t i = 0;
    for (auto & node : m_local_electset) {
        std::string raw_pub_key = base::xstring_utl::base64_decode(node.election_info.consensus_public_key.to_string());
        xassert(raw_pub_key.size() == 65);
        top::utl::xecpubkey_t pub_key_obj(raw_pub_key);
        std::string pubkey_str = std::string((char *)(pub_key_obj.data()), pub_key_obj.size());
        auto it = m_relay_multisign.find(pubkey_str);
        if (it == m_relay_multisign.end()) {
            data::xrelay_signature_node_t signature;
            siggroup.signature_vector.push_back(signature);
            xdbg("xrelay_packer2::proc_vote_complate not found in election.proposal:%s,elect idx:%u,pubkey=%s", proposal_block->dump().c_str(), i, node.election_info.consensus_public_key.to_string().c_str());
        } else {
            data::xrelay_signature_node_t signature{it->second.second};
            siggroup.signature_vector.push_back(signature);
            num++;
            xdbg("xrelay_packer2::proc_vote_complate found in election.proposal:%s,elect idx:%u,pubkey=%s", proposal_block->dump().c_str(), i, node.election_info.consensus_public_key.to_string().c_str());
        }
        i++;        
    }

    if (num != m_relay_multisign.size()) {
        xerror("xrelay_packer2::proc_vote_complate relay multisign not match with election.proposal:%s,match num:%u,multisign num:%u,xip=%s,election_round=%ld,local_electset=%zu",
               proposal_block->dump().c_str(),
               num,
               m_relay_multisign.size(),
               xcons_utl::xip_to_hex(get_xip2_addr()).c_str(),
               election_round,
               m_local_electset.size());
        return false;
    }

    std::string siggroup_str = top::to_string(siggroup.encodeBytes());
    xdbg("xrelay_packer2::proc_vote_complate.proposal:%s,sign num:%u, election num:%u", proposal_block->dump().c_str(), num, m_relay_multisign.size());
    proposal_block->set_extend_data(siggroup_str);
    return true;
}

bool xrelay_packer2::verify_commit_msg_extend_data(base::xvblock_t * block, const std::string & extend_data) {
    if ((block->get_cert()->get_consensus_flags() & base::enum_xconsensus_flag_extend_vote) == 0) {
        return true;
    }
    std::error_code ec;
    std::shared_ptr<data::xrelay_block> relay_block = data::xblockextract_t::unpack_relay_block_from_table(block, ec);    
    if (ec || nullptr == relay_block) {
        xwarn(
            "xrelay_proposal_maker_t:verify_commit_msg_extend_data fail-unpack relay block.block:%s,error %s; err msg %s", block->dump().c_str(), ec.category().name(), ec.message().c_str());
        return false;
    }

    data::xrelay_signature_group_t siggroup;
    siggroup.decodeBytes(top::to_bytes(extend_data), ec);
    if (ec) {
        xerror("xrelay_proposal_maker_t:verify_commit_msg_extend_data fail-decode extend data.block:%s,error %s; err msg %s", block->dump().c_str(), ec.category().name(), ec.message().c_str());
        return false;        
    }

    uint64_t election_round = siggroup.signature_epochID;
    relay_block->set_epochid(election_round);
    relay_block->set_viewid(block->get_viewid());
    uint256_t hash256 = from_bytes<uint256_t>(relay_block->build_signature_hash().to_bytes());

    auto local_xip = get_xip2_addr();
    xelection_cache_face::elect_set electset;
    get_elect_set(local_xip, electset);

    // check if multisignature is match with election data.
    if (siggroup.signature_vector.size() != electset.size()) {
        xerror("xrelay_packer2::verify_commit_msg_extend_data block:%s, multisign size not match:%u:%u", block->dump().c_str(), siggroup.signature_vector.size(), electset.size());
        return false;
    }

    uint32_t num = 0;
    for (uint32_t i = 0; i < siggroup.signature_vector.size(); i++) {
        if (false == siggroup.signature_vector[i].exist) {
            xdbg("xrelay_packer2::verify_commit_msg_extend_data,block:%s,signature[%u] is empty", block->dump().c_str(), i);
            continue;            
        }

        std::string signature_str = siggroup.signature_vector[i].signature.to_string();
        utl::xecdsasig_t signature_obj((uint8_t*)signature_str.c_str());
        uint8_t out_publickey_data[65] = {0};
        if (!top::utl::xsecp256k1_t::get_publickey_from_signature(signature_obj, hash256, out_publickey_data)) {
            xerror("xrelay_packer2::verify_commit_msg_extend_data get_publickey_from_signature fail,block:%s,i:%d", block->dump().c_str(), i);
            return false;
        }

        std::string raw_pub_key_str = base::xstring_utl::base64_decode(electset[i].election_info.consensus_public_key.to_string());
        xassert(raw_pub_key_str.size() == 65);
        top::utl::xecpubkey_t pub_key_obj(raw_pub_key_str);
        std::string pubkey_str = std::string((char *)(pub_key_obj.data()), pub_key_obj.size());
        if (std::string((char *)(out_publickey_data), 65) != pubkey_str) {
            xerror("xrelay_packer2::verify_commit_msg_extend_data pubkey not match,block:%s,i:%d", block->dump().c_str(), i);
            return false;
        }

        num++;
        xdbg("xrelay_packer2::verify_commit_msg_extend_data,block:%s,num:%u,signature[%u]", block->dump().c_str(), num, i);
    }
    // todo(nathan): signature number check.

    xdbg("xrelay_packer2::verify_commit_msg_extend_data ok,block:%s,elect size:%zu,sign num:%d", block->dump().c_str(), siggroup.signature_vector.size(), num);
    return true;
}

bool xrelay_packer2::get_election_round(const xvip2_t & xip, uint64_t & election_round) {
    auto network_proxy = get_resources()->get_network();
    if (network_proxy == nullptr) {
        xerror("xrelay_packer2::get_election_round network_proxy is null");
        return false;
    }

    if (!network_proxy->get_election_round(xip, election_round)) {
        xwarn("xrelay_packer2::get_election_round get_election_round fail.xip=%s",xcons_utl::xip_to_hex(xip).c_str());
        return false;
    }
    return true;    
}

void xrelay_packer2::get_elect_set(const xvip2_t & xip, xelection_cache_face::elect_set & elect_set) {
    auto election = get_resources()->get_election();
    election->get_election_cache_face()->get_election(xip, &elect_set);
}

bool xrelay_packer2::set_election_round(bool is_leader, data::xblock_consensus_para_t & proposal_para) {
    auto local_xip = get_xip2_addr();
    uint64_t election_round = 0;
    if (!get_election_round(local_xip, election_round)) {
        xerror("xrelay_packer2::set_election_round get_election_round fail.xip=%s",xcons_utl::xip_to_hex(local_xip).c_str());
        return false;
    }
    proposal_para.set_election_round(election_round);

    if (!is_leader) {
        return true;
    }

    // leader update cache, should not do for backup, because of multithread
    if (m_local_electset.size() > 0 && election_round <= m_election_round) {
        xdbg("xrelay_packer2::set_election_round no need update.xip=%s,round=%ld,%ld,electset=%zu",xcons_utl::xip_to_hex(local_xip).c_str(), election_round, m_election_round, m_local_electset.size());
        return true;
    }

    m_local_electset.clear();
    get_elect_set(local_xip, m_local_electset);

    for (auto & node : m_local_electset) {
        xdbg("xrelay_packer2::set_election_round pubkey=%s", node.election_info.consensus_public_key.to_string().c_str());
    }

    xdbg("xrelay_packer2::set_election_round update.xip=%s,round=%ld,electset=%zu",
        xcons_utl::xip_to_hex(local_xip).c_str(), election_round, m_local_electset.size());
    m_election_round = election_round;

    return true;
}

NS_END2
