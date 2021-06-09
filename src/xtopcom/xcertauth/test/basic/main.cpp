#include "xbase/xhash.h"
#include "xbase/xutl.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xutility/xhash.h"
#include "xmutisig/xmutisig.h"
#include "xcrypto/xckey.h"
#include "xcrypto/xcrypto_util.h"
#include "xcertauth/xcertauth_face.h"
#include "xunitblock.hpp"
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

int test_ckey(const char addr_type,const uint16_t ledger_id,const int test_round)
{
    for(int i = 0; i < test_round; ++i)
    {
        utl::xecprikey_t  random_data; //using xecprikey_t generate random data
        uint256_t msg_digest((uint8_t*)random_data.data());
        
        utl::xecprikey_t raw_pri_key_obj;
        utl::xecpubkey_t raw_pub_key_obj = raw_pri_key_obj.get_public_key();
        const std::string uncompressed_pub_key_data((const char*)raw_pub_key_obj.data(),raw_pub_key_obj.size());
        const std::string compressed_pub_key_data = raw_pri_key_obj.get_compress_public_key();
        const std::string account_addr_from_raw_pri_key = raw_pri_key_obj.to_account_address(addr_type, ledger_id);
                
        utl::xecdsasig_t signature_obj = raw_pri_key_obj.sign(msg_digest);
        const std::string signature = utl::xcrypto_util::digest_sign(msg_digest,raw_pri_key_obj.data());
        xassert(utl::xcrypto_util::verify_sign(msg_digest,signature,account_addr_from_raw_pri_key));
       
        
        //test org pub key
        {
            const std::string account_addr_from_raw_pub_key = raw_pub_key_obj.to_address(addr_type, ledger_id);
            xassert(account_addr_from_raw_pri_key == account_addr_from_raw_pub_key);
            xassert(raw_pub_key_obj.verify_signature(signature_obj, msg_digest));
        }
        //test uncompressed key from serialization(65 bytes)
        {
            utl::xecpubkey_t uncompress_pub_key_obj(uncompressed_pub_key_data);
            const std::string account_addr_from_uncompress_pub_key = uncompress_pub_key_obj.to_address(addr_type, ledger_id);
            xassert(account_addr_from_raw_pri_key == account_addr_from_uncompress_pub_key);
            xassert(uncompress_pub_key_obj.verify_signature(signature_obj, msg_digest));
        }
        //test uncompressed key from serialization(64 bytes)
        {
            utl::xecpubkey_t uncompress_pub_key_obj((uint8_t*)uncompressed_pub_key_data.data() + 1, (int)uncompressed_pub_key_data.size() - 1);
            const std::string account_addr_from_uncompress_pub_key = uncompress_pub_key_obj.to_address(addr_type, ledger_id);
            xassert(account_addr_from_raw_pri_key == account_addr_from_uncompress_pub_key);
            xassert(uncompress_pub_key_obj.verify_signature(signature_obj, msg_digest));
        }
        
        //test compressed pub key
        {
            utl::xecpubkey_t compressed_pub_key_obj(compressed_pub_key_data);
            const std::string account_addr_from_compressed_pub_key = compressed_pub_key_obj.to_address(addr_type, ledger_id);
            xassert(account_addr_from_raw_pri_key == account_addr_from_compressed_pub_key);
            xassert(compressed_pub_key_obj.verify_signature(signature_obj, msg_digest));
        }
 
    }
    
    //test bad/modified private key
    {
        utl::xecprikey_t  random_data; //using xecprikey_t generate random data
        uint256_t msg_digest((uint8_t*)random_data.data());
        
        utl::xecprikey_t good_pri_key_obj;
        utl::xecpubkey_t good_pub_key_obj = good_pri_key_obj.get_public_key();
        const std::string account_addr_from_raw_pri_key = good_pri_key_obj.to_account_address(addr_type, ledger_id);
        
        uint8_t bad_priv_key[32];
        memcpy(bad_priv_key, good_pri_key_obj.data(), good_pri_key_obj.size());
        bad_priv_key[3] = 0;
        bad_priv_key[9] = 0x3f; //random value
        utl::xecprikey_t bad_pri_key_obj(bad_priv_key);
        
        utl::xecdsasig_t signature_obj = bad_pri_key_obj.sign(msg_digest);
        xassert(good_pub_key_obj.verify_signature(signature_obj, msg_digest) == false);
        
        const std::string bad_signature((char*)(signature_obj.get_compact_signature()), signature_obj.get_compact_signature_size());
        xassert(utl::xcrypto_util::verify_sign(msg_digest,bad_signature,account_addr_from_raw_pri_key) == false);
        
    }
    
    //test bad public key
    {
        utl::xecprikey_t  random_data; //using xecprikey_t generate random data
        uint256_t msg_digest((uint8_t*)random_data.data());
        
        utl::xecprikey_t good_pri_key_obj;
        utl::xecpubkey_t good_pub_key_obj = good_pri_key_obj.get_public_key();
        
        utl::xecdsasig_t signature_obj = good_pri_key_obj.sign(msg_digest);
        const std::string signature((char*)(signature_obj.get_compact_signature()), signature_obj.get_compact_signature_size());
        
        uint8_t bad_pub_key[65];
        memcpy(bad_pub_key, good_pub_key_obj.data(), good_pub_key_obj.size());
        bad_pub_key[base::xtime_utl::get_fast_randomu() % 65] = base::xtime_utl::get_fast_randomu() % 255;
        bad_pub_key[base::xtime_utl::get_fast_randomu() % 65] = base::xtime_utl::get_fast_randomu() % 255;
        utl::xecpubkey_t  bad_pub_key_obj(bad_pub_key);
        const std::string bad_account_addr = bad_pub_key_obj.to_address(addr_type, ledger_id);
        
        xassert(bad_pub_key_obj.verify_signature(signature_obj, msg_digest) == false);
        xassert(utl::xcrypto_util::verify_sign(msg_digest,signature,bad_account_addr) == false);
    }
    
    //bad account address
    {
        utl::xecprikey_t  random_data; //using xecprikey_t generate random data
        uint256_t msg_digest((uint8_t*)random_data.data());
        
        utl::xecprikey_t good_pri_key_obj;
        utl::xecpubkey_t good_pub_key_obj = good_pri_key_obj.get_public_key();
        
        utl::xecdsasig_t signature_obj = good_pri_key_obj.sign(msg_digest);
        const std::string signature((char*)(signature_obj.get_compact_signature()), signature_obj.get_compact_signature_size());
        
        std::string bad_account_addr = good_pub_key_obj.to_address(addr_type, ledger_id);
        bad_account_addr[bad_account_addr.size() - 3] = '0' + (const char) (base::xtime_utl::get_fast_randomu() % 32);
        
        xassert(utl::xcrypto_util::verify_sign(msg_digest,signature,bad_account_addr) == false);
    }
    return 0;
}

