#pragma once

#include "xbase/xhash.h"
#include "xbase/xutl.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xutility/xhash.h"
#include "xcrypto/xckey.h"
#include "xcrypto/xcrypto_util.h"
#include "xcertauth/xcertauth_face.h"
#include "xdata/xtableblock.h"
#include "xrpc/xuint_format.h"
#include "xsafebox/safebox_proxy.h"

#include <random>
#include <cinttypes>

using namespace top::base;

namespace top {
namespace mock {
class xcertauth_util
{
private:
    base::xvnodehouse_t* m_nodesvr{nullptr};
    std::vector<base::xvnode_t*> m_consensus_nodes;
    base::xvnodegroup_t* m_consensus_group;
    size_t m_node_count;

    xcertauth_util(size_t node_count) : m_node_count(node_count) {
        m_nodesvr = new base::xvnodehouse_t();

        xvip2_t _shard_xipaddr = {0};
        _shard_xipaddr.high_addr = (((uint64_t)node_count) << 54) | 1; //encode node'size of group
        _shard_xipaddr.low_addr  = 1 << 10; //at group#1

        for(size_t i = 0; i < node_count; ++i) {
            xvip2_t node_addr;
            node_addr.high_addr = _shard_xipaddr.high_addr;
            node_addr.low_addr  = _shard_xipaddr.low_addr | i;

            xdbg("xcertauth_util::xcertauth_util,nodes for signer(%" PRIx64 " : %" PRIx64 " )",node_addr.high_addr,node_addr.low_addr);

            utl::xecprikey_t node_prv_key;
            std::string _node_prv_key_str((const char*)node_prv_key.data(),node_prv_key.size());
            std::string _node_pub_key_str = node_prv_key.get_compress_public_key();
            
            safebox::xsafebox_proxy::get_instance().add_key_pair(xpublic_key_t{_node_pub_key_str}, std::move(_node_prv_key_str));

            const std::string node_account = node_prv_key.to_account_address('0', 0);
            m_consensus_nodes.push_back(new base::xvnode_t(node_account, node_addr, _node_pub_key_str));
        }
        m_consensus_group = new base::xvnodegroup_t(_shard_xipaddr,0,m_consensus_nodes);
        m_nodesvr->add_group(m_consensus_group);
        auth::xauthcontext_t::instance(*m_nodesvr);
    }

public:
    static xcertauth_util& instance(size_t node_count = 4)
    {
        static xcertauth_util xca(node_count);
        return xca;
    }

    ~xcertauth_util() {
        m_nodesvr->remove_group(m_consensus_group->get_xip2_addr());
        m_nodesvr->release_ref();
        for(auto it : m_consensus_nodes)
            it->release_ref();
        m_consensus_group->release_ref();
    }

    base::xvcertauth_t& get_certauth() {
        return auth::xauthcontext_t::instance(*m_nodesvr);
    }

    void do_sign(base::xvblock_t* block) {
        xassert(block != nullptr);
        std::random_device rd;
        std::uniform_int_distribution<size_t> dist(0, m_node_count - 1);
        xvip2_t node = m_consensus_nodes[dist(rd)]->get_xip2_addr();
        block->get_cert()->set_validator(node);
        std::string signature = get_certauth().do_sign(node, block->get_cert(),base::xtime_utl::get_fast_random64());
        block->set_verify_signature(signature);
        xassert(base::enum_vcert_auth_result::enum_successful == get_certauth().verify_sign(node, block->get_cert(),block->get_account()));
        block->set_block_flag(base::enum_xvblock_flag_authenticated);
    }

    // use node 0 as default leader
    xvip2_t get_leader_xip() {
        return m_consensus_nodes[0]->get_xip2_addr();
    }

    void do_multi_sign(base::xvblock_t* block, bool is_fixed = false) {
        xassert(block != nullptr);
        if (is_fixed) {
            std::string hex_sig = "0x740000005800000002000000000000002100025701e98c80198da9c2c620866ec4eff40ff81512bd09e86993621a7a10fe2ea3200086e97a10412fd226d2fa066a981cb7877d6931490f4fc089c1e89e159eeed60b04000f";
            std::string muti_signature = xrpc::hex_to_uint8_str(hex_sig);
            block->set_verify_signature(muti_signature);
            block->set_block_flag(base::enum_xvblock_flag_authenticated);
            return;
        }
#if 0
        std::random_device rd;
        std::uniform_int_distribution<size_t> dist(1, m_node_count - 1);
        xvip2_t byzantine_node = m_consensus_nodes[dist(rd)]->get_xip2_addr();
#endif

        auto tableblock = dynamic_cast<data::xtable_block_t*>(block);
        if (tableblock == nullptr) {
            block->get_cert()->set_validator(get_leader_xip());
        }

        std::vector<xvip2_t>     nodes_xvip2;
        std::vector<std::string> nodes_signatures;
        for (size_t i = 0; i < m_node_count; ++i) {
            xvip2_t sign_node = m_consensus_nodes[i]->get_xip2_addr();
#if 0
            if (is_xip2_equal(sign_node, byzantine_node)) {
                continue;
            }
#endif
            nodes_xvip2.push_back(sign_node);
            nodes_signatures.push_back(get_certauth().do_sign(sign_node, block, base::xtime_utl::get_fast_random64()));
        }

        std::string muti_signature = get_certauth().merge_muti_sign(nodes_xvip2, nodes_signatures, block->get_cert());
        // std::cout << "muti_signature " << to_hex_str(muti_signature) << std::endl;
        block->set_verify_signature(muti_signature);

        xassert(base::enum_vcert_auth_result::enum_successful == get_certauth().verify_muti_sign(block->get_cert(),block->get_account()));
        block->set_block_flag(base::enum_xvblock_flag_authenticated);
    }
};

}
}
