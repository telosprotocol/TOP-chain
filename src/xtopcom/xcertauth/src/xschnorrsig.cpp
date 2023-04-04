// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "xbase/xutl.h"
#include "xschnorrsig.h"
#include "xsigndata.h"
#include "xmutisig/xschnorr.h"
#include "xmutisig/xmutisig.h"
#include "xsafebox/safebox_proxy.h"


namespace top
{
    namespace auth
    {
        xschnorrsig_t::xschnorrsig_t()
        {
        }
        
        xschnorrsig_t::~xschnorrsig_t()
        {
        }
        
        bool   xschnorrsig_t::create_keypair(std::string & prikey,std::string & pubkey)
        {
            xmutisig::xprikey _private_key;
            xmutisig::xpubkey _public_key(_private_key);
            
            prikey = _private_key.get_serialize_str();
            pubkey = _public_key.get_serialize_str();
            
            return true;
        }
        
        const std::string    xschnorrsig_t::do_sign(const base::xvnode_t & signer,const std::string & sign_target_hash,const uint64_t random_seed,const uint64_t auth_mutisign_token)
        {
            if((0 == auth_mutisign_token) || sign_target_hash.empty())
            {
                xerror("xmutisig::do_sign,fail-bad params for signer(%s)",signer.get_account().c_str());
                return std::string();
            }

            auto signature_result = safebox::xsafebox_proxy::get_instance().get_proxy_signature(signer.get_sign_pubkey(), sign_target_hash);
            
            if (signature_result.second.empty())
            {
                xerror("xmutisig::do_sign,fail-sign for for signer(%s)",signer.get_account().c_str());
                return std::string();
            }

            xmutisigdata_t schnorr_sig_data(signature_result.first, signature_result.second, get_group_nodes_count_from_xip2(signer.get_xip2_addr()), auth_mutisign_token);
            schnorr_sig_data.get_nodebitset().set(get_node_id_from_xip2(signer.get_xip2_addr()));//mark who sign it
            
            return schnorr_sig_data.serialize_to_string();//convert to string
        }
        
        bool                 xschnorrsig_t::verify_sign(const base::xvnode_t & signer,const std::string & target_hash,const std::string & signature,const uint64_t auth_mutisign_token)
        {
            if(signature.empty() || target_hash.empty() || (0 == auth_mutisign_token))
            {
                xerror("xschnorrsig_t::verify_sign,fail-bad parameters");
                return false;
            }
            
            xmutisigdata_t schnorr_sig_data;
            if(schnorr_sig_data.serialize_from_string(signature) > 0)
            {
                if(schnorr_sig_data.get_mutisig_token() != auth_mutisign_token)
                {
                    xerror("xschnorrsig_t::verify_sign,fail-unmatched tokens,token(%" PRIx64 ") != auth-token(%" PRIx64 ") ",schnorr_sig_data.get_mutisig_token(),auth_mutisign_token);
                    return false;
                }
                xmutisig::xpubkey _singer_public_key(signer.get_sign_pubkey());
                if(_singer_public_key.ec_point() == nullptr)
                {
                    xerror("xschnorrsig_t::verify_sign,fail-an invalid public key for signer(%s)",signer.get_account().c_str());
                    return false;
                }
                return xmutisig::xmutisig::verify_sign(target_hash,
                                                       _singer_public_key,
                                                       schnorr_sig_data.get_mutisig_seal(),
                                                       schnorr_sig_data.get_mutisig_point(),
                                                       xmutisig::xschnorr::instance());
            }
            xerror("xschnorrsig_t::verify_sign,fail-bad signature");
            return false;
        }
        
