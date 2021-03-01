// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtestca.hpp"
#include "xbase/xutl.h"

namespace top
{
    namespace test
    {
        xschnorrcert_t::xschnorrcert_t(const uint32_t total_nodes)
        {
            m_total_nodes = total_nodes;
        }
        
        xschnorrcert_t::~xschnorrcert_t()
        {
        }
        
        const std::string   xschnorrcert_t::get_signer(const xvip2_t & signer) //query account address of xvip2_t
        {
            return base::xstring_utl::tostring(signer.low_addr);
        }
        
        //all returned information build into a xvip_t structure
        xvip_t              xschnorrcert_t::get_validator_addr(const std::string & account_addr) //mapping account to target group
        {
            return 0;
        }
        bool                xschnorrcert_t::verify_validator_addr(const base::xvblock_t * test_for_block)//verify validator and account
        {
            return true;
        }
        bool                xschnorrcert_t::verify_validator_addr(const std::string & for_account,const base::xvqcert_t * for_cert)//verify validator and account
        {
            return true;
        }

        //random_seed allow pass a customzied random seed to provide unique signature,it ask xvcertauth_t generate one if it is 0
        //signature by owner ' private-key
        const std::string    xschnorrcert_t::do_sign(const xvip2_t & signer,const base::xvqcert_t * sign_for_cert,const uint64_t random_seed)
        {
            if(NULL == sign_for_cert)
                return std::string();
            
            const std::string ask_sign_hash = sign_for_cert->get_hash_to_sign();

            std::string random_seed_string = base::xstring_utl::tostring(base::xtime_utl::get_fast_random64());
            if(random_seed != 0)
                random_seed_string += base::xstring_utl::tostring(random_seed);
            
            return (base::xstring_utl::tostring(signer.low_addr) + ask_sign_hash + random_seed_string);
        }
        
        const std::string    xschnorrcert_t::do_sign(const xvip2_t & signer,const base::xvblock_t * sign_for_block,const uint64_t random_seed)
        {
            if(NULL == sign_for_block)
                return std::string();
            
            return do_sign(signer,sign_for_block->get_cert(),random_seed);
        }
        
        base::enum_vcert_auth_result                 xschnorrcert_t::verify_sign(const xvip2_t & signer,const base::xvqcert_t * test_for_cert,const std::string & block_account)
        {
            return base::enum_vcert_auth_result::enum_successful;
        }
        base::enum_vcert_auth_result                 xschnorrcert_t::verify_sign(const xvip2_t & signer,const base::xvblock_t * test_for_block)
        {
            return base::enum_vcert_auth_result::enum_successful;
        }
     
        const std::string   xschnorrcert_t::merge_muti_sign(const std::vector<xvip2_t> & muti_nodes,const std::vector<std::string> & muti_signatures,const base::xvqcert_t * for_cert)
        {
            if(NULL == for_cert)
                return std::string();
            
            if(muti_signatures.empty()) //must have this protection to genereate empty hash
                return std::string();
            
            std::string   merged_signature;
            for(auto it = muti_signatures.begin(); it != muti_signatures.end(); ++it)
            {
                merged_signature += (*it);
            }
            return merged_signature;
        }
        
        //merge multiple single-signature into threshold signature,and return a merged signature
        const std::string    xschnorrcert_t::merge_muti_sign(const std::map<xvip2_t,std::string,xvip2_compare> & muti_signatures,const base::xvqcert_t * for_cert)
        {
            if(NULL == for_cert)
                return std::string();
            
            if(muti_signatures.empty()) //must have this protection to genereate empty hash
                return std::string();
            
            std::string   merged_signature;
            for(auto it = muti_signatures.begin(); it != muti_signatures.end(); ++it)
            {
                merged_signature += it->second;
            }
            return merged_signature;
        }
        
        const std::string    xschnorrcert_t::merge_muti_sign(const std::map<xvip2_t,std::string,xvip2_compare> & muti_signatures,const base::xvblock_t * for_block)
        {
            if(NULL == for_block)
                return std::string();
            
            return merge_muti_sign(muti_signatures,for_block->get_cert());
        }
 
        base::enum_vcert_auth_result                 xschnorrcert_t::verify_muti_sign(const base::xvqcert_t * test_for_cert,const std::string & block_account)
        {
            if(NULL == test_for_cert)
                return base::enum_vcert_auth_result::enum_verify_fail;
            
            return base::enum_vcert_auth_result::enum_successful;
        }
 
        base::enum_vcert_auth_result                 xschnorrcert_t::verify_muti_sign(const base::xvblock_t * test_for_block)
        {
            if(NULL == test_for_block)
                return base::enum_vcert_auth_result::enum_verify_fail;
            
            return verify_muti_sign(test_for_block->get_cert(),test_for_block->get_account());
        }
    };
};
