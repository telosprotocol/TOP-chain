// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "xcertauth_face.h"
#include "xauthscheme.h"
#include "xmutisig/xmutisig.h"
#include "xsigndata.h"
#include "xbasic/xlru_cache_specialize.h"

namespace top
{
    namespace auth
    {
        class xauthcontext_t_impl : public xauthcontext_t
        {
            friend class xauthcontext_t;
        protected:
            xauthcontext_t_impl(base::xvnodesrv_t & node_service);
            virtual ~xauthcontext_t_impl();
        private:
            xauthcontext_t_impl();
            xauthcontext_t_impl(const xauthcontext_t_impl &);
            xauthcontext_t_impl & operator = (const xauthcontext_t_impl &);

        public:
            virtual const std::string   get_signer(const xvip2_t & signer) override; //query account address of xvip2_t

            //all returned information build into a xvip_t structure
            virtual xvip_t              get_validator_addr(const std::string & account_addr) override; //mapping account to target group
            virtual bool                verify_validator_addr(const base::xvblock_t * test_for_block) override;//verify validator and account
            virtual bool                verify_validator_addr(const std::string & block_account,const base::xvqcert_t * for_cert) override;//verify validator and account
        public:
            //random_seed allow pass a customzied random seed to provide unique signature,it ask xvcertauth_t generate one if it is 0
            //signature by owner ' private-key
            virtual const std::string    do_sign(const xvip2_t & signer,const base::xvqcert_t * sign_for_cert,const uint64_t random_seed)  override;
            virtual const std::string    do_sign(const xvip2_t & signer,const base::xvqcert_t * sign_for_cert, 
                                                 const uint64_t random_seed, const std::string sign_hash) override;
            virtual const std::string    do_sign(const xvip2_t & signer,const base::xvblock_t * sign_for_block,const uint64_t random_seed) override;

            virtual base::enum_vcert_auth_result     verify_sign(const xvip2_t & signer,const base::xvqcert_t * test_for_cert,const std::string & block_account)  override;
            virtual base::enum_vcert_auth_result     verify_sign(const xvip2_t & signer,const base::xvqcert_t * test_for_cert,
                                                                 const std::string & block_account, const std::string sign_hash) override;
            virtual base::enum_vcert_auth_result     verify_sign(const xvip2_t & signer,const base::xvblock_t * test_for_block) override;

        public:
            //merge multiple single-signature into threshold signature,and return a merged signature
            virtual const std::string   merge_muti_sign(const std::vector<xvip2_t> & muti_nodes,const std::vector<std::string> & muti_signatures,const base::xvqcert_t * for_cert) override;

            virtual const std::string    merge_muti_sign(const std::map<xvip2_t,std::string,xvip2_compare> & muti_signatures,const base::xvqcert_t * for_cert) override;
            virtual const std::string    merge_muti_sign(const std::map<xvip2_t,std::string,xvip2_compare> & muti_signatures,const base::xvblock_t * for_block) override;

        public:
            //note:just verify multi-sign of group is ok for 'sign_hash', but not check whether the sign_hash is good or not
            virtual base::enum_vcert_auth_result    verify_muti_sign(const base::xvqcert_t * test_for_cert,const std::string & block_account) override;

            //note:check from ground: generate/check vbody'hash->  generate/check vheader'hash -> generate/check vqcert'sign-hash-> finally verify multi-signature of group. for safety please check threshold first to see it was ready
            virtual base::enum_vcert_auth_result    verify_muti_sign(const base::xvblock_t * test_for_block) override;

            virtual const std::string get_pubkey(const xvip2_t & signer) override;

        protected:
            base::enum_vcert_auth_result            verify_validator_mutisig(xauthscheme_t & auth_scheme_obj,const std::string & ask_verify_hash,const base::xvqcert_t * test_for_cert,std::set<std::string> & exclude_accounts,std::set<std::string> & exclude_keys);
            base::enum_vcert_auth_result            verify_auditor_mutisig(xauthscheme_t & auth_scheme_obj,const std::string & ask_verify_hash,const base::xvqcert_t * test_for_cert,std::set<std::string> & exclude_accounts,std::set<std::string> & exclude_keys);

        private:
            const std::string   do_sign_impl(const xvip2_t & signer,const base::xvqcert_t * sign_for_cert,const uint64_t random_seed);
            const std::string   do_sign_impl(const xvip2_t & signer,const base::xvqcert_t * sign_for_cert,const uint64_t random_seed, const std::string ask_sign_hash);
            base::enum_vcert_auth_result            verify_sign_impl(const xvip2_t & signer,const base::xvqcert_t * test_for_cert, const std::string ask_verify_hash);
            base::enum_vcert_auth_result            verify_muti_sign_impl(const base::xvqcert_t * test_for_cert);

            xauthscheme_t*       get_auth_scheme(const base::xvqcert_t * test_for_cert);
        protected:
            xauthscheme_t*       m_auth_schemes[base::enum_xvchain_sign_scheme_max + 1];  //total not over 8 as refer enum_xvchain_sign_scheme
            base::xvnodesrv_t&   m_node_service;
        private:
            basic::xlru_cache_specialize<std::string, bool> m_verified_hash{2000};
        };

