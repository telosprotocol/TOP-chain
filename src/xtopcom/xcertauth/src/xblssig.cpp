// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xblssig.h"

namespace top
{
    namespace auth
    {
        xtbls_t::xtbls_t()
        {
        }
        
        xtbls_t::~xtbls_t()
        {
        }
        
        bool  xtbls_t::create_keypair(std::string & prikey,std::string & pubkey)
        {
            return false;
        }
        
        const std::string    xtbls_t::do_sign(const base::xvnode_t & signer,const std::string & sign_target_hash,const uint64_t random_seed,const uint64_t auth_mutisign_token)
        {
            std::string random_seed_string = base::xstring_utl::tostring(base::xtime_utl::get_fast_random64());
            if(random_seed != 0)
                random_seed_string += base::xstring_utl::tostring(random_seed);
            
            return (signer.get_account() + sign_target_hash + random_seed_string);
        }
        
        bool                 xtbls_t::verify_sign(const base::xvnode_t & signer,const std::string & target_hash,const std::string & signature,const uint64_t auth_mutisign_token)
        {
            return true;
        }
        
        //return a merged signature
        const std::string    xtbls_t::merge_muti_sign(const base::xvnodegroup_t & nodes_group,const std::vector<xvip2_t> & muti_nodes,const std::vector<std::string> & muti_signatures,const uint64_t auth_mutisign_token)
        {
            if(muti_signatures.empty()) //must have this protection to genereate empty hash
                return std::string();
            
            std::string   merged_string;
            for(auto it = muti_signatures.begin(); it != muti_signatures.end(); ++it)
            {
                merged_string += (*it);
            }
            return merged_string;
        }
        
        bool                 xtbls_t::verify_muti_sign(const base::xvnodegroup_t & group,const uint32_t sig_threshold,const std::string & target_hash,const std::string & multi_signatures,const uint64_t auth_mutisign_token,std::set<std::string> & exclude_accounts,std::set<std::string> & exclude_keys)
        {
            return true;
        }
    }; //end of namespace of auth
};//end of namesapce of top
