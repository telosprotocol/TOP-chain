// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xunitblock.hpp"
#include "xbase/xcontext.h"
#include "xbase/xmem.h"
#include "xbase/xhash.h"
#include "xbase/xobject.h"
#include "xbase/xutl.h"
#include "xvledger/xvcontractstore.h"
#include "xvledger/xvledger.h"
#include "xvledger/xvaction.h"
#include "xvledger/xvcontract.h"

#ifdef DEBUG
    #define __test_cache_for_block_store__
#endif

namespace top
{
    namespace test
    {
        //xvaction_t::xvaction_t(const std::string & tx_hash,const std::string & caller_addr,const std::string & target_uri,const std::string & method_name)
    
        tep0_tx::tep0_tx(const std::string & target_account,const int change_balance) //>0 for deposit, < 0 for withdraw
        {
            m_raw_action = NULL;
            const uint64_t random = base::xtime_utl::get_fast_random64();
            m_raw_tx_hash = target_account + base::xstring_utl::tostring(random);
            
            base::xvalue_t target_account_val(target_account);
            const std::string contract_uri = base::xvcontractstore_t::get_sys_tep0_contract_uri(target_account,0);
            if(change_balance > 0) //deposit
            {
                m_raw_action = new base::xvaction_t( base::xvcontract_TEP0::new_deposit(m_raw_tx_hash,target_account,(base::vtoken_t)change_balance,0,target_account));
            }
            else
            {
                m_raw_action = new base::xvaction_t( base::xvcontract_TEP0::new_withdraw(m_raw_tx_hash,target_account,(base::vtoken_t)(-change_balance),0,target_account));
            }
            m_raw_action->set_max_tgas(1000);
        }
    
        tep0_tx::~tep0_tx()
        {
            delete m_raw_action;
        }

        bool                     tep0_tx::is_valid()   const //verify signature,content,format etc
        {
            return true;
        }
    
        const std::string        tep0_tx::get_hash()   const
        {
            return m_raw_tx_hash;
        }
    
        const base::xvaction_t&  tep0_tx::get_action() const
        {
            return  *m_raw_action;
        }

        //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
        int32_t     tep0_tx::do_write(base::xstream_t & stream)     //write whole object to binary
        {
            const int32_t begin_size = stream.size();
            stream << m_raw_tx_hash;
            return (stream.size() - begin_size);
        }
    
        int32_t     tep0_tx::do_read(base::xstream_t & stream)      //read from binary and regeneate content of xdataobj_t
        {
            const int32_t begin_size = stream.size();
            stream >> m_raw_tx_hash;
            return (begin_size - stream.size());
        }
    
        xunitheader_t::xunitheader_t(const std::string & account,uint64_t height,const std::string & last_block_hash,const std::string & last_full_block_hash,const uint64_t last_full_block_height,base::enum_xvblock_class block_class)
        {
            const uint16_t ledger_id = base::xvaccount_t::get_ledgerid_from_account(account);
            set_chainid(base::xvaccount_t::get_chainid_from_ledgerid(ledger_id));
            set_account(account);
            set_height(height);
            if(0 == height) //init for genesis block
            {
                set_block_type(base::enum_xvblock_type_genesis); //must be enum_xvblock_type_genesis
                set_block_class(base::enum_xvblock_class_nil);   //must be nil
                set_block_level(base::enum_xvblock_level_unit);  //must be nil
                
                set_last_block_hash(std::string());              //must be nil
                set_last_full_block(std::string(),0);            //must be nil and 0
            }
            else
            {
                set_block_type(base::enum_xvblock_type_txs); //just test txs
                set_last_block_hash(last_block_hash);
                set_last_full_block(last_full_block_hash,last_full_block_height);
                set_block_level(base::enum_xvblock_level_unit);     //just test unit block
                
                set_block_class(block_class);
            }
        }
        
        xunitheader_t::~xunitheader_t()
        {
        }
        
        xunitcert_t::xunitcert_t()
            :base::xvqcert_t(std::string()) //init as empty
        {
            set_consensus_type(base::enum_xconsensus_type_xhbft);             //test pBFT
            set_consensus_threshold(base::enum_xconsensus_threshold_2_of_3);  // >= 2/3 vote
            
            set_crypto_key_type(base::enum_xvchain_key_curve_secp256k1); //default one
            set_crypto_sign_type(base::enum_xvchain_threshold_sign_scheme_schnorr); //test schnorr scheme
            set_crypto_hash_type(enum_xhash_type_sha2_256);  //basic sha2_256
        }
        