        base::xvcertauth_t &  xauthcontext_t::instance(base::xvnodesrv_t & node_service)
        {
            static xauthcontext_t_impl _static_authcontext(node_service);
            return _static_authcontext;
        }

        xobject_ptr_t<base::xvcertauth_t>  xauthcontext_t::create(base::xvnodesrv_t & node_service)
        {
            base::xvcertauth_t* certauth_raw = new xauthcontext_t_impl(node_service);
            xobject_ptr_t<base::xvcertauth_t> certauth = nullptr;
            certauth.attach(certauth_raw);
            return certauth;
        }

        xauthcontext_t::xauthcontext_t()
        {
        }
        xauthcontext_t::~xauthcontext_t()
        {
        }

        xauthcontext_t_impl::xauthcontext_t_impl(base::xvnodesrv_t & node_service)
            :m_node_service(node_service)
        {
            m_node_service.add_ref();
            memset(m_auth_schemes,0,sizeof(m_auth_schemes));

            //init all avaible scheme of auth
            m_auth_schemes[base::enum_xvchain_threshold_sign_scheme_schnorr] = xauthscheme_t::create_auth_scheme(base::enum_xvchain_threshold_sign_scheme_schnorr);
        }
        xauthcontext_t_impl::~xauthcontext_t_impl()
        {
            m_node_service.release_ref();
            for(int i = 0; i <= base::enum_xvchain_sign_scheme_max; ++i)
            {
                if(m_auth_schemes[i] != NULL)
                    m_auth_schemes[i]->release_ref();
            }
        }

        xauthscheme_t*  xauthcontext_t_impl::get_auth_scheme(const base::xvqcert_t * test_for_cert)
        {
            return m_auth_schemes[test_for_cert->get_crypto_sign_type()];
        }

        const std::string   xauthcontext_t_impl::get_signer(const xvip2_t & signer)  //query account address of xvip2_t
        {
            base::xauto_ptr<base::xvnode_t> signer_node = m_node_service.get_node(signer);
            if(signer_node == nullptr)
            {
                xerror("xauthcontext_t_impl::get_signer,fail-found target nodes for signer(%" PRIx64 " : %" PRIx64 ")",signer.high_addr,signer.low_addr);
                return std::string();
            }
            return signer_node->get_address();
        }

        //all returned information build into a xvip_t structure
        xvip_t  xauthcontext_t_impl::get_validator_addr(const std::string & test_for_account)
        {
            assert(false);
            return xvip_t{};
            // deprecated api
#if 0
            
            xvip2_t  validator_addr;
            validator_addr.low_addr  = 0;
            validator_addr.high_addr = 0;

            const uint16_t  ledger_id       = base::xvaccount_t::get_ledgerid_from_account(test_for_account);
            const uint16_t  chain_id        = base::xvaccount_t::get_chainid_from_ledgerid(ledger_id);
            const uint16_t  zone_index      = base::xvaccount_t::get_zoneindex_from_ledgerid(ledger_id);

            const uint16_t  ledger_subaddr  = base::xvaccount_t::get_ledgersubaddr_from_account(test_for_account);
            const uint16_t  book_index      = base::xvaccount_t::get_book_index_from_subaddr(ledger_subaddr);
            //const uint16_t  table_index     = base::xvaccount_t::get_table_index_from_subaddr(ledger_subaddr);

            set_address_domain_to_xip2(validator_addr, enum_xaddress_domain_xip2);
            set_xip_type_to_xip2(validator_addr, enum_xip_type_static);
            set_network_type_to_xip2(validator_addr, enum_xnetwork_type_xchain);

            set_network_id_to_xip2(validator_addr,chain_id);
            set_zone_id_to_xip2(validator_addr,zone_index);
            set_cluster_id_to_xip2(validator_addr,book_index);

            //const int target_table_id = accountobj.get_table_index();
            //TODO mapping target_table_id to group address,and check again
            return validator_addr.low_addr;
#endif
        }

        bool   xauthcontext_t_impl::verify_validator_addr(const std::string & for_account,const base::xvqcert_t * test_for_cert)//verify validator and account
        {
            if( (NULL == test_for_cert) || (for_account.empty()) )
                return false;


            #ifdef __enable_account_address_check__
            assert(false);
            // chain_id might be wrong.

            const uint16_t  ledger_id       = base::xvaccount_t::get_ledgerid_from_account(for_account);
            const uint16_t  chain_id        = base::xvaccount_t::get_chainid_from_ledgerid(ledger_id);
            const uint16_t  zone_index      = base::xvaccount_t::get_zoneindex_from_ledgerid(ledger_id);

            const uint16_t  ledger_subaddr  = base::xvaccount_t::get_ledgersubaddr_from_account(for_account);
            const uint16_t  book_index      = base::xvaccount_t::get_book_index_from_subaddr(ledger_subaddr);
            //const uint16_t  table_index     = base::xvaccount_t::get_table_index_from_subaddr(ledger_subaddr);


            if(   (get_address_domain_from_xip2(test_for_cert->get_validator()) != enum_xaddress_domain_xip2)
               || (get_xip_type_from_xip2(test_for_cert->get_validator())       != enum_xip_type_static)
               || (get_network_type_from_xip2(test_for_cert->get_validator())   != enum_xnetwork_type_xchain)
               || (get_network_id_from_xip2(test_for_cert->get_validator())     != chain_id)
               || (get_zone_id_from_xip2(test_for_cert->get_validator())        != zone_index)
               || (get_cluster_id_from_xip2(test_for_cert->get_validator())     != book_index) )
            {
                xerror("xauthcontext_t_impl::verify_validator_addr,bad validator address=0x[%" PRIx64 " : %" PRIx64 "] vs account(%s)",test_for_cert->get_validator().high_addr,test_for_cert->get_validator().low_addr,for_account.c_str());
                return false;
            }
            #endif

            //const int target_table_id = accountobj.get_table_index();
            //TODO mapping target_table_id to group address,and check again
            return true;
        }

