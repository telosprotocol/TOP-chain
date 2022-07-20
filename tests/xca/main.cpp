//
//  main.cpp
//  xBFT-basic-test
//
#include "xbase/xhash.h"
#include "xbase/xutl.h"
// TODO(jimmy) #include "xbase/xvledger.h"

#include "xutility/xhash.h"
#include "xunitblock.hpp"

#include "xcrypto/xckey.h"
#include "xcrypto/xcrypto_util.h"
#include "xcertauth/xcertauth_face.h"

#include <limits.h>
#include <inttypes.h>

using namespace top;
using namespace top::test;

namespace top
{
    class xhashtest_t : public base::xhashplugin_t
    {
    public:
        xhashtest_t()
            :base::xhashplugin_t(-1) //-1 = support every hash types
        {
        }
    private:
        xhashtest_t(const xhashtest_t &);
        xhashtest_t & operator = (const xhashtest_t &);
        virtual ~xhashtest_t(){};
    public:
        virtual const std::string hash(const std::string & input,enum_xhash_type type) override
        {
            const uint256_t hash_to_sign = utl::xsha2_256_t::digest(input);
            return std::string((const char*)hash_to_sign.data(),hash_to_sign.size());
            //return base::xstring_utl::tostring(base::xhash64_t::digest(input));
        }
    };
}

class xtimeout_header_t : public base::xvheader_t {
protected:
    virtual ~xtimeout_header_t() {}

public:
    xtimeout_header_t(uint32_t chainid, std::string const & account) {
        set_chainid(chainid);
        set_account(account);
        set_height(1);
        set_block_level(base::enum_xvblock_level_unit);
        set_block_class(base::enum_xvblock_class_nil);
        set_last_block_hash("0"); // NOT empty
        set_last_full_block("0", 0);
        // set_nonce(-1);
    }
};

class xtimeout_qcert_t : public base::xvqcert_t {
protected:
    virtual ~xtimeout_qcert_t() {}

public:
    xtimeout_qcert_t() : base::xvqcert_t("") {
        set_consensus_type(base::enum_xconsensus_type_xhbft);                    // test pBFT
        set_consensus_threshold(base::enum_xconsensus_threshold_2_of_3);         // >= 2/3 vote
        set_crypto_key_type(base::enum_xvchain_key_curve_secp256k1);             // default one
        set_crypto_sign_type(base::enum_xvchain_threshold_sign_scheme_schnorr);  // default schnorr scheme
        set_crypto_hash_type(enum_xhash_type_sha2_256);                          // default sha2_256
        set_clock(1);
        set_viewtoken(-1);
    }
};

class xtimecert_block_t : public base::xvblock_t {
protected:
    virtual ~xtimecert_block_t() {}

public:
    xtimecert_block_t() : base::xvblock_t((enum_xdata_type)base::enum_xobject_type_vblock) {}
    xtimecert_block_t(base::xvheader_t & _vheader, base::xvqcert_t & _vcert) : base::xvblock_t(_vheader, _vcert, nullptr, nullptr) {}

    bool is_done() {
        if (m_validators.size() < get_cert()->get_validator_threshold()) {
            return false;
        }

        if (m_auditors.size() < get_cert()->get_auditor_threshold()) {
            return false;
        }
        return true;
    }

    bool add_qcert(const xvip2_t& replica_xip, base::xvqcert_t * qcert_ptr)  {
        if ((qcert_ptr != NULL) && (replica_xip.low_addr != 0))  // sanity test first
        {
            if (get_cert()->is_validator(replica_xip)) {
                auto it = m_validators.find(replica_xip);
                if (it == m_validators.end())  // new one
                {
                    std::string qcert_bin = qcert_ptr->get_verify_signature();
                    m_validators[replica_xip] = qcert_bin;

                    return true;
                }
            } else if (get_cert()->is_auditor(replica_xip)) {
                auto it = m_auditors.find(replica_xip);
                if (it == m_auditors.end())  // new one
                {
                    std::string qcert_bin = qcert_ptr->get_verify_signature();
                    m_auditors[replica_xip] = qcert_bin;

                    return true;
                }
            }
        }
        return false;
    }

    void update_multi_sign(base::xvcertauth_t * certauth) {
        // update validator cert
        xdbg("[xtimecert_block_t::update_multi_sign] with validators %d, qcert=%s", m_validators.size(), get_cert()->dump().c_str());
        std::string sign = certauth->merge_muti_sign(m_validators, get_cert());
        reset_block_flags();
        set_verify_signature(sign);

        // assert(certauth->verify_muti_sign(this));
        xdbg("[xtimecert_block_t::update_multi_sign] verify muti_sign pass");

        // update auditor cert if need
        if (get_cert()->get_auditor_threshold() > 0) {
            sign = certauth->merge_muti_sign(m_auditors, get_cert());
            this->set_audit_signature(sign);
        }

        set_block_flag(base::enum_xvblock_flag_authenticated);
    }

protected:
    std::map<xvip2_t,std::string,xvip2_compare> m_validators;
    std::map<xvip2_t,std::string,xvip2_compare> m_auditors;
};