        //return a merged & aggregated signature,muti_nodes must be at nodes of group
        const std::string    xschnorrsig_t::merge_muti_sign(const base::xvnodegroup_t & nodes_group,const std::vector<xvip2_t> & muti_nodes,const std::vector<std::string> & muti_signatures,const uint64_t auth_mutisign_token)
        {
            //must have this protection to genereate empty hash
            if(muti_nodes.empty() ||  muti_signatures.empty() || (muti_nodes.size() != muti_signatures.size()) )
            {
                xerror("xschnorrsig_t::merge_muti_sign,fail-bad parameters");
                return std::string();
            }
       
            std::vector<std::string>  muti_sig_seal_datas;
            muti_sig_seal_datas.reserve(muti_signatures.size());
            std::vector<std::string>  muti_sig_point_datas;
            muti_sig_point_datas.reserve(muti_signatures.size());
            for(auto it = muti_signatures.begin(); it != muti_signatures.end(); ++it)
            {
                xmutisigdata_t _single_sig_data;
                if(_single_sig_data.serialize_from_string(*it) <= 0)
                {
                    xerror("xschnorrsig_t::merge_muti_sign,fail-bad xmutisigdata");
                    return std::string();
                }
                if(auth_mutisign_token != _single_sig_data.get_mutisig_token())
                {
                    xerror("xschnorrsig_t::merge_muti_sign,fail-bad xmutisigdata with token(%" PRIx64 ") != auth-token(%" PRIx64 ") ",_single_sig_data.get_mutisig_token(),auth_mutisign_token);
                    return std::string();
                }
                if(_single_sig_data.get_nodebitset().get_alloc_bits() != (int) nodes_group.get_size())
                {
                    xerror("xschnorrsig_t::merge_muti_sign,fail-bad xmutisigdata with bits size(%d) != group(%u)",_single_sig_data.get_nodebitset().get_alloc_bits(),nodes_group.get_size());
                    return std::string();
                }
                if( _single_sig_data.get_mutisig_seal().empty() || _single_sig_data.get_mutisig_point().empty() )
                {
                    xerror("xschnorrsig_t::merge_muti_sign,fail-bad xmutisigdata with empty seal or point");
                    return std::string();
                }
                muti_sig_seal_datas.push_back(_single_sig_data.get_mutisig_seal());
                muti_sig_point_datas.push_back(_single_sig_data.get_mutisig_point());
            }
            
            std::vector<xmutisig::xsignature>  muti_sig_seal_objects;
            std::vector<xmutisig::xrand_point> muti_sig_point_objects;
            muti_sig_seal_objects.reserve(muti_signatures.size());
            muti_sig_point_objects.reserve(muti_signatures.size());
            for (auto & it  : muti_sig_seal_datas)
            {
                muti_sig_seal_objects.emplace_back(xmutisig::xsignature(it));
                auto & item_obj = *muti_sig_seal_objects.rbegin();
                if(item_obj.bn_value() == nullptr)
                {
                    xerror("xschnorrsig_t::merge_muti_sign,fail-empty seal object");
                    return std::string();
                }
            }
            for (auto & it : muti_sig_point_datas)
            {
                muti_sig_point_objects.emplace_back(xmutisig::xrand_point(it));
                auto & item_obj = *muti_sig_point_objects.rbegin();
                if(item_obj.ec_point() == nullptr)
                {
                    xerror("xschnorrsig_t::merge_muti_sign,fail-empty point object");
                    return std::string();
                }
            }
            
            std::shared_ptr<xmutisig::xrand_point> aggregated_points{ nullptr };
            std::shared_ptr<xmutisig::xsignature>  aggregated_signs{ nullptr };
            xmutisig::xmutisig::aggregate_sign_points_2(muti_sig_point_objects, muti_sig_seal_objects, aggregated_points, aggregated_signs, xmutisig::xschnorr::instance());
            xassert(aggregated_points != nullptr);
            xassert(aggregated_signs != nullptr);
            
            xmutisigdata_t aggregated_sig_data(aggregated_points->get_serialize_str(),aggregated_signs->get_serialize_str(),nodes_group.get_size(),auth_mutisign_token);//each node must be at in same group
            for(auto it = muti_nodes.begin(); it != muti_nodes.end(); ++it)
            {
                xdbgassert(is_xip2_group_equal(nodes_group.get_xip2_addr(), (*it))); //xcetauth has verified actually
                aggregated_sig_data.get_nodebitset().set(get_node_id_from_xip2((*it)));
            }
            
            #ifdef DEBUG //check whether has any wrong address passed in
            int total_node_bits = 0;
            for(uint32_t i = 0; i < nodes_group.get_size(); ++i)
            {
                if(aggregated_sig_data.get_nodebitset().is_set(i))
                    ++total_node_bits;
            }
            xassert(total_node_bits == (int)muti_signatures.size());
            #endif
            return aggregated_sig_data.serialize_to_string();//convert to string
        }
        