        bool   xauthcontext_t_impl::verify_validator_addr(const base::xvblock_t * test_for_block)
        {
            if(NULL == test_for_block)
                return false;

            return verify_validator_addr(test_for_block->get_account(),test_for_block->get_cert());
        }

        ///////////////////////////////////////////do_sign/////////////////////////////////////////////////////////
        const std::string    xauthcontext_t_impl::do_sign_impl(const xvip2_t & signer,const base::xvqcert_t * sign_for_cert,const uint64_t random_seed)
        {
            if (NULL == sign_for_cert) {
                xerror("xauthcontext_t_impl::do_sign_impl, sign_for_cert is null ");
                return std::string();
            }
            const std::string ask_sign_hash = sign_for_cert->get_hash_to_sign();
            return do_sign_impl(signer, sign_for_cert, random_seed, ask_sign_hash);
        }
        const std::string   xauthcontext_t_impl::do_sign_impl(const xvip2_t & signer,const base::xvqcert_t * sign_for_cert,
                                                              const uint64_t random_seed, const std::string ask_sign_hash)
        {
            if(sign_for_cert->get_consensus_type() != base::enum_xconsensus_type_xhbft)
            {
                xerror("xauthcontext_t_impl::do_sign,fail-cert_auth requrest enum_xconsensus_type_xhbft for cert:%s",sign_for_cert->dump().c_str());
                return std::string();
            }

            xauthscheme_t * auth_scheme_obj = get_auth_scheme(sign_for_cert);
            if(NULL == auth_scheme_obj)
            {
                xerror("xauthcontext_t_impl::do_sign,fail-found related auth scheme for cert:%s",sign_for_cert->dump().c_str());
                return std::string();
            }
            base::xauto_ptr<base::xvnode_t> signer_node = m_node_service.get_node(signer);
            if(signer_node == nullptr)
            {
                xerror("xauthcontext_t_impl::do_sign,fail-found target nodes for signer(%" PRIx64 " : %" PRIx64 ")",signer.high_addr,signer.low_addr);
                return std::string();
            }
            const uint64_t auth_mutisign_token = sign_for_cert->get_viewid() + sign_for_cert->get_viewtoken();
            return auth_scheme_obj->do_sign(*signer_node, ask_sign_hash, random_seed,auth_mutisign_token);
        }
        const std::string    xauthcontext_t_impl::do_sign(const xvip2_t & signer,const base::xvqcert_t * sign_for_cert,const uint64_t random_seed)
        {
            if(NULL == sign_for_cert)
                return std::string();

            const std::string ask_sign_hash = sign_for_cert->get_hash_to_sign();
            return do_sign(signer, sign_for_cert, random_seed, ask_sign_hash);
        }
        const std::string  xauthcontext_t_impl::do_sign(const xvip2_t & signer, const base::xvqcert_t * sign_for_cert, 
                                                        const uint64_t random_seed, const std::string sign_hash)
        {
             if(NULL == sign_for_cert)
                return std::string();

            if(false == sign_for_cert->is_valid()) //do valid test before do sign cert
            {
                xerror("xauthcontext_t_impl::do_sign,fail-pass the valid test for cert:%s",sign_for_cert->dump().c_str());
                return std::string();
            }
            return do_sign_impl(signer, sign_for_cert, random_seed, sign_hash);
        }
        const std::string    xauthcontext_t_impl::do_sign(const xvip2_t & signer,const base::xvblock_t * sign_for_block,const uint64_t random_seed)
        {
            if(NULL == sign_for_block)
                return std::string();

            if(false == sign_for_block->is_valid(true)) //do deep test before do sign block
            {
                xerror("xauthcontext_t_impl::do_sign,fail-pass the valid test for block:%s",sign_for_block->dump().c_str());
                return std::string();
            }
            return do_sign_impl(signer,sign_for_block->get_cert(),random_seed);
        }