xtimecert_block_t * create_timeout_block(uint64_t view, std::string const& account, xvip2_t const& xip, xvip2_t const& target_xip, base::xvcertauth_t* certauth) {
    auto header = new xtimeout_header_t(0, account);
    auto qcert = new xtimeout_qcert_t();
    auto cache = new xtimecert_block_t(*header, *qcert);
    header->release_ref();
    qcert->release_ref();

    cache->get_cert()->set_viewid(view);
    cache->get_cert()->set_validator(target_xip);

    // sign
    auto sign = certauth->do_sign(xip, cache, base::xtime_utl::get_fast_random64());
    cache->set_verify_signature(sign);
    cache->set_block_flag(base::enum_xvblock_flag_authenticated);

    // add self vote
    // assert(certauth->verify_sign(xip, cache));
    xdbg("[xconspacemaker_t::create_timeout_block] verify sign pass");
    cache->add_qcert(xip, qcert);

    xdbg("dump timeout cert %s", cache->dump().c_str());

    return cache;
}

int test_ca_api()
{
    utl::xecprikey_t sec256k1_private_key;
    const std::string target_account = sec256k1_private_key.to_account_address('0', 0);
    {
        base::xvnodehouse_t* _nodesvr_ptr = new base::xvnodehouse_t();

        const int  _total_nodes = 5;
        xvip2_t _shard_xipaddr = {0};
        _shard_xipaddr.high_addr = (((uint64_t)_total_nodes) << 54) | 1; //encode node'size of group
        _shard_xipaddr.low_addr  = 1 << 10; //at group#1

        std::vector<base::xvnode_t*> _consensus_nodes;
        std::vector<xtimecert_block_t*> tcs;
        for(uint32_t i = 0; i < _total_nodes; ++i)
        {
            xvip2_t node_addr;
            node_addr.high_addr = _shard_xipaddr.high_addr;
            node_addr.low_addr  = _shard_xipaddr.low_addr | i;

            utl::xecprikey_t node_prv_key;
            std::string _node_prv_key_str((const char*)node_prv_key.data(),node_prv_key.size());
            std::string _node_pub_key_str = node_prv_key.get_compress_public_key();
            xdbg("\"%s\" = \"%s\"", base::xstring_utl::base64_encode((const unsigned char *)_node_prv_key_str.data(), _node_prv_key_str.size()).c_str(), base::xstring_utl::base64_encode((const unsigned char *)_node_pub_key_str.data(), _node_pub_key_str.size()).c_str());
            const std::string node_account  = node_prv_key.to_account_address('0', 0);

            _consensus_nodes.push_back(new base::xvnode_t(node_account,node_addr,_node_pub_key_str,_node_prv_key_str));
        }
        base::xauto_ptr<base::xvnodegroup_t> _consensus_group(new base::xvnodegroup_t(_shard_xipaddr,0,_consensus_nodes));
        _nodesvr_ptr->add_group(_consensus_group.get());

        auto xip = _consensus_nodes[0]->get_xip2_addr();
        reset_node_id_to_xip2(xip);
        set_node_id_to_xip2(xip, 0xFFF);
        for(uint32_t i = 0; i < _total_nodes; ++i) {
            tcs.push_back(create_timeout_block(1, _consensus_nodes[0]->get_account(), _consensus_nodes[i]->get_xip2_addr(), xip, &auth::xauthcontext_t::instance(*_nodesvr_ptr)));
        }

        std::string empty_tx;
        xunitblock_t* target_block = xunitblock_t::create_unitblock(target_account,1,1,1,std::string("0"),std::string("0"),0,empty_tx,empty_tx);
        target_block->get_cert()->set_validator(_consensus_nodes[0]->get_xip2_addr());

        //auto tc = create_timeout_block(1, _consensus_nodes[0]->get_account(), _consensus_nodes[0]->get_xip2_addr(), _consensus_nodes[0]->get_xip2_addr(), &auth::xauthcontext_t::instance(*_nodesvr_ptr));

        //do_sign & do_verify
        {
            const std::string _signature = auth::xauthcontext_t::instance(*_nodesvr_ptr).do_sign(_consensus_nodes[0]->get_xip2_addr(), target_block, 0);
            target_block->set_verify_signature(_signature);

            // xassert(auth::xauthcontext_t::instance(*_nodesvr_ptr).verify_sign(_consensus_nodes[0]->get_xip2_addr(), target_block->get_cert()));
        }

        //do muti-sign/verify
        {
            /*std::vector<xvip2_t>     _nodes_xvip2;
            std::vector<std::string> _nodes_signatures;

            // (2 * 4) / 3 + 1 = 3
            _nodes_xvip2.push_back(_consensus_nodes[2]->get_xip2_addr());
            _nodes_xvip2.push_back(_consensus_nodes[0]->get_xip2_addr());
            _nodes_xvip2.push_back(_consensus_nodes[1]->get_xip2_addr());
            _nodes_xvip2.push_back(_consensus_nodes[4]->get_xip2_addr());

            _nodes_signatures.push_back(auth::xauthcontext_t::instance(*_nodesvr_ptr).do_sign(_nodes_xvip2[2], target_block, 0));
            _nodes_signatures.push_back(auth::xauthcontext_t::instance(*_nodesvr_ptr).do_sign(_nodes_xvip2[0], target_block, 0));
            _nodes_signatures.push_back(auth::xauthcontext_t::instance(*_nodesvr_ptr).do_sign(_nodes_xvip2[1], target_block, 0));
            _nodes_signatures.push_back(auth::xauthcontext_t::instance(*_nodesvr_ptr).do_sign(_nodes_xvip2[4], target_block, 0));*/

            std::map<xvip2_t,std::string,xvip2_compare> _map;
            _map[_consensus_nodes[2]->get_xip2_addr()] = tcs[2]->get_cert()->get_verify_signature();
                                                        //auth::xauthcontext_t::instance(*_nodesvr_ptr).do_sign(_consensus_nodes[2]->get_xip2_addr(), tc, base::xtime_utl::get_fast_random64());
            _map[_consensus_nodes[0]->get_xip2_addr()] = tcs[0]->get_cert()->get_verify_signature();
                                                        //auth::xauthcontext_t::instance(*_nodesvr_ptr).do_sign(_consensus_nodes[0]->get_xip2_addr(), tc, base::xtime_utl::get_fast_random64());
            _map[_consensus_nodes[1]->get_xip2_addr()] = tcs[1]->get_cert()->get_verify_signature();
                                                        //auth::xauthcontext_t::instance(*_nodesvr_ptr).do_sign(_consensus_nodes[1]->get_xip2_addr(), tc, base::xtime_utl::get_fast_random64());
            _map[_consensus_nodes[4]->get_xip2_addr()] = tcs[4]->get_cert()->get_verify_signature();
                                                        //auth::xauthcontext_t::instance(*_nodesvr_ptr).do_sign(_consensus_nodes[4]->get_xip2_addr(), tc, base::xtime_utl::get_fast_random64());

            const std::string _muti_signature = auth::xauthcontext_t::instance(*_nodesvr_ptr).merge_muti_sign(_map, tcs[0]->get_cert());
            tcs[0]->reset_block_flags();
            tcs[0]->set_verify_signature(_muti_signature);

            // xassert(auth::xauthcontext_t::instance(*_nodesvr_ptr).verify_muti_sign(tcs[0]->get_cert())); // error: no matching function for call to ‘top::base::xvcertauth_t::verify_muti_sign(top::base::xvqcert_t*)’
            tcs[0]->set_block_flag(base::enum_xvblock_flag_authenticated); //then add flag of auth

            /*tcs[0]->add_qcert(_consensus_nodes[4]->get_xip2_addr(), tcs[4]->get_cert());
            tcs[0]->add_qcert(_consensus_nodes[1]->get_xip2_addr(), tcs[1]->get_cert());
            tcs[0]->add_qcert(_consensus_nodes[2]->get_xip2_addr(), tcs[2]->get_cert());
            assert(tcs[0]->is_done());
            tcs[0]->update_multi_sign(&auth::xauthcontext_t::instance(*_nodesvr_ptr));*/
        }

        target_block->release_ref();
        _nodesvr_ptr->remove_group(_consensus_group->get_xip2_addr());
        _nodesvr_ptr->release_ref();
        for(auto it : _consensus_nodes)
            it->release_ref();
    }
    return 0;
}

//#define __network_simulator_test__
int main(int argc, const char * argv[])
{
#ifdef __WIN_PLATFORM__
    xinit_log("C:\\Users\\taylo\\Downloads\\", true, true);
#else
    xinit_log("/tmp/",true,true);
#endif

#ifdef DEBUG
    xset_log_level(enum_xlog_level_debug);
#else
    //xset_log_level(enum_xlog_level_debug);
    xset_log_level(enum_xlog_level_key_info);
#endif

    new top::xhashtest_t(); //register this plugin into xbase

    test_ca_api();

    return 0;
}
