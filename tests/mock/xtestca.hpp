// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xvblock.h"

namespace top
{
    namespace test
    {
        class xschnorrcert_t : public base::xvcertauth_t //CA system
        {
        public:
            xschnorrcert_t(const uint32_t total_nodes);
        protected:
            virtual ~xschnorrcert_t();
        private:
            xschnorrcert_t();
            xschnorrcert_t(const xschnorrcert_t &);
            xschnorrcert_t & operator = (const xschnorrcert_t &);
        public:
            const uint32_t get_total_nodes() const {return m_total_nodes;} //just for test purpose ,we put total nodes here

            virtual const std::string   get_signer(const xvip2_t & signer) override; //query account address of xvip2_t

            //all returned information build into a xvip_t structure
            virtual xvip_t              get_validator_addr(const std::string & account_addr) override; //mapping account to target group
            virtual bool                verify_validator_addr(const base::xvblock_t * test_for_block) override;//verify validator and account
            virtual bool                verify_validator_addr(const std::string & for_account,const base::xvqcert_t * for_cert) override;//verify validator and account
        public:

            //random_seed allow pass a customzied random seed to provide unique signature,it ask xvcertauth_t generate one if it is 0
            //signature by owner ' private-key
            virtual const std::string    do_sign(const xvip2_t & signer,const base::xvqcert_t * sign_for_cert,const uint64_t random_seed)  override;
            virtual const std::string    do_sign(const xvip2_t & signer,const base::xvblock_t * sign_for_block,const uint64_t random_seed) override;

            virtual base::enum_vcert_auth_result                 verify_sign(const xvip2_t & signer,const base::xvqcert_t * test_for_cert,const std::string & block_account)  override;
            virtual base::enum_vcert_auth_result                 verify_sign(const xvip2_t & signer,const base::xvblock_t * test_for_block) override;

        public:
            //merge multiple single-signature into threshold signature,and return a merged signature
            virtual const std::string   merge_muti_sign(const std::vector<xvip2_t> & muti_nodes,const std::vector<std::string> & muti_signatures,const base::xvqcert_t * for_cert) override;

            virtual const std::string    merge_muti_sign(const std::map<xvip2_t,std::string,xvip2_compare> & muti_signatures,const base::xvqcert_t * for_cert) override;
            virtual const std::string    merge_muti_sign(const std::map<xvip2_t,std::string,xvip2_compare> & muti_signatures,const base::xvblock_t * for_block) override;

        public:
            //note:just verify multi-sign of group is ok for 'sign_hash', but not check whether the sign_hash is good or not
            virtual base::enum_vcert_auth_result                 verify_muti_sign(const base::xvqcert_t * test_for_cert,const std::string & block_account) override;

            //note:check from ground: generate/check vbody'hash->  generate/check vheader'hash -> generate/check vqcert'sign-hash-> finally verify multi-signature of group. for safety please check threshold first to see it was ready
            virtual base::enum_vcert_auth_result                 verify_muti_sign(const base::xvblock_t * test_for_block) override;

        private:
            uint32_t        m_total_nodes;
        };
    };
};