        ///////////////////////////////////////////verify_sign/////////////////////////////////////////////////////////
        base::enum_vcert_auth_result   xauthcontext_t_impl::verify_sign_impl(const xvip2_t & signer,const base::xvqcert_t * test_for_cert, const std::string ask_verify_hash)
        {
            if(test_for_cert->get_consensus_type() != base::enum_xconsensus_type_xhbft)
            {
                xerror("xauthcontext_t_impl::verify_sign,fail-cert_auth requrest enum_xconsensus_type_xhbft for cert:%s",test_for_cert->dump().c_str());
                return base::enum_vcert_auth_result::enum_bad_consensus;
            }
            xauthscheme_t * verify_scheme_obj = get_auth_scheme(test_for_cert);
            if(NULL == verify_scheme_obj)
            {
                xerror("xauthcontext_t_impl::verify_sign,fail-found related auth scheme for cert:%s",test_for_cert->dump().c_str());
                return base::enum_vcert_auth_result::enum_bad_scheme;
            }
            base::xauto_ptr<base::xvnode_t> verify_node = m_node_service.get_node(signer);
            if(verify_node == nullptr)
            {
                xwarn("xauthcontext_t_impl::verify_sign,fail-found target nodes for signer(%" PRIx64 " : %" PRIx64 ")",signer.high_addr,signer.low_addr);
                return base::enum_vcert_auth_result::enum_nodes_notfound;
            }
            const uint64_t auth_mutisign_token = test_for_cert->get_viewid() + test_for_cert->get_viewtoken();
            if(test_for_cert->is_validator(signer))
            {
                if(verify_scheme_obj->verify_sign(*verify_node, ask_verify_hash, test_for_cert->get_verify_signature(),auth_mutisign_token))
                    return base::enum_vcert_auth_result::enum_successful;
            }
            else if(test_for_cert->is_auditor(signer))
            {
                if(verify_scheme_obj->verify_sign(*verify_node, ask_verify_hash, test_for_cert->get_audit_signature(),auth_mutisign_token))
                    return base::enum_vcert_auth_result::enum_successful;
            }
            xwarn_err("xauthcontext_t_impl::verify_sign,fail-invalid signer(%" PRIx64 " : %" PRIx64 ") for cert=%s",signer.high_addr,signer.low_addr,test_for_cert->dump().c_str());
            return base::enum_vcert_auth_result::enum_verify_fail;
        }
        base::enum_vcert_auth_result   xauthcontext_t_impl::verify_sign(const xvip2_t & signer,const base::xvqcert_t * test_for_cert,const std::string & block_account)
        {
            if(NULL == test_for_cert)
                return base::enum_vcert_auth_result::enum_bad_cert;

            const std::string sign_hash = test_for_cert->get_hash_to_sign();
            return verify_sign(signer, test_for_cert, block_account, sign_hash);
        }
        base::enum_vcert_auth_result     xauthcontext_t_impl::verify_sign(const xvip2_t & signer,const base::xvqcert_t * test_for_cert,
                                                     const std::string & block_account, const std::string sign_hash)
        {
            if(NULL == test_for_cert)
                return base::enum_vcert_auth_result::enum_bad_cert;

            if(false == verify_validator_addr(block_account,test_for_cert))
            {
                xerror("xauthcontext_t_impl::verify_sign,fail-validator address for block:%s",test_for_cert->dump().c_str());
                return base::enum_vcert_auth_result::enum_bad_address;
            }

            if(false == test_for_cert->is_valid())
            {
                xerror("xauthcontext_t_impl::verify_sign,fail-an undeliver cert:%s",test_for_cert->dump().c_str());
                return base::enum_vcert_auth_result::enum_bad_cert;
            }
            base::enum_vcert_auth_result result = verify_sign_impl(signer,test_for_cert, sign_hash);
            if(result != base::enum_vcert_auth_result::enum_successful)
                xwarn("xauthcontext_t_impl::verify_sign,fail-with error code:%d",result);
            return result;
        }
        base::enum_vcert_auth_result   xauthcontext_t_impl::verify_sign(const xvip2_t & signer,const base::xvblock_t * test_for_block)
        {
            if(NULL == test_for_block)
                return base::enum_vcert_auth_result::enum_bad_block;

            if(false == test_for_block->is_valid(true)) //do deep test before verify signature
            {
                xerror("xauthcontext_t_impl::verify_sign,fail-pass the deep test for block:%s",test_for_block->dump().c_str());
                return base::enum_vcert_auth_result::enum_bad_block;
            }
            #ifndef DEBUG //enable exception check at release mode first
            if( (test_for_block->check_block_flag(base::enum_xvblock_flag_authenticated))
               ||(test_for_block->get_cert_hash().empty() == false) ) //should not set any those before verify_sign
            {
                xerror("xauthcontext_t_impl::verify_sign,fail-bad status for block:%s",test_for_block->dump().c_str());
                return base::enum_vcert_auth_result::enum_bad_block;
            }
            #endif
            return verify_sign(signer,test_for_block->get_cert(),test_for_block->get_account());
        }

