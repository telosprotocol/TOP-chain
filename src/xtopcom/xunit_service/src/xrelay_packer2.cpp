// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xBFT/xconsevent.h"
#include "xunit_service/xrelay_packer2.h"
#include "xdata/xblockextract.h"
#include "xverifier/xverifier_utl.h"

#include <cinttypes>
NS_BEG2(top, xunit_service)

xrelay_packer2::xrelay_packer2(observer_ptr<mbus::xmessage_bus_face_t> const   &mb,
                           base::xtable_index_t &                          tableid,
                           const std::string &                             account_id,
                           std::shared_ptr<xcons_service_para_face> const &para,
                           std::shared_ptr<xblock_maker_face> const &      block_maker,
                           base::xcontext_t &                              _context,
                           uint32_t                                        target_thread_id) : xbatch_packer(mb, tableid, account_id, para, block_maker, _context, target_thread_id) {
    para->get_resources()->get_relay_chain_mgr()->start(target_thread_id);
    xunit_dbg("xrelay_packer2::xrelay_packer,create,this=%p,account=%s", this, account_id.c_str());
}

xrelay_packer2::~xrelay_packer2() {
    xunit_dbg("xrelay_packer2::~xrelay_packer,destory,this=%p", this); 
}

bool xrelay_packer2::close(bool force_async) {
    get_resources()->get_relay_chain_mgr()->stop();
    xcsaccount_t::close(force_async);
    xunit_dbg("xrelay_packer2::close, this=%p,refcount=%d", this, get_refcount());
    return true;
}

bool xrelay_packer2::on_timer_fire(const int32_t thread_id,
                                  const int64_t timer_id,
                                  const int64_t current_time_ms,
                                  const int32_t start_timeout_ms,
                                  int32_t & in_out_cur_interval_ms) {
    get_resources()->get_relay_chain_mgr()->on_timer();
    xbatch_packer::on_timer_fire(thread_id, timer_id, current_time_ms, start_timeout_ms, in_out_cur_interval_ms);
    xdbg("xrelay_packer2::on_timer_fire");
    return true;
}

void xrelay_packer2::set_vote_extend_data(base::xvblock_t * proposal_block, const uint256_t & hash, bool is_leader) {
    top::uint256_t hash_0;
    if (hash == hash_0) {
        return;
    }

    auto prikey_str = get_vcertauth()->get_prikey(get_xip2_addr());
    uint8_t priv_content[xverifier::PRIKEY_LEN];
    memcpy(priv_content, prikey_str.data(), prikey_str.size());
    top::utl::xecprikey_t ecpriv(priv_content);

    auto signature = ecpriv.sign(hash);
    std::string signature_str = std::string((char *)signature.get_compact_signature(), signature.get_compact_signature_size());
    xdbg("xrelay_packer2::set_vote_extend_data signer:%s", get_vcertauth()->get_signer(get_xip2_addr()).c_str());
    if (is_leader) {
        top::utl::xecpubkey_t pub_key_obj = ecpriv.get_public_key();
        std::string pubkey_str = std::string((char *)(pub_key_obj.data() + 1), 64);
        m_relay_multisign[pubkey_str] = std::make_pair(get_xip2_addr(), signature_str);
        m_relay_hash = hash;
    } else {
        proposal_block->set_vote_extend_data(signature_str);
    }

    // // for test
    // top::utl::xecpubkey_t pub_key_obj = ecpriv.get_public_key();

    // uint8_t signature_content[65];
    // memcpy(signature_content, signature_str.data(), signature_str.size());

    // utl::xecdsasig_t signature1(signature_content);
    // bool verify_ret = pub_key_obj.verify_signature(signature1, hash);
    // if (verify_ret) {
    //     xdbg("nathan test verify_signature ok");
    // } else {
    //     xerror("nathan test verify_signature fail");
    // }
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

    result = std::string((char *)(out_publickey_data + 1), 64);
    xdbg("xrelay_packer2::verify_vote_extend_data verify sign succ.proposal_block:%s,signer:%s", proposal_block->dump().c_str(), account_addr_of_node.c_str());
    return true;
}

void xrelay_packer2::add_vote_extend_data(base::xvblock_t * proposal_block, const xvip2_t & replica_xip, const std::string & vote_extend_data, const std::string & result) {
    if ((proposal_block->get_cert()->get_consensus_flags() & base::enum_xconsensus_flag_extend_vote) == 0) {
        return;
    }

    m_relay_multisign[result] = std::make_pair(replica_xip, vote_extend_data);
    xdbg("xrelay_packer2::add_vote_extend_data proposal:%s m_inner_voted_datas size:%u", proposal_block->dump().c_str(), m_relay_multisign.size());
}

