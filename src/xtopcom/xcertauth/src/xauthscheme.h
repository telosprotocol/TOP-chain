// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xvcnode.h"
#include "xvledger/xvblock.h"

namespace top
{
    namespace auth
    {
        class xauthscheme_t : public base::xrefcount_t
        {
        public:
            static xauthscheme_t*  create_auth_scheme(base::enum_xvchain_sign_scheme scheme);
        protected:
            xauthscheme_t();
            virtual ~xauthscheme_t();
        private:
            xauthscheme_t(const xauthscheme_t &);
            xauthscheme_t & operator = (const xauthscheme_t &);
        public:
            virtual const std::string    do_sign(const base::xvnode_t & signer,const std::string & sign_target_hash,const uint64_t random_seed,const uint64_t auth_mutisign_token) = 0;
            
            virtual bool                 verify_sign(const base::xvnode_t & signer,const std::string & target_hash,const std::string & signature,const uint64_t auth_mutisign_token) = 0;
            
            //return a merged signature
            virtual const std::string    merge_muti_sign(const base::xvnodegroup_t & nodes_group,const std::vector<xvip2_t> & muti_nodes,const std::vector<std::string> & muti_signatures,const uint64_t shared_mutisign_token) = 0;
            
            //std::set<std::string> & exclude_accounts filter any duplicated accounts
            virtual bool                 verify_muti_sign(const base::xvnodegroup_t & group,const uint32_t sig_threshold,const std::string & target_hash,const std::string & multi_signatures,const uint64_t auth_mutisign_token,std::set<std::string> & exclude_accounts,std::set<std::string> & exclude_keys) = 0;
        public:
            virtual bool                 create_keypair(std::string & prikey,std::string & pubkey) = 0;
        };
        
    }; //end of namespace of auth
};//end of namesapce of top