        ///////////////////////////////////////////merge_muti_sign/////////////////////////////////////////////////////////
        const std::string   xauthcontext_t_impl::merge_muti_sign(const std::vector<xvip2_t> & muti_nodes,const std::vector<std::string> & muti_signatures,const base::xvqcert_t * for_cert)
        {
            if( (NULL == for_cert) || muti_nodes.empty() || muti_signatures.empty() || (muti_nodes.size() != muti_signatures.size()) )
            {
                xerror("xauthcontext_t_impl::merge_muti_sign,fail-bad params");
                return std::string();
            }
            xauthscheme_t * auth_scheme_obj = get_auth_scheme(for_cert);
            if(NULL == auth_scheme_obj)
            {
                xerror("xauthcontext_t_impl::merge_muti_sign,fail-found related auth scheme for cert:%s",for_cert->dump().c_str());
                return std::string();
            }

            xvip2_t target_group_addres = *muti_nodes.begin();
            reset_node_id_to_xip2(target_group_addres);//convert to group address
            for(auto it = muti_nodes.begin(); it != muti_nodes.end(); ++it)
            {
                if(   (false == is_xip2_group_equal(target_group_addres,(*it)))
                   || (get_node_id_from_xip2((*it)) >= get_group_nodes_count_from_xip2(target_group_addres)) )
                {
                    xwarn_err("xauthcontext_t_impl::merge_muti_sign,fail-nodes from difference group,cert:%s",for_cert->dump().c_str());
                    return std::string(); //found any nodes are not at same group
                }
            }
            if(   (is_xip2_group_equal(target_group_addres, for_cert->get_validator())) //merger for validator'signature
               || (is_xip2_group_equal(target_group_addres, for_cert->get_auditor())) )//merger for auditor'signature
            {
                base::xauto_ptr<base::xvnodegroup_t> signers_group = m_node_service.get_group(target_group_addres);
                if(signers_group == nullptr) //verify group address
                {
                    xerror("xauthcontext_t_impl::merge_muti_sign,fail-found any related groups for addr(%" PRIx64 " : %" PRIx64 ")",target_group_addres.high_addr,target_group_addres.low_addr);
                    return std::string();
                }
                xdbgassert(get_group_nodes_count_from_xip2(target_group_addres) == signers_group->get_size());
                #ifdef __check_mini_nodes_count__
                if(signers_group->get_size() < 21) //minimal 21 nodes at least at official env
                {
                    xerror("xauthcontext_t_impl::merge_muti_sign,fail-too little nodes(%u) in group(%" PRIx64 " : %" PRIx64 ")",signers_group->get_size(),target_group_addres.high_addr,target_group_addres.low_addr);
                    return std::string();
                }
                #endif

                //re-calculate threshold count
                uint32_t ask_sign_threshold = (uint32_t)(-1); //init to an unreachable number
                if(is_xip2_group_equal(target_group_addres, for_cert->get_validator()))
                    ask_sign_threshold = for_cert->get_validator_threshold();
                else
                    ask_sign_threshold = for_cert->get_auditor_threshold();

                //do threshold check befor merge
                if(muti_signatures.size() < ask_sign_threshold)
                {
                    xerror("xauthcontext_t_impl::merge_muti_sign,fail-carry too few signatures(%d) for group(%" PRIx64 " : %" PRIx64 ") vs cert=%s",(int32_t)muti_signatures.size(),target_group_addres.high_addr,target_group_addres.low_addr,for_cert->dump().c_str());
                    return std::string();
                }
                //finally go to merge process
                const uint64_t auth_mutisign_token = for_cert->get_viewid() + for_cert->get_viewtoken();
                return auth_scheme_obj->merge_muti_sign(*signers_group,muti_nodes,muti_signatures,auth_mutisign_token);
            }
            xerror("xauthcontext_t_impl::merge_muti_sign,fail-group address of node NOT match cert,group(%" PRIx64 " : %" PRIx64 ") vs cert=%s",target_group_addres.high_addr,target_group_addres.low_addr,for_cert->dump().c_str());
            return std::string();
        }

        //merge multiple single-signature into threshold signature,and return a merged signature
        const std::string   xauthcontext_t_impl::merge_muti_sign(const std::map<xvip2_t,std::string,xvip2_compare> & nodes_and_signatures,const base::xvqcert_t * for_cert)
        {
            if( (NULL == for_cert) || nodes_and_signatures.empty() )
            {
                xerror("xauthcontext_t_impl::merge_muti_sign,fail-bad params");
                return std::string();
            }

            std::vector<xvip2_t>         muti_nodes;
            std::vector<std::string>     muti_signatures;
            muti_nodes.reserve(nodes_and_signatures.size());
            muti_signatures.reserve(nodes_and_signatures.size());
            //collect valid node and signature
            for(auto it = nodes_and_signatures.begin(); it != nodes_and_signatures.end(); ++it)
            {
                muti_nodes.push_back(it->first);
                muti_signatures.push_back(it->second);
            }
            return merge_muti_sign(muti_nodes,muti_signatures,for_cert);
        }

        const std::string   xauthcontext_t_impl::merge_muti_sign(const std::map<xvip2_t,std::string,xvip2_compare> & muti_signatures,const base::xvblock_t * for_block)
        {
            if(NULL == for_block)
                return std::string();

            #ifndef DEBUG //add protection at release mode first
            if( (for_block->check_block_flag(base::enum_xvblock_flag_authenticated))
               ||(for_block->get_cert_hash().empty() == false) ) //should not set any those before verify_sign
            {
                xerror("xauthcontext_t_impl::merge_muti_sign,fail-bad status for block:%s",for_block->dump().c_str());
                return std::string();
            }
            #endif
            return merge_muti_sign(muti_signatures,for_block->get_cert());
        }

