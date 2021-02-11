//
//  main.cpp
//  xBFT-basic-test
//
#include "xbase/xhash.h"
#include "xbase/xutl.h"
#include "xbase/xvledger.h"

#include "xutility/xhash.h"
#include "xunitblock.hpp"

#include "xmutisig/xmutisig.h"
#include "xcrypto/xckey.h"
#include "xcrypto/xcrypto_util.h"
#include "xcertauth/xcertauth_face.h"

#include <limits.h>

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

int test_ca_api()
{
    utl::xecprikey_t sec256k1_private_key;
    const std::string target_account = sec256k1_private_key.to_account_address('0', 0);
    {
        base::xvnodehouse_t* _nodesvr_ptr = new base::xvnodehouse_t();
        
        const int  _total_nodes = 4;
        xvip2_t _shard_xipaddr = {0};
        _shard_xipaddr.high_addr = (((uint64_t)_total_nodes) << 54) | 1; //encode node'size of group
        _shard_xipaddr.low_addr  = 1 << 10; //at group#1
        
        std::vector<base::xvnode_t*> _consensus_nodes;
        for(uint32_t i = 0; i < _total_nodes; ++i)
        {
            xvip2_t node_addr;
            node_addr.high_addr = _shard_xipaddr.high_addr;
            node_addr.low_addr  = _shard_xipaddr.low_addr | i;
            
            utl::xecprikey_t node_prv_key;
            std::string _node_prv_key_str((const char*)node_prv_key.data(),node_prv_key.size());
            std::string _node_pub_key_str = node_prv_key.get_compress_public_key();
            const std::string node_account  = node_prv_key.to_account_address('0', 0);
            
            _consensus_nodes.push_back(new base::xvnode_t(node_account,node_addr,_node_pub_key_str,_node_prv_key_str));
        }
        base::xauto_ptr<base::xvnodegroup_t> _consensus_group(new base::xvnodegroup_t(_shard_xipaddr,0,_consensus_nodes));
        _nodesvr_ptr->add_group(_consensus_group.get());

        std::string empty_tx;
        xunitblock_t* target_block = xunitblock_t::create_unitblock(target_account,1,1,1,std::string("0"),std::string("0"),0,empty_tx,empty_tx);
        target_block->get_cert()->set_validator(_consensus_nodes[0]->get_xip2_addr());
        
        //do_sign & do_verify
        {
            const std::string _signature = auth::xauthcontext_t::instance(*_nodesvr_ptr).do_sign(_consensus_nodes[0]->get_xip2_addr(), target_block, 0);
            target_block->set_verify_signature(_signature);
 
            xassert(auth::xauthcontext_t::instance(*_nodesvr_ptr).verify_sign(_consensus_nodes[0]->get_xip2_addr(), target_block->get_cert()));
        }
        
        //do muti-sign/verify
        {
            std::vector<xvip2_t>     _nodes_xvip2;
            std::vector<std::string> _nodes_signatures;
  
            // (2 * 4) / 3 + 1 = 3
            _nodes_xvip2.push_back(_consensus_nodes[0]->get_xip2_addr());
            _nodes_xvip2.push_back(_consensus_nodes[1]->get_xip2_addr());
            _nodes_xvip2.push_back(_consensus_nodes[2]->get_xip2_addr());
 
            _nodes_signatures.push_back(auth::xauthcontext_t::instance(*_nodesvr_ptr).do_sign(_nodes_xvip2[0], target_block, 0));
            _nodes_signatures.push_back(auth::xauthcontext_t::instance(*_nodesvr_ptr).do_sign(_nodes_xvip2[1], target_block, 0));
            _nodes_signatures.push_back(auth::xauthcontext_t::instance(*_nodesvr_ptr).do_sign(_nodes_xvip2[2], target_block, 0));
            
            const std::string _muti_signature = auth::xauthcontext_t::instance(*_nodesvr_ptr).merge_muti_sign(_nodes_xvip2, _nodes_signatures, target_block->get_cert());
            target_block->set_verify_signature(_muti_signature);
            
            xassert(auth::xauthcontext_t::instance(*_nodesvr_ptr).verify_muti_sign(target_block->get_cert()));
            target_block->set_block_flag(base::enum_xvblock_flag_authenticated); //then add flag of auth
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
