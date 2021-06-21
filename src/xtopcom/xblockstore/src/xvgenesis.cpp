// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//#include "xdata/xrootblock.h"
#include "xvgenesis.h"
#include "xvledger/xvaccount.h"

#if !defined(__ENABLE_MOCK_XSTORE__)
    #include "xdata/xblocktool.h"
#endif

namespace top
{
    namespace store
    {
        xgenesis_header::xgenesis_header(const std::string & account)
        {
            const uint16_t ledger_id = base::xvaccount_t::get_ledgerid_from_account(account);
            set_chainid(base::xvaccount_t::get_chainid_from_ledgerid(ledger_id));
            //set_chainid(data::xrootblock_t::get_rootblock_chainid());
            set_account(account);
            set_height(0);
            
            set_block_type(base::enum_xvblock_type_genesis); //must be enum_xvblock_type_genesis
            set_block_class(base::enum_xvblock_class_nil);   //must be nil
            set_block_level(base::enum_xvblock_level_unit);  //must be nil
            
            set_last_block_hash(std::string());              //must be nil
            set_last_full_block(std::string(),0);            //must be nil and 0
        }
        
        xgenesis_header::~xgenesis_header()
        {
        }
        
        xgenesis_cert::xgenesis_cert(const std::string & account)
            :base::xvqcert_t(std::string())
        {
            set_consensus_type(base::enum_xconsensus_type_genesis);                 //must be enum_xconsensus_type_genesis
            set_consensus_threshold(base::enum_xconsensus_threshold_anyone);        //must be anyone
        
            set_crypto_sign_type(base::enum_xvchain_threshold_sign_scheme_none);    //genesis dose not need signature
            set_crypto_hash_type(enum_xhash_type_sha2_256);                         //default sha2_256
            
            set_clock(0);                                   //clock  must be 0
            set_viewid(0);                                  //viewid must be 0
            set_drand(0);                                   //drand height must be 0
            set_viewtoken(-1);                              //must be -1
            set_nonce(-1);                                  //must be -1
            xvip2_t _wildaddr{(xvip_t)(-1),(uint64_t)-1};
            set_validator(_wildaddr);                       //genesis block not need verify
            
            base::enum_vaccount_addr_type addr_type = base::xvaccount_t::get_addrtype_from_account(account);
            switch (addr_type)
            {
                case base::enum_vaccount_addr_type_secp256k1_user_account:
                case base::enum_vaccount_addr_type_secp256k1_eth_user_account:
                case base::enum_vaccount_addr_type_secp256k1_user_sub_account:
                case base::enum_vaccount_addr_type_native_contract:
                case base::enum_vaccount_addr_type_custom_contract:
                case base::enum_vaccount_addr_type_black_hole:
                case base::enum_vaccount_addr_type_block_contract:
                {
                    set_crypto_key_type(base::enum_xvchain_key_curve_secp256k1);  //secp256k1 key
                }
                break;
                    
                case base::enum_vaccount_addr_type_ed25519_user_account:
                case base::enum_vaccount_addr_type_ed25519_user_sub_account:
                {
                    set_crypto_key_type(base::enum_xvchain_key_curve_ed25519);    //ed25519 key
                }
                    break;
                    
                default:
                {
                    if( ((int)addr_type >= base::enum_vaccount_addr_type_ed25519_reserved_start) && ((int)addr_type <= base::enum_vaccount_addr_type_ed25519_reserved_end)
                       )
                    {
                        set_crypto_key_type(base::enum_xvchain_key_curve_ed25519);    //ed25519 key
                    }
                    else
                    {
                        set_crypto_key_type(base::enum_xvchain_key_curve_secp256k1);  //secp256k1 key
                    }
                }
                break;
            }
        }        
        xgenesis_cert::~xgenesis_cert()
        {
        }
        
        xgenesis_block::xgenesis_block(xgenesis_header & header,xgenesis_cert & cert)
           : base::xvblock_t(header,cert,nullptr,nullptr)
        {
            set_block_flag(base::enum_xvblock_flag_authenticated);
            set_block_flag(base::enum_xvblock_flag_locked);
            set_block_flag(base::enum_xvblock_flag_committed);
            set_block_flag(base::enum_xvblock_flag_confirmed);
            set_block_flag(base::enum_xvblock_flag_connected);
        }
        xgenesis_block::~xgenesis_block()
        {
            xinfo("xgenesis_block::~xgenesis_block");
        }
        
        base::xvblock_t*  xgenesis_block::create_genesis_block(const std::string & account)
        {
#if defined(__MAC_PLATFORM__) && defined(__ENABLE_MOCK_XSTORE__)
            xgenesis_header * _header_obj = new xgenesis_header(account);
            xgenesis_cert   * _cert_obj   = new xgenesis_cert(account);
            base::xvblock_t * _block_ptr  = new xgenesis_block(*_header_obj,*_cert_obj);

            //then clean
            _header_obj->release_ref();//safe to release now
            _cert_obj->release_ref();  //safe to release now
#else
            base::xvblock_t * _block_ptr = data::xblocktool_t::create_genesis_empty_table(account);
#endif
            _block_ptr->reset_modified_count(); //clean the flag of modification
            return _block_ptr;
        }

    };//end of namespace of vstore
};//end of namespace of top