        ///////////////////////////////////////////verify_muti_sign/////////////////////////////////////////////////////////
        base::enum_vcert_auth_result   xauthcontext_t_impl::verify_validator_mutisig(xauthscheme_t & auth_scheme_obj,const std::string & ask_verify_hash,const base::xvqcert_t * test_for_cert,std::set<std::string> & exclude_accounts,std::set<std::string> & exclude_keys)
        {
            //sanity test
            if(test_for_cert->get_consensus_type() != base::enum_xconsensus_type_xhbft)
            {
                xerror("xauthcontext_t_impl::verify_validator_mutisig,fail-cert_auth requrest enum_xconsensus_type_xhbft for cert:%s",test_for_cert->dump().c_str());
                return base::enum_vcert_auth_result::enum_bad_consensus;
            }
            base::xauto_ptr<base::xvnodegroup_t> verify_group = m_node_service.get_group(test_for_cert->get_validator());
            if(verify_group == nullptr)
            {
                xwarn("xauthcontext_t_impl::verify_validator_mutisig,fail-found group nodes for validator(%" PRIx64 " : %" PRIx64 ")",test_for_cert->get_validator().high_addr,test_for_cert->get_validator().low_addr);
                return base::enum_vcert_auth_result::enum_nodes_notfound;
            }
            if(test_for_cert->get_clock() < verify_group->get_effect_clock())
            {
                xwarn("xauthcontext_t_impl::verify_validator_mutisig,fail-nodes not be effective yet,effective clock=%" PRIx64 " > cert=%s",verify_group->get_effect_clock(),test_for_cert->dump().c_str());
                return base::enum_vcert_auth_result::enum_nodes_unactived;
            }
            if(get_group_nodes_count_from_xip2(test_for_cert->get_validator()) != verify_group->get_size())
            {
               xerror("xauthcontext_t_impl::verify_validator_mutisig,fail-modified group size(%u) for validator(%" PRIx64 " : %" PRIx64 ")",verify_group->get_size(),test_for_cert->get_validator().high_addr,test_for_cert->get_validator().low_addr);
                return base::enum_vcert_auth_result::enum_bad_address;
            }
            #ifdef __check_mini_nodes_count__
            if(verify_group->get_size() < 21) //minimal 21 nodes at least at official env
            {
                xerror("xauthcontext_t_impl::verify_validator_mutisig,fail-too little nodes(%u) for validator(%" PRIx64 " : %" PRIx64 ")",verify_group->get_size(),test_for_cert->get_validator().high_addr,test_for_cert->get_validator().low_addr);
                return base::enum_vcert_auth_result::enum_nodes_toofew;
            }
            #endif

            //re-calcu the threshold for validators
            const uint32_t ask_verify_threshold = test_for_cert->get_validator_threshold();

            const uint64_t auth_mutisign_token = test_for_cert->get_viewid() + test_for_cert->get_viewtoken();
            if(false == auth_scheme_obj.verify_muti_sign(*verify_group, ask_verify_threshold,ask_verify_hash,test_for_cert->get_verify_signature(),auth_mutisign_token,exclude_accounts,exclude_keys))
            {
                //some blocks may received before elect block
                xwarn("xauthcontext_t_impl::verify_validator_mutisig,fail-verify sinagure of validator for cert:%s",test_for_cert->dump().c_str());
                return base::enum_vcert_auth_result::enum_verify_fail;
            }
            return base::enum_vcert_auth_result::enum_successful;
        }

        base::enum_vcert_auth_result   xauthcontext_t_impl::verify_auditor_mutisig(xauthscheme_t & auth_scheme_obj,const std::string & ask_verify_hash,const base::xvqcert_t * test_for_cert,std::set<std::string> & exclude_accounts,std::set<std::string> & exclude_keys)
        {
            //sanity test
            if(test_for_cert->get_consensus_type() != base::enum_xconsensus_type_xhbft)
            {
                xerror("xauthcontext_t_impl::verify_auditor_mutisig,fail-cert_auth requrest enum_xconsensus_type_xhbft for cert:%s",test_for_cert->dump().c_str());
                return base::enum_vcert_auth_result::enum_bad_consensus;
            }
            base::xauto_ptr<base::xvnodegroup_t> verify_group = m_node_service.get_group(test_for_cert->get_auditor());
            if(verify_group == nullptr)
            {
                xerror("xauthcontext_t_impl::verify_auditor_mutisig,fail-found any related nodes for auditor(%" PRIx64 " : %" PRIx64 ")",test_for_cert->get_auditor().high_addr,test_for_cert->get_auditor().low_addr);
                return base::enum_vcert_auth_result::enum_nodes_notfound;
            }
            if(get_group_nodes_count_from_xip2(test_for_cert->get_auditor()) != verify_group->get_size())
            {
                xerror("xauthcontext_t_impl::verify_validator_mutisig,fail-modified group size(%u) for auditor(%" PRIx64 " : %" PRIx64 ")",verify_group->get_size(),test_for_cert->get_auditor().high_addr,test_for_cert->get_auditor().low_addr);
                return base::enum_vcert_auth_result::enum_bad_address;
            }
            #ifdef __check_mini_nodes_count__
            if(verify_group->get_size() < 21) //minimal 21 nodes at least at official env
            {
                xerror("xauthcontext_t_impl::verify_auditor_mutisig,fail-too little nodes(%u) for auditor(%" PRIx64 " : %" PRIx64 ")",verify_group->get_size(),test_for_cert->get_auditor().high_addr,test_for_cert->get_auditor().low_addr);
                return base::enum_vcert_auth_result::enum_nodes_toofew;
            }
            #endif

            const uint32_t ask_verify_threshold = test_for_cert->get_auditor_threshold();
            const uint64_t auth_mutisign_token  = test_for_cert->get_viewid() + test_for_cert->get_viewtoken();
            if(false == auth_scheme_obj.verify_muti_sign(*verify_group, ask_verify_threshold,ask_verify_hash,test_for_cert->get_audit_signature(),auth_mutisign_token,exclude_accounts,exclude_keys))
            {
                xwarn_err("xauthcontext_t_impl::verify_auditor_mutisig,fail-verify sinagure of validator for cert:%s",test_for_cert->dump().c_str());
                return base::enum_vcert_auth_result::enum_verify_fail;
            }
            return base::enum_vcert_auth_result::enum_successful;
        }