int test_ca_api(const char addr_type,const uint16_t ledger_id,const int test_round)
{
    for(int i = 0; i < test_round; ++i)
    {
        utl::xecprikey_t sec256k1_private_key;
        const std::string target_account = sec256k1_private_key.to_account_address(addr_type, ledger_id);
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
                const std::string node_account  = node_prv_key.to_account_address(addr_type, ledger_id);
                
                _consensus_nodes.push_back(new base::xvnode_t(node_account,node_addr,_node_pub_key_str,_node_prv_key_str));
            }
            base::xauto_ptr<base::xvnodegroup_t> _consensus_group(new base::xvnodegroup_t(_shard_xipaddr,0,_consensus_nodes));
            _nodesvr_ptr->add_group(_consensus_group.get());
            
            std::string empty_tx;
            xunitblock_t* target_block = xunitblock_t::create_unitblock(target_account,1,1,1,std::string("0"),std::string("0"),0,empty_tx,empty_tx);
            target_block->get_cert()->set_validator(_consensus_nodes[0]->get_xip2_addr());
            
            xobject_ptr_t<base::xvcertauth_t> auth_instance(auth::xauthcontext_t::create(*_nodesvr_ptr));
            //do_sign & do_verify
            {
                const std::string _signature = auth_instance->do_sign(_consensus_nodes[0]->get_xip2_addr(), target_block, 0);
                target_block->set_verify_signature(_signature);
                
                xassert(base::enum_vcert_auth_result::enum_successful == auth_instance->verify_sign(_consensus_nodes[0]->get_xip2_addr(), target_block->get_cert(),target_account));
            }
            
            //do muti-sign/verify
            {
                std::vector<xvip2_t>     _nodes_xvip2;
                std::vector<std::string> _nodes_signatures;
                
                // (2 * 4) / 3 + 1 = 3
                _nodes_xvip2.push_back(_consensus_nodes[0]->get_xip2_addr());
                _nodes_xvip2.push_back(_consensus_nodes[1]->get_xip2_addr());
                _nodes_xvip2.push_back(_consensus_nodes[2]->get_xip2_addr());
                
                _nodes_signatures.push_back(auth_instance->do_sign(_nodes_xvip2[0], target_block, 0));
                _nodes_signatures.push_back(auth_instance->do_sign(_nodes_xvip2[1], target_block, 0));
                _nodes_signatures.push_back(auth_instance->do_sign(_nodes_xvip2[2], target_block, 0));
                
                const std::string _muti_signature = auth_instance->merge_muti_sign(_nodes_xvip2, _nodes_signatures, target_block->get_cert());
                target_block->set_verify_signature(_muti_signature);
                
                xassert(base::enum_vcert_auth_result::enum_successful == auth_instance->verify_muti_sign(target_block->get_cert(),target_account));
                target_block->set_block_flag(base::enum_xvblock_flag_authenticated); //then add flag of auth
            }
            
            target_block->release_ref();
            _nodesvr_ptr->remove_group(_consensus_group->get_xip2_addr());
            _nodesvr_ptr->release_ref();
            for(auto it : _consensus_nodes)
                it->release_ref();
        }
    }
    return 0;
}