bool xrelay_packer2::proc_vote_complate(base::xvblock_t * proposal_block) {
    if ((proposal_block->get_cert()->get_consensus_flags() & base::enum_xconsensus_flag_extend_vote) == 0) {
        return true;
    }

    if (m_relay_multisign.empty()) {
        xerror("xrelay_packer2::proc_vote_complate m_relay_multisign is empty,proposal:%s", proposal_block->dump().c_str());
        return false;
    }
    auto network_proxy = get_resources()->get_network();
    if (network_proxy == nullptr) {
        xerror("xrelay_packer2::proc_vote_complate network_proxy is null,proposal:%s", proposal_block->dump().c_str());
        return false;
    }

    std::error_code ec;
    data::xrelay_block relay_block;
    data::xblockextract_t::unpack_relayblock(proposal_block, false, relay_block, ec);
    if (ec) {
        xwarn("xrelay_packer2::proc_vote_complate last_relay_block decodeBytes,proposal:%s,error %s; err msg %s",
              proposal_block->dump().c_str(),
              ec.category().name(),
              ec.message().c_str());
        return false;
    }
    uint64_t election_round = relay_block.get_inner_header().get_epochID();

    // order multisign by election info.
    std::vector<data::xrelay_election_node_t> reley_election;
    auto ret = get_resources()->get_relay_chain_mgr()->get_elect_cache(election_round, reley_election);
    if (!ret) {
        xwarn("xrelay_packer2::proc_vote_complate get elect cache fail.proposal:%s,round:%lu", proposal_block->dump().c_str(), election_round);
        return false;
    }

    base::xstream_t _stream(base::xcontext_t::instance());
    uint16_t size = reley_election.size();
    _stream << size;

    uint32_t num = 0;
    uint32_t i = 0;
    for (auto & node : reley_election) {
        auto it = m_relay_multisign.find(node.get_pubkey_str());
        if (it == m_relay_multisign.end()) {
            std::string empty_str = "";
            _stream << empty_str;
            xdbg("xrelay_packer2::proc_vote_complate not found in election.proposal:%s,elect idx:%u", proposal_block->dump().c_str(), i);
        } else {
            _stream << it->second.second;
            num++;
            xdbg("xrelay_packer2::proc_vote_complate found in election.proposal:%s,elect idx:%u", proposal_block->dump().c_str(), i);
        }
        i++;
    }
    if (num != m_relay_multisign.size()) {
        xerror("xrelay_packer2::proc_vote_complate relay multisign not match with election.proposal:%s,match num:%u, multisign num:%u",
               proposal_block->dump().c_str(),
               num,
               m_relay_multisign.size());
        return false;
    }

    xdbg("xrelay_packer2::proc_vote_complate.proposal:%s,sign num:%u, election num:%u", proposal_block->dump().c_str(), num, reley_election.size());
    std::string extend_data = std::string((char *)_stream.data(), _stream.size());
    proposal_block->set_extend_data(extend_data);
    return true;
}

bool xrelay_packer2::verify_commit_msg_extend_data(base::xvblock_t * block, const std::string & extend_data) {
    if ((block->get_cert()->get_consensus_flags() & base::enum_xconsensus_flag_extend_vote) == 0) {
        return true;
    }
    std::error_code ec;
    data::xrelay_block relay_block;
    data::xblockextract_t::unpack_relayblock(block, false, relay_block, ec);
    if (ec) {
        xwarn(
            "xrelay_proposal_maker_t:make_proposal last_relay_block decodeBytes block:%s,error %s; err msg %s", block->dump().c_str(), ec.category().name(), ec.message().c_str());
        return false;
    }

    uint64_t election_round = relay_block.get_inner_header().get_epochID();
    auto hash = relay_block.get_block_hash();
    uint256_t hash256 = from_bytes<uint256_t>(hash.to_bytes());

    std::vector<data::xrelay_election_node_t> reley_election;
    auto ret = get_resources()->get_relay_chain_mgr()->get_elect_cache(election_round, reley_election);
    if (!ret) {
        xwarn("xrelay_packer2::verify_commit_msg_extend_data get elect cache fail.block:%s,round:%lu", block->dump().c_str(), election_round);
        return false;
    }

    // check if multisignature is match with election data.
    base::xstream_t stream{base::xcontext_t::instance(), (uint8_t *)extend_data.data(), static_cast<uint32_t>(extend_data.size())};
    uint16_t size = 0;
    stream >> size;
    if (size != reley_election.size()) {
        xerror("xrelay_packer2::verify_commit_msg_extend_data block:%,smultisign size not match:%u:%u", block->dump().c_str(), size, reley_election.size());
        return false;
    }

    uint32_t num = 0;
    for (uint16_t i = 0; i < size; i++) {
        std::string signature;
        stream >> signature;

        if (signature == "") {
            xdbg("xrelay_packer2::verify_commit_msg_extend_data,block:%,signature[%u] is empty", block->dump().c_str(), i);
            continue;
        }

        utl::xecdsasig_t signature_obj((uint8_t *)signature.c_str());
        uint8_t out_publickey_data[65] = {0};
        if (!top::utl::xsecp256k1_t::get_publickey_from_signature(signature_obj, hash256, out_publickey_data)) {
            xerror("xrelay_packer2::verify_commit_msg_extend_data get_publickey_from_signature fail,block:%s,i:%d", block->dump().c_str(), i);
            return false;
        }

        if (std::string((char *)(out_publickey_data + 1), 64) != reley_election[i].get_pubkey_str()) {
            xerror("xrelay_packer2::verify_commit_msg_extend_data pubkey not match,block:%s,i:%d", block->dump().c_str(), i);
            return false;
        }

        num++;
        xdbg("xrelay_packer2::verify_commit_msg_extend_data,block:%s,num:%u,signature[%u]:%s", block->dump().c_str(), num, i, signature.c_str());
    }
    // todo(nathan): signature number check.

    xdbg("xrelay_packer2::verify_commit_msg_extend_data ok,block:%s,elect size:%d,sign num:%d", block->dump().c_str(), size, num);
    return true;
}

bool xrelay_packer2::set_election_round(data::xblock_consensus_para_t & proposal_para) {
    auto network_proxy = get_resources()->get_network();
    if (network_proxy == nullptr) {
        xerror("xrelay_packer2::set_election_round network_proxy is null");
        return false;
    }

    uint64_t election_round = 0;
    if (!network_proxy->get_election_round(get_xip2_addr(), election_round)) {
        xerror("xrelay_packer2::set_election_round get_election_round fail");
        return false;
    }

    proposal_para.set_election_round(election_round);
    return false;
}

NS_END2