        base::enum_vcert_auth_result   xauthcontext_t_impl::verify_muti_sign_impl(const base::xvqcert_t * test_for_cert)
        {
            const std::string ask_verify_hash = test_for_cert->get_hash_to_sign();
            const std::string cert_hash = const_cast<base::xvqcert_t *>(test_for_cert)->build_block_hash();
            bool result = false;
            bool ret = m_verified_hash.get(cert_hash, result);
            if (ret && result) {
                return base::enum_vcert_auth_result::enum_successful;
            }

            if(false == test_for_cert->is_deliver())
            {
                xerror("xauthcontext_t_impl::verify_muti_sign,fail-an undeliver cert:%s",test_for_cert->dump().c_str());
                return base::enum_vcert_auth_result::enum_bad_cert;
            }
            xauthscheme_t * verify_scheme_obj = get_auth_scheme(test_for_cert);
            if(NULL == verify_scheme_obj)
            {
                xerror("xauthcontext_t_impl::verify_muti_sign,fail-found related auth scheme for cert:%s",test_for_cert->dump().c_str());
                return base::enum_vcert_auth_result::enum_bad_scheme;
            }

            //test whether validator and auditor are at same group,which is not allow
            //by check -[zone-id:7bit|cluster-id:7bit|group-id:8bit|node-id:10bit]
            const uint64_t validator_group_address = test_for_cert->get_validator().low_addr & 0xFFFFFC00;
            const uint64_t auditor_group_address   = test_for_cert->get_auditor().low_addr & 0xFFFFFC00;
            if( validator_group_address == auditor_group_address )
            {
                xerror("xauthcontext_t_impl::verify_muti_sign,fail-duplicated validtor and auditor for cert:%s",test_for_cert->dump().c_str());
                return base::enum_vcert_auth_result::enum_bad_address;
            }
            if(test_for_cert->get_consensus_flags() == base::enum_xconsensus_flag_audit_cert) //ask audit
            {
                //auditor and validator must at same cluster
                if( get_cluster_id_from_xip2(test_for_cert->get_validator()) != get_cluster_id_from_xip2(test_for_cert->get_auditor()) )
                {
                    xerror("xauthcontext_t_impl::verify_muti_sign,fail-invalid validtor and auditor for cert:%s",test_for_cert->dump().c_str());
                    return base::enum_vcert_auth_result::enum_bad_address;
                }
            }

            std::set<std::string>  exclude_nodes;
            std::set<std::string>  exclude_keys;
            base::enum_vcert_auth_result verify_result = verify_validator_mutisig(*verify_scheme_obj,ask_verify_hash,test_for_cert,exclude_nodes,exclude_keys);
            if(verify_result != base::enum_vcert_auth_result::enum_successful )
                return verify_result;

            if(test_for_cert->get_consensus_flags() == base::enum_xconsensus_flag_audit_cert) //ask audit
            {
                base::enum_vcert_auth_result audit_result = verify_auditor_mutisig(*verify_scheme_obj,ask_verify_hash,test_for_cert,exclude_nodes,exclude_keys);
                if(audit_result != base::enum_vcert_auth_result::enum_successful)
                    return audit_result;
            }
            m_verified_hash.put(cert_hash, true);
            return base::enum_vcert_auth_result::enum_successful;
        }

        base::enum_vcert_auth_result   xauthcontext_t_impl::verify_muti_sign(const base::xvqcert_t * target_cert,const std::string & block_account)
        {
            if(NULL == target_cert)
                return base::enum_vcert_auth_result::enum_bad_cert;

            if(false == verify_validator_addr(block_account,target_cert))
            {
                xerror("xauthcontext_t_impl::verify_muti_sign,fail-validator address for block:%s",target_cert->dump().c_str());
                return base::enum_vcert_auth_result::enum_bad_address;
            }

            base::enum_vcert_auth_result result = base::enum_vcert_auth_result::enum_verify_fail;
            if(target_cert->is_consensus_flag_has_extend_cert()) //by extend cert to verify
            {
                base::xauto_ptr<base::xvqcert_t> extend_cert(base::xvblock_t::create_qcert_object(target_cert->get_extend_cert()));
                if(extend_cert == nullptr)
                {
                    xerror("xauthcontext_t_impl::verify_muti_sign,fail-invalid extend cert carried by cert:%s",target_cert->dump().c_str());
                    return base::enum_vcert_auth_result::enum_bad_cert;
                }
                result = verify_muti_sign_impl(extend_cert.get());
            }
            else //go regular cert
            {
                result = verify_muti_sign_impl(target_cert);
            }
            if(result != base::enum_vcert_auth_result::enum_successful)
                xwarn("xauthcontext_t_impl::verify_muti_sign,fail-with error code:%d",result);

            return result;
        }