int test_key_accounts()
{
    std::set<std::string> all_pri_keys;
    //test openssl->secp256k1

    if(1) //test key-generate from openssl and do check by sec256k1
    {
        for(int i = 0; i < 100; ++i)
        {
            //pub/pri keys from openssl
            xmutisig::xprikey _openssl_private_key;
            xmutisig::xpubkey _openssl_public_key(_openssl_private_key);
            std::string openssl_pri_key_bin = _openssl_private_key.to_string();
            std::string openssl_pub_key_bin = _openssl_public_key.get_serialize_str();
            xassert(openssl_pri_key_bin.size() == 32);
            xassert(openssl_pub_key_bin.size() == 33);

            //pub/pri keys from sec256k1 libs
            utl::xecprikey_t sec256k1_private_key((uint8_t*)openssl_pri_key_bin.data());
            std::string sec256k1_pri_key_bin((const char*)sec256k1_private_key.data(),sec256k1_private_key.size());
            std::string sec256k1_pub_key_bin = sec256k1_private_key.get_compress_public_key();
            xassert(sec256k1_pri_key_bin.size() == 32);
            xassert(sec256k1_pub_key_bin.size() == 33);

            xassert(sec256k1_pri_key_bin == openssl_pri_key_bin);
            xassert(sec256k1_pub_key_bin == openssl_pub_key_bin);

            if(all_pri_keys.find(sec256k1_pri_key_bin) == all_pri_keys.end())
            {
                all_pri_keys.emplace(sec256k1_pri_key_bin);
            }
            else
            {
                xassert(0);
            }

            const std::string account_address = sec256k1_private_key.to_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, i);

            const uint256_t hash_to_sign = utl::xsha2_256_t::digest(account_address);
            utl::xecdsasig_t sign_res_obj = sec256k1_private_key.sign(hash_to_sign);
            const std::string sign_res_string( (const char *)sign_res_obj.get_compact_signature(),sign_res_obj.get_compact_signature_size());
            xassert(sign_res_string == utl::xcrypto_util::digest_sign(hash_to_sign, sec256k1_private_key.data()));

            utl::xkeyaddress_t key_addr(account_address);
            xassert(key_addr.is_valid());
            xassert(key_addr.verify_signature(sign_res_obj, hash_to_sign));
            xassert(utl::xcrypto_util::verify_sign(hash_to_sign,sign_res_string,account_address));

            xassert(base::xvaccount_t::get_addrtype_from_account(account_address) == base::enum_vaccount_addr_type_secp256k1_user_account);
            xassert(base::xvaccount_t::get_ledgerid_from_account(account_address) == (uint16_t)i);
        }
    }

    all_pri_keys.clear();
    if(1) //test key-generate from sec256k1 libs and do check by openssl
    {
        for(int i = 0; i < 100; ++i)
        {
            //pub/pri keys from sec256k1 libs
            utl::xecprikey_t sec256k1_private_key;
            std::string sec256k1_pri_key_bin((const char*)sec256k1_private_key.data(),sec256k1_private_key.size());
            std::string sec256k1_pub_key_bin = sec256k1_private_key.get_compress_public_key();
            xassert(sec256k1_pri_key_bin.size() == 32);
            xassert(sec256k1_pub_key_bin.size() == 33);

            //pub/pri keys from openssl
            xmutisig::xprikey _openssl_private_key(sec256k1_pri_key_bin);
            xmutisig::xpubkey _openssl_public_key(_openssl_private_key);
            std::string openssl_pri_key_bin = _openssl_private_key.to_string();
            std::string openssl_pub_key_bin = _openssl_public_key.get_serialize_str();
            xassert(openssl_pri_key_bin.size() == 32);
            xassert(openssl_pub_key_bin.size() == 33);

            xassert(sec256k1_pri_key_bin == openssl_pri_key_bin);
            xassert(sec256k1_pub_key_bin == openssl_pub_key_bin);

            if(all_pri_keys.find(sec256k1_pri_key_bin) == all_pri_keys.end())
            {
                all_pri_keys.emplace(sec256k1_pri_key_bin);
            }
            else
            {
                xassert(0);
            }
        }
    }
    return 0;
}
int main(int argc, const char * argv[]) {


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

    test_ckey(base::enum_vaccount_addr_type_secp256k1_eth_user_account,0,1000);
    test_ckey(base::enum_vaccount_addr_type_secp256k1_user_account,0,1000);
    
    test_ca_api(base::enum_vaccount_addr_type_secp256k1_eth_user_account,0,100);
    test_ca_api(base::enum_vaccount_addr_type_secp256k1_user_account,0,100);
    
    test_key_accounts();

    printf("test over, quit now! \n");
    return 0;
}
