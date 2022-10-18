// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject.h"
#include "xvblock.h"

namespace top
{
    namespace base
    {
        enum class enum_vcert_auth_result
        {
            enum_successful     =  0, //successful
            enum_verify_fail    = -1, //verification & signatures are not pass
            
            enum_bad_cert       = -2,
            enum_bad_block      = -3,
            enum_bad_address    = -4,
            enum_bad_signature  = -5, //invlaid or currupt data of sigature
            enum_bad_scheme     = -6, //invalid scheme of singature
            enum_bad_consensus  = -7, //invalid consensus type
            
            enum_nodes_notfound = -10,//node service dont have data of nodes,so fail to verify or sign
            enum_nodes_toofew   = -11,//few nodes to sign & verify
            enum_nodes_unactived= -12,//nodes/shard not effective yet according clock
        };
        
        //Certificate-Authority
        class xvcertauth_t : public xobject_t //CA system
        {
            friend class xvheader_t;
        public:
            static  const std::string   name(){ return std::string("xvcertauth");}
            virtual std::string         get_obj_name() const override {return name();}
        protected:
            xvcertauth_t();
            virtual ~xvcertauth_t();
        private:
            xvcertauth_t(const xvcertauth_t &);
            xvcertauth_t & operator = (const xvcertauth_t &);
            
        public:
            virtual const std::string   get_signer(const xvip2_t & signer) = 0; //query account address of xvip2_t
            //all returned information build into a xvip_t structure
            virtual xvip_t              get_validator_addr(const std::string & account_addr) = 0; //mapping account to target group
            virtual bool                verify_validator_addr(const base::xvblock_t * test_for_block) = 0;//verify validator and account
            virtual bool                verify_validator_addr(const std::string & for_account,const base::xvqcert_t * for_cert) = 0;//verify validator and account

        public: //returned_errcode parameter carry detail error if verify_muti_sign fail(return false)
            //random_seed allow pass a customzied random seed to provide unique signature,it ask xvcertauth_t generate one if it is 0
            //signature by owner ' private-key
            virtual const std::string   do_sign(const xvip2_t & signer,const base::xvqcert_t * sign_for_cert,const uint64_t random_seed)   = 0;
            virtual const std::string   do_sign(const xvip2_t & signer,const base::xvqcert_t * sign_for_block,const uint64_t random_seed, 
                                                const std::string sign_hash)= 0;
            virtual const std::string   do_sign(const xvip2_t & signer,const base::xvblock_t * sign_for_block,const uint64_t random_seed)= 0;
            
            virtual enum_vcert_auth_result   verify_sign(const xvip2_t & signer,const xvqcert_t * test_for_cert,const std::string & block_account)  = 0;
            virtual enum_vcert_auth_result   verify_sign(const xvip2_t & signer,const xvqcert_t * test_for_cert,
                                                         const std::string & block_account, const std::string sign_hash)  = 0;   
            virtual enum_vcert_auth_result   verify_sign(const xvip2_t & signer,const xvblock_t * test_for_block) = 0;
            
        public:
            //merge multiple single-signature into threshold signature,and return a merged signature
            virtual const std::string   merge_muti_sign(const std::vector<xvip2_t> & muti_nodes,const std::vector<std::string> & muti_signatures,const xvqcert_t * for_cert) = 0;
            virtual const std::string   merge_muti_sign(const std::map<xvip2_t,std::string,xvip2_compare> & muti_nodes_signatures,const xvqcert_t * for_cert) = 0;
            virtual const std::string   merge_muti_sign(const std::map<xvip2_t,std::string,xvip2_compare> & muti_nodes_signatures,const xvblock_t * for_block) = 0;
            
        public://returned_errcode parameter carry detail error if verify_muti_sign fail(return false)
            //note:just verify multi-sign of group is ok for 'sign_hash', but not check whether the sign_hash is good or not
            virtual enum_vcert_auth_result   verify_muti_sign(const xvqcert_t * test_for_cert,const std::string & block_account) = 0;
            
            //note:check from ground: generate/check vbody'hash->  generate/check vheader'hash -> generate/check vqcert'sign-hash-> finally verify multi-signature of group. for safety please check threshold first to see it was ready
            virtual enum_vcert_auth_result   verify_muti_sign(const xvblock_t * test_for_block) = 0;

            virtual const std::string get_pubkey(const xvip2_t & signer) = 0;
        };

    }//end of namespace of base
    
}//end of namespace top