        base::enum_vcert_auth_result   xauthcontext_t_impl::verify_muti_sign(const base::xvblock_t * test_for_block)
        {
            if(NULL == test_for_block)
                return base::enum_vcert_auth_result::enum_bad_block;
            xdbg("xauthcontext_t_impl::verify_muti_sign enter block=%s",test_for_block->dump().c_str());
            if(false == test_for_block->is_valid(true)) //do deep test before verify signature
            {
                xwarn("xauthcontext_t_impl::verify_muti_sign,fail-pass the deep test for block:%s",test_for_block->dump().c_str());
                return base::enum_vcert_auth_result::enum_bad_block;
            }
            return verify_muti_sign(test_for_block->get_cert(),test_for_block->get_account());
        }

        //return pair of <private-key,public-key>
        //private-key must = 32bytes of raw private key of ECC(secp256k1)
        //public-key  must = 33 bytes of the compressed public key of ECC(secp256k1)
        std::pair<std::string, std::string>  xauthcontext_t::create_secp256k1_keypair(const uint64_t random_seed)
        {
            xmutisig::xprikey _private_key(random_seed);
            xmutisig::xpubkey _public_key(_private_key);

            std::string prikey = _private_key.to_string(); //alignment to generete always 32bytes(256bits) private-key
            std::string pubkey = _public_key.get_serialize_str();

            return std::pair<std::string, std::string>(prikey,pubkey);
        }

        const std::string xauthcontext_t_impl::get_pubkey(const xvip2_t & signer) {
            base::xauto_ptr<base::xvnode_t> signer_node = m_node_service.get_node(signer);
            return signer_node->get_sign_pubkey();
        }

        std::vector<base::xvoter>  xauthcontext_t::query_validators(base::xvblock_t & block,base::xvnodesrv_t & node_service)
        {
            if(block.is_deliver(false))
            {
                base::xauto_ptr<base::xvnodegroup_t> nodes_group = node_service.get_group(block.get_cert()->get_validator());
                if(nodes_group == nullptr)
                {
                    xerror("xauthcontext_t::query_validators,fail-found any related nodes for validators(%" PRIu64 " : %" PRIu64 ")",block.get_cert()->get_validator().high_addr,block.get_cert()->get_validator().low_addr);
                    return std::vector<base::xvoter>();
                }

                std::vector<base::xvoter> voters_list;
                voters_list.reserve(nodes_group->get_size());

                const std::string& aggregated_signatures_bin = block.get_cert()->get_verify_signature();
                xmutisigdata_t aggregated_sig_obj;
                if(aggregated_sig_obj.serialize_from_string(aggregated_signatures_bin) > 0)
                {
                    xnodebitset  & nodebits = aggregated_sig_obj.get_nodebitset();
                    if((uint32_t)nodebits.get_alloc_bits() == nodes_group->get_size()) //must be same
                    {
                        for(int i = 0; i < nodebits.get_alloc_bits(); ++i)
                        {
                            base::xvnode_t * _node_ptr = nodes_group->get_node(i);
                            if(NULL != _node_ptr)
                            {
                                voters_list.emplace_back( base::xvoter(_node_ptr->get_account(),_node_ptr->get_xip2_addr(),nodebits.is_set(i),is_xip2_equal(block.get_cert()->get_validator(), _node_ptr->get_xip2_addr())) );
                            }
                        }
                    }
                }
                return voters_list;
            }
            return std::vector<base::xvoter>();
        }

        std::vector<base::xvoter>  xauthcontext_t::query_auditors(base::xvblock_t & block,base::xvnodesrv_t & node_service)
        {
            if(block.is_deliver(false))
            {
                base::xauto_ptr<base::xvnodegroup_t> nodes_group = node_service.get_group(block.get_cert()->get_auditor());
                if(nodes_group == nullptr)
                {
                    xerror("xauthcontext_t::query_validators,fail-found any related nodes for auditor(%" PRIx64 " : %" PRIx64 ")",block.get_cert()->get_auditor().high_addr,block.get_cert()->get_auditor().low_addr);
                    return std::vector<base::xvoter>();
                }

                std::vector<base::xvoter> voters_list;
                voters_list.reserve(nodes_group->get_size());

                const std::string& aggregated_signatures_bin = block.get_cert()->get_audit_signature();
                xmutisigdata_t aggregated_sig_obj;
                if(aggregated_sig_obj.serialize_from_string(aggregated_signatures_bin) > 0)
                {
                    xnodebitset  & nodebits = aggregated_sig_obj.get_nodebitset();
                    if((uint32_t)nodebits.get_alloc_bits() == nodes_group->get_size()) //must be same
                    {
                        for(int i = 0; i < nodebits.get_alloc_bits(); ++i)
                        {
                            base::xvnode_t * _node_ptr = nodes_group->get_node(i);
                            if(NULL != _node_ptr)
                            {
                                voters_list.emplace_back( base::xvoter(_node_ptr->get_account(),_node_ptr->get_xip2_addr(),nodebits.is_set(i),is_xip2_equal(block.get_cert()->get_validator(), _node_ptr->get_xip2_addr())) );
                            }
                        }
                    }
                }
                return voters_list;
            }
            return std::vector<base::xvoter>();
        }

    }; //end of namespace of auth
};//end of namesapce of top