        //std::set<std::string> & exclude_accounts filter any duplicated accounts, and std::set<std::string> & exclude_keys filter any duplicated keys
        bool                 xschnorrsig_t::verify_muti_sign(const base::xvnodegroup_t & group,const uint32_t sig_threshold,const std::string & target_hash,const std::string & aggregated_signatures_bin,const uint64_t auth_mutisign_token,std::set<std::string> & exclude_accounts,std::set<std::string> & exclude_keys)
        {
            if(aggregated_signatures_bin.empty() || target_hash.empty() || (0 == sig_threshold) || (0 == auth_mutisign_token))
            {
                xerror("xschnorrsig_t::verify_muti_sign,fail-bad parameters");
                return false;
            }
            xmutisigdata_t aggregated_sig_obj;
            if(aggregated_sig_obj.serialize_from_string(aggregated_signatures_bin) > 0)
            {
                xnodebitset  & nodebits = aggregated_sig_obj.get_nodebitset();
                if(aggregated_sig_obj.get_mutisig_token() != auth_mutisign_token)
                {
                    xwarn("xschnorrsig_t::verify_muti_sign,fail-token(%" PRIx64 ") != auth-token(%" PRIx64 ") ",aggregated_sig_obj.get_mutisig_token(),auth_mutisign_token);
                    return false;
                }
                if(nodebits.get_alloc_bits() != (int) group.get_size()) //must be same
                {
                    xerror("xschnorrsig_t::verify_muti_sign,fail-NOT match node_bits to node_group");
                    return false;
                }

                std::vector<xmutisig::xpubkey> muti_signers_pubkey;
                muti_signers_pubkey.reserve(nodebits.get_alloc_bits());
                for(int i = 0; i < nodebits.get_alloc_bits(); ++i)
                {
                    if(nodebits.is_set(i))
                    {
                        base::xvnode_t * _node_ptr = group.get_node(i);
                        if(NULL == _node_ptr)
                        {
                            xerror("xschnorrsig_t::verify_muti_sign,fail-found missed node at slot(%d) for group(%" PRIx64 " : %" PRIx64 ") ",i,group.get_xip2_addr().high_addr,group.get_xip2_addr().low_addr);
                            continue;
                        }
                        if(false == _node_ptr->get_sign_pubkey().empty())
                        {
                            auto account_insert_result = exclude_accounts.emplace(_node_ptr->get_account());
                            if(account_insert_result.second)//insert successful
                            {
                                auto key_insert_result = exclude_keys.emplace(_node_ptr->get_sign_pubkey());
                                if(key_insert_result.second)//insert successful
                                    muti_signers_pubkey.emplace_back(xmutisig::xpubkey(_node_ptr->get_sign_pubkey()));
                            }
                            else
                            {
                                xerror("xschnorrsig_t::verify_muti_sign,fail-found duplicated node with account(%s) for group(%" PRIx64 " : %" PRIx64 ") ",_node_ptr->get_account().c_str(),group.get_xip2_addr().high_addr,group.get_xip2_addr().low_addr);
                            }
                        }
                    }
                }
                if(muti_signers_pubkey.size() < sig_threshold)
                {
                    xerror("xschnorrsig_t::verify_muti_sign,fail-too little signers(%d) < sig_threshold(%u)",(int32_t)muti_signers_pubkey.size(),sig_threshold);
                    return false;
                }
                std::shared_ptr<xmutisig::xpubkey> vote_agg_pub = xmutisig::xmutisig::aggregate_pubkeys_2(muti_signers_pubkey, xmutisig::xschnorr::instance());
                if(vote_agg_pub == nullptr)
                {
                    xerror("xschnorrsig_t::verify_muti_sign,fail-aggregate pubkeys with size(%d)",(int32_t)muti_signers_pubkey.size());
                    return false;
                }
                return xmutisig::xmutisig::verify_sign(target_hash,
                                                       *vote_agg_pub.get(),
                                                       aggregated_sig_obj.get_mutisig_seal(),
                                                       aggregated_sig_obj.get_mutisig_point(),
                                                       xmutisig::xschnorr::instance());
            }
            xerror("xschnorrsig_t::verify_muti_sign,fail-bad aggregated_signatures_bin");
            return false;
        }
        
    }; //end of namespace of auth
};//end of namesapce of top