        xunitcert_t::~xunitcert_t()
        {
            
        }
    
        xunitblock_t*  xunitblock_t::create_unitblock(const std::string & account,uint64_t height,uint64_t clock,uint64_t viewid,const std::string & last_block_hash,const std::string & last_full_block_hash,const uint64_t last_full_block_height) //link xunitheader_t and xunitcert_t into xunitblock_t
        {
            //setp#1 create header object first,must initialize everyting here
            xunitheader_t* _header_obj = new xunitheader_t(account,height,last_block_hash,last_full_block_hash,last_full_block_height,base::enum_xvblock_class_light);

            //step#2: initialize certifaction object
            xunitcert_t  * _cert_obj   = new xunitcert_t();
            _cert_obj->set_viewid(viewid);
            _cert_obj->set_clock(clock);
 
            //step#3: link header and certification into block
            
            base::xvinput_t* input_ptr = NULL;
            base::xvoutput_t* output_ptr = NULL;
 
            //*****************NOT allow change _header_obj anymore after this line
            xunitblock_t * _block_obj  = new xunitblock_t(*_header_obj,*_cert_obj,input_ptr,output_ptr);
            
            //*****************Allow contiusely modify xunitcert_t object
            
            //then clean
            _header_obj->release_ref();//safe to release now
            _cert_obj->release_ref();  //safe to release now
            if(input_ptr != NULL)
                input_ptr->release_ref();
            
            if(output_ptr != NULL)
                output_ptr->release_ref();
            
            return _block_obj;
        }
        
        xunitblock_t*   xunitblock_t::create_genesis_block(const std::string & account)
        {
            std::string last_block_hash;
            std::string last_full_block_hash;
            std::string body_input;
            std::string body_output;
            xunitblock_t * genesis_block_ptr =  xunitblock_t::create_unitblock(account,0,0,0,last_block_hash,last_full_block_hash,0);
            
            genesis_block_ptr->get_cert()->set_viewtoken(-1); //for test only
            xvip2_t _wildaddr{(xvip_t)(-1),(uint64_t)-1};
            genesis_block_ptr->get_cert()->set_validator(_wildaddr);//genesis block not
            genesis_block_ptr->set_block_flag(base::enum_xvblock_flag_authenticated);
            genesis_block_ptr->set_block_flag(base::enum_xvblock_flag_locked);
            genesis_block_ptr->set_block_flag(base::enum_xvblock_flag_committed);
            genesis_block_ptr->set_block_flag(base::enum_xvblock_flag_executed);
            genesis_block_ptr->set_block_flag(base::enum_xvblock_flag_connected);
            genesis_block_ptr->reset_modified_count();
            
            return genesis_block_ptr;
        }
        
        xunitblock_t::xunitblock_t(xunitheader_t & header,xunitcert_t & cert,base::xvinput_t* input,base::xvoutput_t* output)
          : base::xvblock_t(header,cert,input,output)
        {
        }
 
        xunitblock_t:: ~xunitblock_t()
        {
            xdbg("xunitblock_t::destroy,dump=%s",dump().c_str());
        }
        
#ifdef DEBUG //tracking memory of proposal block
        int32_t   xunitblock_t::add_ref()
        {
            //xdump_stack2(enum_xlog_level_debug,3,"%d reference_to_add,objectid=%lld,height=%llu,view=%llu,this=%llx",get_refcount(),get_obj_id(),get_height(),get_viewid(),(int64_t)this);
            return base::xvblock_t::add_ref();
        }
        int32_t   xunitblock_t::release_ref()
        {
            //xdump_stack2(enum_xlog_level_debug,3,"%d reference_to_release,objectid=%lld,height=%llu,view=%llu,this=%llx",get_refcount(),get_obj_id(),get_height(),get_viewid(),(int64_t)this);
            xdbgassert(get_refcount() > 0);
            return base::xvblock_t::release_ref();
        }
#endif
        
     
    }
}
