// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xauthscheme.h"

namespace top
{
    namespace auth
    {
        //muti-sign schnorr
        class xschnorrsig_t : public xauthscheme_t
        {
        public:
            xschnorrsig_t();
        protected:
            virtual ~xschnorrsig_t();
        private:
            xschnorrsig_t(const xschnorrsig_t &);
            xschnorrsig_t & operator = (const xschnorrsig_t &);
        public:
            virtual const std::string    do_sign(const base::xvnode_t & signer,const std::string & sign_target_hash,const uint64_t random_seed,const uint64_t auth_mutisign_token) override;
            
            virtual bool                 verify_sign(const base::xvnode_t & signer,const std::string & target_hash,const std::string & signature,const uint64_t auth_mutisign_token) override;
            
            //return a merged & aggregated signature,muti_nodes must be at same group of network
            virtual const std::string    merge_muti_sign(const base::xvnodegroup_t & nodes_group,const std::vector<xvip2_t> & muti_nodes,const std::vector<std::string> & muti_signatures,const uint64_t auth_mutisign_token) override;
            
            virtual bool                 verify_muti_sign(const base::xvnodegroup_t & group,const uint32_t sig_threshold,const std::string & target_hash,const std::string & multi_signatures,const uint64_t auth_mutisign_token,std::set<std::string> & exclude_accounts,std::set<std::string> & exclude_keys) override;
            
        public:
            virtual bool                 create_keypair(std::string & prikey,std::string & pubkey) override;
        };
        
    }; //end of namespace of auth
};//end of namesapce of top
