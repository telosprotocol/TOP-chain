// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xvledger/xvblockbuild.h"
#include "xvledger/xmerkle.hpp"
#include "xutility/xhash.h"

namespace top
{
    namespace base
    {
        xbbuild_para_t::xbbuild_para_t() {

        }

        xbbuild_para_t::xbbuild_para_t(enum_xchain_id chainid, const std::string & account, enum_xvblock_level _level, enum_xvblock_class _class, const std::string & last_block_hash) {
            m_chainid = chainid;
            m_account = account;
            m_height = 0;
            m_level = _level;
            m_class = _class;
            m_type = base::enum_xvblock_type_genesis;
            m_last_block_hash = last_block_hash;
            m_last_full_block_hash = std::string();
            m_last_full_block_height = 0;
            m_extra_data = std::string();

            set_genesis_qcert();
        }

        xbbuild_para_t::xbbuild_para_t(enum_xchain_id chainid, const std::string & account, uint64_t height, enum_xvblock_class _class, enum_xvblock_level _level, enum_xvblock_type _type, const std::string & last_block_hash, const std::string & last_full_block_hash) {
            m_chainid = chainid;
            m_account = account;
            m_height = height;
            m_level = _level;
            m_class = _class;
            m_type = _type;
            m_last_block_hash = last_block_hash;
            m_last_full_block_hash = last_full_block_hash;
            m_last_full_block_height = 0;
            m_extra_data = std::string();
        }

        xbbuild_para_t::xbbuild_para_t(base::xvblock_t* prev_block, enum_xvblock_class _class, enum_xvblock_type _type) {
            set_header_para(prev_block, _class, _type);
        }

        void xbbuild_para_t::set_header_para(base::xvblock_t* prev_block, enum_xvblock_class _class, enum_xvblock_type _type) {
            m_chainid = prev_block->get_chainid();
            m_account = prev_block->get_account();
            m_height = prev_block->get_height() + 1;
            m_level = prev_block->get_block_level();
            m_class = _class;
            m_type = _type;
            m_last_block_hash = prev_block->get_block_hash();

            if (prev_block->is_genesis_block() || prev_block->get_block_class() == base::enum_xvblock_class_full) {
                m_last_full_block_hash = prev_block->get_block_hash();
                m_last_full_block_height = prev_block->get_height();
            } else {
                m_last_full_block_hash = prev_block->get_last_full_block_hash();
                m_last_full_block_height = prev_block->get_last_full_block_height();
            }
            m_extra_data = std::string();
        }

        void xbbuild_para_t::set_genesis_qcert() {
            m_clock = 0;
            m_viewtoken = -1; // must be -1
            m_viewid = 0;
            m_validator = xvip2_t({(uint64_t)-1, (uint64_t)-1});  // xvqcert_t::is_valid demand m_validator.low_addr should not be 0
            set_empty_xip2(m_auditor);
            m_drand_height = 0;
            m_justify_cert_hash = std::string();
            m_parent_height = 0;

            m_consensus_type = base::enum_xconsensus_type_genesis;
            m_consensus_threshold = base::enum_xconsensus_threshold_anyone;
            m_consensus_flag = base::enum_xconsensus_flag_validator_cert;
            m_sign_scheme = base::enum_xvchain_threshold_sign_scheme_none;
            m_hash_type = enum_xhash_type_sha2_256;
        }
        void xbbuild_para_t::set_default_qcert() {
            m_clock = 0;
            m_viewtoken = -1;
            m_viewid = 0;
            set_empty_xip2(m_validator);
            set_empty_xip2(m_auditor);
            m_drand_height = 0;
            m_justify_cert_hash = std::string();
            m_parent_height = 0;
            m_consensus_type = base::enum_xconsensus_type_xhbft;
            m_consensus_threshold = base::enum_xconsensus_threshold_2_of_3;
            m_consensus_flag = base::enum_xconsensus_flag_validator_cert;
            m_sign_scheme = base::enum_xvchain_threshold_sign_scheme_schnorr;
            m_hash_type = enum_xhash_type_sha2_256;
        }

        void xbbuild_para_t::set_basic_cert_para(uint64_t _clock, uint32_t _viewtoken, uint64_t _viewid, const xvip2_t & _validator) {
            set_default_qcert();
            m_clock = _clock;
            m_viewtoken = _viewtoken;
            m_viewid = _viewid;
            m_validator = _validator;
        }
        void xbbuild_para_t::set_unit_cert_para(uint64_t _clock, uint32_t _viewtoken, uint64_t _viewid, const xvip2_t & _validator, const xvip2_t & _auditor, uint64_t _drand_height,
                                        uint64_t _parent_height, const std::string & _justify_hash) {
            set_default_qcert();
            m_clock = _clock;
            m_viewtoken = _viewtoken;
            m_viewid = _viewid;
            m_validator = _validator;
            m_auditor = _auditor;
            m_drand_height = _drand_height;
            m_parent_height = _parent_height;
            m_justify_cert_hash = _justify_hash;
            if (!is_xip2_empty(_auditor)) {  // optional
                m_consensus_flag = base::enum_xconsensus_flag_extend_and_audit_cert;
            } else {
                m_consensus_flag = base::enum_xconsensus_flag_extend_cert;
            }            
        }
        void xbbuild_para_t::set_simple_cert_para(uint64_t _clock, uint64_t _viewid, uint64_t _parent_height) {
            m_clock = _clock;
            m_expired = 0;
            m_nonce = 0;
            m_viewtoken = 0;
            m_viewid = _viewid;
            set_empty_xip2(m_validator);
            set_empty_xip2(m_auditor);
            m_drand_height = 0;
            m_justify_cert_hash = std::string();
            m_parent_height = _parent_height;
            m_consensus_type = base::enum_xconsensus_type_none;
            m_consensus_threshold = base::enum_xconsensus_threshold_anyone;
            m_consensus_flag = base::enum_xconsensus_flag_simple_cert;
            m_sign_scheme = base::enum_xvchain_threshold_sign_scheme_none;
            m_hash_type = enum_xhash_type_sha2_256;
        }

        void xbbuild_para_t::set_table_cert_para(uint64_t _clock, uint32_t _viewtoken, uint64_t _viewid, const xvip2_t & _validator, const xvip2_t & _auditor, uint64_t _drand_height,
                                    const std::string & _justify_hash, bool need_relay_prove) {
            set_default_qcert();
            m_clock = _clock;
            m_viewtoken = _viewtoken;
            m_viewid = _viewid;
            m_validator = _validator;
            m_auditor = _auditor;
            m_drand_height = _drand_height;
            m_justify_cert_hash = _justify_hash;
            if (need_relay_prove) {
                xassert(is_xip2_empty(m_auditor));// relay table not has auditor network
                m_consensus_flag = base::enum_xconsensus_flag_extend_vote;
            } else {
                if (!is_xip2_empty(_auditor)) {  // optional
                    m_consensus_flag = base::enum_xconsensus_flag_audit_cert;
                } else {
                    m_consensus_flag = base::enum_xconsensus_flag_validator_cert;
                }
            }
        }


        //----------------------------------------xbbuild_body_para_t-------------------------------------//
        void xbbuild_body_para_t::set_resource(bool is_input_resource, const std::string & key, const std::string & value) {
            if (is_input_resource) {
                set_input_resource(key, value);
            } else {
                set_output_resource(key, value);
            }
        }
        void xbbuild_body_para_t::set_input_resource(const std::string & key, const std::string & value) {
            input_resources[key] = value;
        }
        void xbbuild_body_para_t::set_output_resource(const std::string & key, const std::string & value) {
            output_resources[key] = value;
        }



        //----------------------------------------xvblockbuild_t-------------------------------------//
        xvblockbuild_t::xvblockbuild_t() {

        }

        xvblockbuild_t::xvblockbuild_t(base::xvheader_t* header, base::xvqcert_t* cert) {
            set_header(header);
            set_qcert(cert);
        }
        xvblockbuild_t::xvblockbuild_t(base::xvheader_t* header) {
            set_header(header);
        }

        xvblockbuild_t::~xvblockbuild_t() {
            if (m_header_ptr != nullptr) {
                m_header_ptr->release_ref();
            }
            if (m_qcert_ptr != nullptr) {
                m_qcert_ptr->release_ref();
            }
            if (m_input_ptr != nullptr) {
                m_input_ptr->release_ref();
            }
            if (m_output_ptr != nullptr) {
                m_output_ptr->release_ref();
            }
            if (m_block_ptr != nullptr) {
                m_block_ptr->release_ref();
            }
        }

        void xvblockbuild_t::init_qcert(const xbbuild_para_t & _para) {
            if (get_header() == nullptr) {  // must init header firstly
                xassert(false);
                return;
            }
            if (get_qcert() != nullptr) {
                xassert(false);
                return;
            }
            base::xauto_ptr<base::xvqcert_t> _qcert = new base::xvqcert_t();
            auto key_curve_type = get_key_curve_type_from_account(get_header()->get_account());
            _qcert->set_crypto_key_type(key_curve_type);
            _qcert->set_crypto_hash_type(_para.m_hash_type);
            _qcert->set_drand(_para.m_drand_height);
            _qcert->set_consensus_type(_para.m_consensus_type);
            _qcert->set_consensus_threshold(_para.m_consensus_threshold);
            _qcert->set_consensus_flag(_para.m_consensus_flag);
            _qcert->set_crypto_sign_type(_para.m_sign_scheme);
            _qcert->set_clock(_para.m_clock);
            _qcert->set_viewid(_para.m_viewid);
            _qcert->set_viewtoken(_para.m_viewtoken);
            _qcert->set_validator(_para.m_validator);
            _qcert->set_expired(_para.m_expired);
            _qcert->set_nonce(_para.m_nonce);
            if (!is_xip2_empty(_para.m_auditor)) {  // optional
                _qcert->set_auditor(_para.m_auditor);
                // _qcert->set_consensus_flag(base::enum_xconsensus_flag_audit_cert);
            }
            _qcert->set_justify_cert_hash(_para.m_justify_cert_hash);
            if (get_header()->get_block_level() != base::enum_xvblock_level_unit) {
                xassert(_para.m_parent_height == 0);
            }
            _qcert->set_parent_height(_para.m_parent_height);
            set_qcert(_qcert.get());
        }

        xauto_ptr<xvheader_t> xvblockbuild_t::create_header(const xbbuild_para_t & _para) {
            xauto_ptr<xvheader_t> _header = new base::xvheader_t();
            _header->set_chainid(_para.m_chainid);
            _header->set_account(_para.m_account);
            _header->set_height(_para.m_height);
            _header->set_block_level(_para.m_level);
            _header->set_weight(1);
            _header->set_last_block_hash(_para.m_last_block_hash);
            _header->set_block_class(_para.m_class);
            _header->set_block_type(_para.m_type);
            _header->set_last_full_block(_para.m_last_full_block_hash, _para.m_last_full_block_height);
            _header->set_extra_data(_para.m_extra_data);

            if (_para.m_height == 0) {
                _header->set_block_version(xvblock_fork_t::get_block_init_version());
            } else {
                xassert(_para.m_clock != 0);
                uint32_t expect_version = base::xvblock_fork_t::instance().get_expect_block_version(_para.m_clock);
                _header->set_block_version(expect_version);
                if (expect_version >= enum_xvblock_fork_version_6_0_0) {
                    _header->set_block_character(base::enum_xvblock_character_certify_header_only);
                }
            }
            xdbg("xvblockbuild_t::create_header.account=%s,height=%ld,clock=%ld,version=0x%x",_para.m_account.c_str(),_para.m_height,_para.m_clock,_header->get_block_version());
            return _header;
        }

        void xvblockbuild_t::init_header(const xbbuild_para_t & _para) {
            if (get_header() != nullptr) {
                xassert(false);
                return;
            }
            xauto_ptr<xvheader_t> _header = create_header(_para);
            set_header(_header.get());
        }

        void xvblockbuild_t::set_header_extra(const std::string & _extra) {
            if (get_header() == nullptr) {
                xassert(false);
                return;
            }
            get_header()->set_extra_data(_extra);
        }

        void xvblockbuild_t::set_header_comments(const std::string & comments) {
            if (get_header() == nullptr) {
                xassert(false);
                return;
            }
            get_header()->set_comments(comments);
        }
        void xvblockbuild_t::set_block_character(enum_xvblock_character it) {
            if (get_header() == nullptr) {
                xassert(false);
                return;
            }
            get_header()->set_block_character(it);
        }

        xauto_ptr<xvheader_t> xvblockbuild_t::build_proposal_header(xvblock_t* block, uint64_t _clock) {
            xbbuild_para_t _para(block, block->get_block_class(), block->get_block_type());
            _para.m_clock = _clock;
            return create_header(_para);
        }

        xauto_ptr<xvheader_t> xvblockbuild_t::build_proposal_header(std::string const& account, uint64_t height, std::string const& last_block_hash, uint64_t _clock) {
            xbbuild_para_t _para;
            _para.m_account = account;
            _para.m_height = height;
            _para.m_last_block_hash = last_block_hash;
            _para.m_clock = _clock;
            return create_header(_para);
        }

        void xvblockbuild_t::init_header_qcert(const xbbuild_para_t & _para) {
            init_header(_para);
            init_qcert(_para);
        }

        void xvblockbuild_t::set_header(xvheader_t* _header) {
            xassert(m_header_ptr == nullptr);
            _header->add_ref();
            m_header_ptr = _header;
        }
        void xvblockbuild_t::set_qcert(xvqcert_t* _qcert) {
            xassert(m_qcert_ptr == nullptr);
            _qcert->add_ref();
            m_qcert_ptr = _qcert;
        }
        void xvblockbuild_t::set_output(xvoutput_t* _output) {
            xassert(m_output_ptr == nullptr);
            _output->add_ref();
            m_output_ptr = _output;
        }
        void xvblockbuild_t::set_input(xvinput_t* _input) {
            xassert(m_input_ptr == nullptr);
            _input->add_ref();
            m_input_ptr = _input;
        }
        void xvblockbuild_t::set_block(xvblock_t* _block) {
            xassert(m_block_ptr == nullptr);
            _block->add_ref();
            m_block_ptr = _block;
        }

        base::enum_xvchain_key_curve xvblockbuild_t::get_key_curve_type_from_account(const std::string & account) {
            base::enum_xvchain_key_curve  key_curve_type;
            base::enum_vaccount_addr_type addr_type = base::xvaccount_t::get_addrtype_from_account(account);
            switch (addr_type) {
            case base::enum_vaccount_addr_type_secp256k1_user_account:
            case base::enum_vaccount_addr_type_secp256k1_eth_user_account:
            case base::enum_vaccount_addr_type_secp256k1_evm_user_account:
            // case base::enum_vaccount_addr_type_secp256k1_user_sub_account:
            case base::enum_vaccount_addr_type_native_contract:
            // case base::enum_vaccount_addr_type_custom_contract:
            case base::enum_vaccount_addr_type_black_hole:

            case base::enum_vaccount_addr_type_block_contract: {
                key_curve_type = base::enum_xvchain_key_curve_secp256k1;
            } break;

            case base::enum_vaccount_addr_type_ed25519_user_account:
            case base::enum_vaccount_addr_type_ed25519_user_sub_account: {
                key_curve_type = base::enum_xvchain_key_curve_ed25519;
            } break;

            default: {
                if (((int)addr_type >= base::enum_vaccount_addr_type_ed25519_reserved_start) && ((int)addr_type <= base::enum_vaccount_addr_type_ed25519_reserved_end)) {
                    key_curve_type = base::enum_xvchain_key_curve_ed25519;
                } else {
                    key_curve_type = base::enum_xvchain_key_curve_secp256k1;
                }
            } break;
            }
            return key_curve_type;
        }

        base::enum_xvblock_level xvblockbuild_t::get_block_level_from_account(const std::string & account) {
            base::enum_vaccount_addr_type addrtype = base::xvaccount_t::get_addrtype_from_account(account);
            base::enum_xvblock_level _level;
            if (addrtype == base::enum_vaccount_addr_type_block_contract) {
                _level = base::enum_xvblock_level_table;
            } else if (addrtype == base::enum_vaccount_addr_type_timer) {
                _level = base::enum_xvblock_level_root;
            } else if (addrtype == base::enum_vaccount_addr_type_drand) {
                _level = base::enum_xvblock_level_root;
            } else if (addrtype == base::enum_vaccount_addr_type_root_account) {
                _level = base::enum_xvblock_level_chain;
            } else {
                _level = base::enum_xvblock_level_unit;
            }
            return _level;
        }

        base::enum_xvblock_type xvblockbuild_t::get_block_type_from_empty_block(const std::string & account) {
            base::enum_vaccount_addr_type addrtype = base::xvaccount_t::get_addrtype_from_account(account);
            base::enum_xvblock_type _type;
            if (addrtype == base::enum_vaccount_addr_type_timer || addrtype == base::enum_vaccount_addr_type_drand) {
                _type = base::enum_xvblock_type_clock;
            } else {
                _type = base::enum_xvblock_type_general;
            }
            return _type;
        }

        void xvblockbuild_t::set_block_flags(xvblock_t* block) {
            // genesis block always set consensused forcely
            if (block->get_height() == 0) {
                block->set_block_flag(base::enum_xvblock_flag_authenticated);
                block->set_block_flag(base::enum_xvblock_flag_locked);
                block->set_block_flag(base::enum_xvblock_flag_committed);
                // set_block_flag(base::enum_xvblock_flag_executed);
                block->set_block_flag(base::enum_xvblock_flag_confirmed);
                // set_block_flag(base::enum_xvblock_flag_connected);
                block->reset_modified_count();
                xassert(block->is_input_ready(false));
                xassert(block->is_output_ready(false));
                xassert(block->is_deliver(false));
            }
        }

        std::string  xvblockbuild_t::build_mpt_root(const std::vector<std::string> & elements)
        {
            xmerkle_t<utl::xsha2_256_t, uint256_t> merkle; // TODO(jimmy)
            std::string root = merkle.calc_root(elements);
            xassert(!root.empty());
            return root;
        }

        base::xauto_ptr<base::xvblock_t> xvblockbuild_t::create_new_block() {
            if (get_header() == nullptr || get_qcert() == nullptr) {
                xassert(get_header() != nullptr);
                xassert(get_qcert() != nullptr);
                return nullptr;
            }
            if (get_header()->get_block_class() == enum_xvblock_class_nil) {
                return new xvblock_t(*get_header(), *get_qcert(), nullptr, nullptr);
            }
            return new xvblock_t(*get_header(), *get_qcert(), get_input(), get_output());
        }

        //----------------------------------------xvblockmaker_t-------------------------------------//
        xvblockmaker_t::xvblockmaker_t() {
        }
        xvblockmaker_t::xvblockmaker_t(base::xvheader_t* header) {
            set_header(header);
        }
        xvblockmaker_t::xvblockmaker_t(base::xvheader_t* header, base::xvinput_t* input, base::xvoutput_t* output) {
            set_header(header);
            set_input(input);
            set_output(output);
        }
        xvblockmaker_t::~xvblockmaker_t() {
            if (m_input_resource != nullptr) {
                m_input_resource->release_ref();
            }
            if (m_output_resource != nullptr) {
                m_output_resource->release_ref();
            }
            if (m_primary_input_entity != nullptr) {
                m_primary_input_entity->release_ref();
            }
            if (m_primary_output_entity != nullptr) {
                m_primary_output_entity->release_ref();
            }
        }

        bool xvblockmaker_t::set_input_entity(const xvaction_t & action) {
            if (m_primary_input_entity != nullptr) {  // only allow init once
                xassert(false);
                return false;
            }
            if (get_header()->get_block_class() == base::enum_xvblock_class_nil) {
                xassert(false);
                return false;
            }

            std::vector<xvaction_t> actions;
            actions.push_back(action);
            m_primary_input_entity = new xvinentity_t(actions);
            return true;
        }
        bool xvblockmaker_t::set_input_entity(const std::vector<xvaction_t> & actions, std::string extend_bin) {
            if (m_primary_input_entity != nullptr) {  // only allow init once
                xassert(false);
                return false;
            }
            if (get_header()->get_block_class() == base::enum_xvblock_class_nil) {
                xassert(false);
                return false;
            }

            m_primary_input_entity = new xvinentity_t(extend_bin, actions);
            return true;
        }

        bool    xvblockmaker_t::set_output_entity(const std::string & key, const std::string & value) {
            if (!value.empty()) {
                if (nullptr == m_primary_output_entity) {
                    m_primary_output_entity = new base::xvoutentity_t();                
                }
                m_primary_output_entity->set_value(key, value);
            }            
            return true;
        }
        xvoutentity_t* xvblockmaker_t::create_empty_output_entity() {
            if (nullptr == m_primary_output_entity) {
                m_primary_output_entity = new base::xvoutentity_t();
            }
            return m_primary_output_entity;
        }

        bool    xvblockmaker_t::set_input_resource(const std::string & key, const std::string & value) {
            if (key.empty() || value.empty()) {
                xassert(false);
                return false;
            }
            if (nullptr == m_input_resource) {
                m_input_resource  = new xstrmap_t();
            }
            m_input_resource->set(key, value);
            return true;
        }

        bool    xvblockmaker_t::set_output_resource(const std::string & key, const std::string & value) {
            if (key.empty() || value.empty()) {
                xassert(false);
                return false;
            }
            if (nullptr == m_output_resource) {
                m_output_resource = new xstrmap_t();  
            }          
            m_output_resource->set(key, value);
            return true;
        }

        bool xvblockmaker_t::set_output_full_state_hash(const std::string & value) {
            if (value.empty() || get_header()->get_block_class() == enum_xvblock_class_nil) {
                xassert(false);
                return false;
            }
            m_full_state_hash = value;
            return true;
        }

        bool xvblockmaker_t::set_output_full_state(const std::string & value) {
            if (value.empty() || get_header()->get_block_class() == enum_xvblock_class_nil) {
                xassert(false);
                return false;
            }
            m_full_state = value;
            m_full_state_hash = base::xcontext_t::instance().hash(value, get_qcert()->get_crypto_hash_type());
            return true;
        }
        bool xvblockmaker_t::set_output_full_state_without_hash(const std::string & value) {
            if (value.empty() || get_header()->get_block_class() == enum_xvblock_class_nil) {
                xassert(false);
                return false;
            }
            m_full_state = value;
            // m_full_state_hash = base::xcontext_t::instance().hash(value, get_qcert()->get_crypto_hash_type());
            return true;
        }        
        bool xvblockmaker_t::set_output_binlog(const std::string & value) {
            if (value.empty() || get_header()->get_block_class() != enum_xvblock_class_light) {
                // if (get_header()->get_block_class() != enum_xvblock_class_full) {
                    xassert(false);
                    return false;
                // }
            }

            std::string binlog_hash = base::xcontext_t::instance().hash(value, get_qcert()->get_crypto_hash_type());
            set_output_entity(xvoutentity_t::key_name_binlog_hash(), binlog_hash);
            set_output_resource(binlog_hash, value);
            return true;
        }

        bool xvblockmaker_t::set_output_offdata(const std::string & value) {
            m_output_offdata = value;
            return true;
        }

        bool  xvblockmaker_t::merge_input_resource(const xstrmap_t * src_map) {
            if(NULL == src_map)
                return false;

            //combine resource of input
            get_input_resource()->merge(*src_map);
            return true;
        }

        bool  xvblockmaker_t::merge_output_resource(const xstrmap_t * src_map) {
            if(NULL == src_map)
                return false;

            //combine resource of output
            get_output_resource()->merge(*src_map);
            return true;
        }

        std::vector<std::string> xvblockmaker_t::get_input_merkle_leafs(xvinput_t* input) {
            // input root = merkle(all input actions of all entitys)
            std::vector<std::string> leafs;
            auto & all_entitys = input->get_entitys();
            for (auto & entity : all_entitys) {
                // it must be xinentitys
                xvinentity_t* _inentity = dynamic_cast<xvinentity_t*>(entity);
                if (_inentity == nullptr) {
                    xassert(false);
                    return {};
                }
                auto & all_actions = _inentity->get_actions();
                for (auto & action : all_actions) {
                    std::string action_bin;
                    action.serialize_to(action_bin);
                    if (!action_bin.empty()) {
                        leafs.push_back(action_bin);
                    }
                }
            }

            return leafs;
        }

        bool xvblockmaker_t::calc_merkle_path(const std::vector<std::string> & leafs, const std::string & leaf, xmerkle_path_256_t& hash_path) {
            auto iter = std::find(leafs.begin(), leafs.end(), leaf);
            if (iter == leafs.end()) {
                xassert(0);
                return false;
            }

            int index = static_cast<int>(std::distance(leafs.begin(), iter));
            xmerkle_t<utl::xsha2_256_t, uint256_t> merkle;
            return merkle.calc_path(leafs, index, hash_path.get_levels_for_write());
        }

        bool xvblockmaker_t::calc_merkle_path(const std::vector<std::string> & leafs, const xvaction_t & leaf, xmerkle_path_256_t& hash_path) {
            std::string action_bin;
            leaf.serialize_to(action_bin);
            return calc_merkle_path(leafs, action_bin, hash_path);
        }
        
        bool xvblockmaker_t::calc_merkle_path(const std::string & leaf, xmerkle_path_256_t& hash_path, xmerkle_t<utl::xsha2_256_t, uint256_t> &merkle) {
            return merkle.calc_path_hash(leaf, hash_path.get_levels_for_write());
        }

        bool xvblockmaker_t::calc_merkle_path(const xvaction_t & leaf, xmerkle_path_256_t& hash_path, xmerkle_t<utl::xsha2_256_t, uint256_t> &merkle) {
            std::string action_bin;
            leaf.serialize_to(action_bin);
            return calc_merkle_path(action_bin, hash_path, merkle);
        }

        bool xvblockmaker_t::calc_input_merkle_path(xvinput_t* input, const std::string & leaf, xmerkle_path_256_t& hash_path) {
            if(input == nullptr)
                return false;

            std::vector<std::string> leafs = get_input_merkle_leafs(input);
            if (leafs.empty()) {
                xassert(0);
                return false;
            }
            return calc_merkle_path(leafs, leaf, hash_path);
        }

        // input root = merkle (all input actions)
        bool xvblockmaker_t::make_input_root(xvinput_t* input_obj) {
            //build action' mpt tree,and assign mpt root as input root hash
            std::vector<std::string> leafs = get_input_merkle_leafs(input_obj);
            if (!leafs.empty()) {
                std::string root_hash = build_mpt_root(leafs);
                if (!root_hash.empty()) {
                    input_obj->set_root_hash(root_hash);
                    return true;
                }
            }
            xerror("xvblockmaker_t::make_input_root non-nil block must has input root");
            return false;
        }

        xauto_ptr<xvinput_t> xvblockmaker_t::make_input() {
            xassert(get_input_entity() != nullptr);
            // basic block only has one entity
            std::vector<xventity_t*> _entities;
            _entities.emplace_back(get_input_entity());
            xauto_ptr<xvinput_t>input_obj(new xvinput_t(_entities,get_input_resource()));
            return input_obj;
        }

        bool    xvblockmaker_t::build_input() {
            if (get_header()->get_block_class() == base::enum_xvblock_class_nil) { // nil block no need
                xassert(false);
                return false;
            }

            if (get_input() == nullptr) {
                xauto_ptr<xvinput_t>input_obj = make_input();
                set_input(input_obj.get());
            }
            if (get_input()->get_root_hash().empty()) {
                if (false == make_input_root(get_input())) {
                    xassert(false);
                    return false;
                }
            }
            return true;
        }

        bool    xvblockmaker_t::make_output_root(xvoutput_t* output_obj) {
            if (get_full_state_hash().empty()) {
                xassert(false);
                return false;
            }
            // basic block qcert output root is state hash
            output_obj->set_root_hash(get_full_state_hash());
            return true;
        }

        xauto_ptr<xvoutput_t> xvblockmaker_t::make_output() {
            if (get_output_entity() == nullptr) {
                create_empty_output_entity(); // XTODO must has one output entity for compatibility
            }
            xassert(get_output_entity() != nullptr);
            //basic block has only one entity
            std::vector<xventity_t*> _entities;
            _entities.emplace_back(get_output_entity());
            //full-unit should has full-state in output resource
            if (get_header()->get_block_class() == enum_xvblock_class_full && get_header()->get_block_level() != enum_xvblock_level_table) {
                if (get_full_state().empty()) {
                    xassert(false);
                    return nullptr;
                }
                set_output_resource(get_full_state_hash(), get_full_state());// TODO(jimmy)
            }
            xauto_ptr<xvoutput_t>output_obj(new xvoutput_t(_entities,get_output_resource()));
            return output_obj;
        }

        bool    xvblockmaker_t::build_output() {
            if (get_output() != nullptr) {  // only allow once
                return true;
            }

            if (get_header()->get_block_class() == base::enum_xvblock_class_nil) {  // nil block no need
                xassert(false);
                return false;
            }

            if (get_output() == nullptr) {
                xauto_ptr<xvoutput_t>output_obj = make_output();
                set_output(output_obj.get());
            }
            if (get_output()->get_root_hash().empty()) {
                if (false == make_output_root(get_output())) {
                    xassert(false);
                    return false;
                }
            }
            return true;
        }

        bool xvblockmaker_t::check_block_rules(base::xvblock_t* target_block) {
            enum_xvblock_class _class = target_block->get_block_class();
            if (_class == enum_xvblock_class_nil) {
                if (//!target_block->get_fullstate_hash().empty()
                    // || !target_block->get_binlog_hash().empty()
                    !target_block->get_input_root_hash().empty()
                    || !target_block->get_output_root_hash().empty()) {
                    xassert(false);
                    return false;
                }
            }

            if (_class != enum_xvblock_class_nil) {
                if (target_block->get_input_root_hash().empty()) {
                    xassert(false);
                    return false;
                }
            }

            if (_class == enum_xvblock_class_full) {
                // full-block has state hash, not has binlog hash
                if (target_block->get_fullstate_hash().empty()) {
                    xassert(false);
                    return false;
                }
                if (!target_block->get_binlog_hash().empty()) {
                    xassert(false);
                    return false;
                }
                // full-table not has state
                if (target_block->get_block_level() == enum_xvblock_level_table && !target_block->get_full_state().empty()) {
                    xassert(false);
                    return false;
                }
                //full-unit has state
                if (target_block->get_block_level() != enum_xvblock_level_table && target_block->get_full_state().empty()) {
                    xassert(false);
                    return false;
                }
                // full-blcok not has binlog
                if (!target_block->get_binlog().empty()) {
                    xassert(false);
                    return false;
                }
            }
            if (_class == enum_xvblock_class_light) {
                // light-block has state hash and binlog hash
                if (target_block->get_fullstate_hash().empty() || target_block->get_binlog_hash().empty()) {
                    xassert(false);
                    return false;
                }
                // fullstate hash should not same with binlog hash
                // if (target_block->get_fullstate_hash() == target_block->get_binlog_hash()) {
                //     xassert(false);
                //     return false;
                // }
                // // light-block not has state
                // if (!target_block->get_full_state().empty()) {
                //     xassert(false);
                //     return false;
                // }
                // // light-block has binlog
                // if (target_block->get_binlog().empty()) {
                //     xassert(false);
                //     return false;
                // }
            }
            return true;
        }

        base::xauto_ptr<base::xvblock_t> xvblockmaker_t::build_new_block() {
            if (get_block() != nullptr) {  // only allow once
                xassert(false);
                return nullptr;
            }
            // set input and set output before make new block
            if (get_header() == nullptr || get_qcert() == nullptr) {
                xassert(get_header() != nullptr);
                xassert(get_qcert() != nullptr);
                return nullptr;
            }
            if (get_header()->get_block_class() != base::enum_xvblock_class_nil) {
                if (false == build_input()) {
                    xassert(false);
                    return nullptr;
                }
                if (false == build_output()) {
                    xassert(false);
                    return nullptr;
                }
            }

            base::xauto_ptr<base::xvblock_t> _block_ptr = create_new_block();
            if (_block_ptr == nullptr) {
                xassert(false);
                return nullptr;
            }
            set_block_flags(_block_ptr.get());
            set_block(_block_ptr.get());
            
            if (!get_output_offdata().empty()) {
                if (false == _block_ptr->set_output_offdata(get_output_offdata())) {
                    xassert(false);
                    return nullptr;
                }
            }

            if (false == check_block_rules(_block_ptr.get())) {
                xerror("xvblockmaker_t::build_new_block,done,%s,ir=%s,jc=%s,input=%s,output=%s",
                    get_block()->dump().c_str(), base::xstring_utl::to_hex(get_block()->get_input_root_hash()).c_str(), base::xstring_utl::to_hex(get_block()->get_justify_cert_hash()).c_str(),
                    get_input()->dump().c_str(), get_output()->dump().c_str());
                return nullptr;
            }

            if (get_block()->get_block_class() != base::enum_xvblock_class_nil) {
                xdbg("xvblockmaker_t::build_new_block,done,%s,ir=%s,jc=%s,input=%s,output=%s,binlog=%zu,fullstate=%zu,offdata=%zu",
                    get_block()->dump().c_str(), base::xstring_utl::to_hex(get_block()->get_input_root_hash()).c_str(), base::xstring_utl::to_hex(get_block()->get_justify_cert_hash()).c_str(),
                    get_input()->dump().c_str(), get_output()->dump().c_str(), get_block()->get_binlog().size(), get_block()->get_full_state().size(),
                    get_output_offdata().size());
            } else {
                xdbg("xvblockmaker_t::build_new_block,done,%s", get_block()->dump().c_str());                
            }

            return _block_ptr;
        }

        //----------------------------------------xtable_unit_resource_t-------------------------------------//
        xtable_unit_resource_t::xtable_unit_resource_t(xvblock_t* _block) {
            m_unit_input_resources = _block->get_input_data();
            m_unit_output_resources = _block->get_output_data();
        }
        xtable_unit_resource_t::~xtable_unit_resource_t() {

        }

        int32_t xtable_unit_resource_t::do_write(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            stream.write_compact_var(m_unit_input_resources);
            stream.write_compact_var(m_unit_output_resources);
            return (stream.size() - begin_size);
        }

        int32_t xtable_unit_resource_t::do_read(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            stream.read_compact_var(m_unit_input_resources);
            stream.read_compact_var(m_unit_output_resources);
            return (begin_size - stream.size());
        }
        int32_t xtable_unit_resource_t::serialize_to_string(std::string & _str) {
            base::xautostream_t<1024> _raw_stream(xcontext_t::instance());
            int32_t ret = do_write(_raw_stream);
            _str.assign((const char*)_raw_stream.data(),_raw_stream.size());
            return ret;
        }
        int32_t xtable_unit_resource_t::serialize_from_string(const std::string & _str) {
            base::xstream_t _stream(base::xcontext_t::instance(), (uint8_t *)_str.data(), (int32_t)_str.size());
            int32_t ret = do_read(_stream);
            return ret;
        }
        //----------------------------------------xtable_inentity_extend_t-------------------------------------//
        xtable_inentity_extend_t::xtable_inentity_extend_t() {

        }
        xtable_inentity_extend_t::xtable_inentity_extend_t(xvheader_t* header, xvqcert_t* qcert) {
            header->add_ref();
            m_unit_header = header;
            m_unit_justify_hash = qcert->get_justify_cert_hash();
            m_unit_output_root_hash = qcert->get_output_root_hash();
            m_unit_input_root_hash = std::string();  // not set now, may be used future
        }
        xtable_inentity_extend_t::~xtable_inentity_extend_t() {
        }

        int32_t xtable_inentity_extend_t::do_write(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            std::string vheader_bin;
            xassert(m_unit_header != nullptr);
            if (m_unit_header != nullptr) {
                m_unit_header->serialize_to_string(vheader_bin);
            }
            stream.write_compact_var(vheader_bin);
            stream.write_compact_var(m_unit_justify_hash);
            stream.write_compact_var(m_unit_output_root_hash);
            stream.write_compact_var(m_unit_input_root_hash);
            return (stream.size() - begin_size);
        }

        int32_t xtable_inentity_extend_t::do_read(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            std::string vheader_bin;
            stream.read_compact_var(vheader_bin);
            if (!vheader_bin.empty()) {
                xvheader_t*  vheader_ptr = xvblock_t::create_header_object(vheader_bin);
                xassert(vheader_ptr != NULL); //should has value
                if(vheader_ptr != NULL) {
                    m_unit_header.attach(vheader_ptr); //create_header_object has added reference
                }
            }
            stream.read_compact_var(m_unit_justify_hash);
            stream.read_compact_var(m_unit_output_root_hash);
            stream.read_compact_var(m_unit_input_root_hash);
            return (begin_size - stream.size());
        }
        int32_t xtable_inentity_extend_t::serialize_to_string(std::string & _str) {
            base::xautostream_t<1024> _raw_stream(xcontext_t::instance());
            int32_t ret = do_write(_raw_stream);
            _str.assign((const char*)_raw_stream.data(),_raw_stream.size());
            return ret;
        }
        int32_t xtable_inentity_extend_t::serialize_from_string(const std::string & _str) {
            base::xstream_t _stream(base::xcontext_t::instance(), (uint8_t *)_str.data(), (int32_t)_str.size());
            int32_t ret = do_read(_stream);
            return ret;
        }

        //----------------------------------------xvtableblock_maker_t-------------------------------------//
        xvtableblock_maker_t::xvtableblock_maker_t() {

        }
        xvtableblock_maker_t::~xvtableblock_maker_t() {

        }
        bool xvtableblock_maker_t::set_batch_units(const std::vector<xobject_ptr_t<xvblock_t>> & _batch_units) {
            // if (!m_batch_units.empty()) {
            //     xassert(false);
            //     return false;
            // }
            // m_batch_units = _batch_units;

            // uint32_t count = (uint32_t)_batch_units.size();
            // for (uint32_t index = 0; index < count; index++) {
            //     auto & _unit = _batch_units[index];
            //     merge_input_resource(_unit->get_input()->get_resources());
            //     merge_output_resource(_unit->get_output()->get_resources());
            // }
            assert(false);
            return true;
        }

        std::vector<std::string> xvtableblock_maker_t::get_table_out_merkle_leafs(const std::vector<xobject_ptr_t<xvblock_t>> & _batch_units) {
            // build merkle root. table input root = merkle(all units input root)
            std::vector<std::string> input_leafs;
            for (auto & _unit : _batch_units) {
                std::string leaf = get_table_out_merkle_leaf(_unit.get());
                xassert(!leaf.empty());
                input_leafs.push_back(leaf);
            }
            return input_leafs;
        }
        std::string xvtableblock_maker_t::get_table_out_merkle_leaf(base::xvblock_t* _unit) {
            return _unit->get_cert()->get_hash_to_sign();
        }

        bool xvtableblock_maker_t::units_set_parent_cert(std::vector<xobject_ptr_t<xvblock_t>> & units, const xvblock_t* parent) {
            std::vector<std::string> out_leafs = get_table_out_merkle_leafs(units);

            std::string parent_cert_bin;
            parent->get_cert()->serialize_to_string(parent_cert_bin);
            xmerkle_t<utl::xsha2_256_t, uint256_t> merkle(out_leafs);
            for (size_t leaf_index = 0; leaf_index < out_leafs.size(); leaf_index++) {
                auto & _unit = units[leaf_index];
                if (!_unit->get_cert()->get_extend_cert().empty()) { // already set extend cert
                    xassert(false);
                    return true;
                }
                auto const &_leaf = out_leafs[leaf_index];
                base::xmerkle_path_256_t path;
                bool ret = xvblockmaker_t::calc_merkle_path(_leaf, path, merkle);
                if (!ret) {
                    xerror("xvtableblock_maker_t::units_set_parent_cert fail calc merkle path");
                    return false;
                }

                _unit->set_extend_cert(parent_cert_bin);

                base::xstream_t _stream(base::xcontext_t::instance());
                path.serialize_to(_stream);
                std::string extend_data = std::string((char *)_stream.data(), _stream.size());
                _unit->set_extend_data(extend_data);

                _unit->set_block_flag(base::enum_xvblock_flag_authenticated);
                xassert(!_unit->get_block_hash().empty());
            }
            return true;
        }

        xtable_unit_resource_ptr_t xvtableblock_maker_t::query_unit_resource(const base::xvblock_t* _tableblock, uint32_t index) {
            xassert(index != 0);  // index#0 is table primary
            std::string _res_key = "u" + base::xstring_utl::tostring(index);
            std::string _res_value = _tableblock->query_input_resource(_res_key);
            xassert(!_res_value.empty());

            xtable_unit_resource_ptr_t _unit_res = std::make_shared<xtable_unit_resource_t>();
            _unit_res->serialize_from_string(_res_value);
            return _unit_res;
        }

        xauto_ptr<xvinput_t> xvtableblock_maker_t::make_input() {
            // std::vector<xventity_t*> all_input_entities;
            // get_input_entity()->add_ref();
            // all_input_entities.push_back(get_input_entity());

            // const std::vector<xobject_ptr_t<xvblock_t>> & _batch_units = get_batch_units();
            // for (auto & _unit : _batch_units) {
            //     auto & unit_input_entitys = _unit->get_input()->get_entitys();
            //     std::string extend_bin;
            //     xtable_inentity_extend_t extend(_unit->get_header(), _unit->get_cert());
            //     extend.serialize_to_string(extend_bin);

            //     xvinentity_t* new_entity = nullptr;
            //     if (unit_input_entitys.empty()) {
            //         new_entity = new xvinentity_t(extend_bin,std::vector<xvaction_t>{});
            //     } else {
            //         xassert(unit_input_entitys.size() == 1);
            //         new_entity = new xvinentity_t(extend_bin,((xvinentity_t*)unit_input_entitys[0])->get_actions());
            //     }
            //     all_input_entities.push_back(new_entity);
            // }
            // xauto_ptr<xvinput_t>input_obj(new xvinput_t(all_input_entities, *get_input_resource()));
            // for (auto & v : all_input_entities) {  // should release entitys after make obj
            //     v->release_ref();
            // }
            // return input_obj;
            assert(false);
            return nullptr;
        }

        xauto_ptr<xvoutput_t> xvtableblock_maker_t::make_output() {
            // // light-table set full-state hash to primary output entity, other block set full-state hash to output root hash
            // if (!get_full_state_hash().empty()) {
            //     set_output_entity(xvoutentity_t::key_name_state_hash(), get_full_state_hash());
            // } else {
            //     xassert(false);
            //     return nullptr;
            // }

            // std::vector<xventity_t*> all_output_entities;
            // get_output_entity()->add_ref();
            // all_output_entities.push_back(get_output_entity());

            // const std::vector<xobject_ptr_t<xvblock_t>> & _batch_units = get_batch_units();
            // for (auto & _unit : _batch_units) {
            //     auto & unit_output_entitys = _unit->get_output()->get_entitys();
            //     xvoutentity_t* new_entity = nullptr;
            //     if (unit_output_entitys.empty()) {
            //         new_entity = new xvoutentity_t();  // must has related output entity with input entity
            //     } else {
            //         xassert(unit_output_entitys.size() == 1);
            //         new_entity = new xvoutentity_t(*((xvoutentity_t*)unit_output_entitys[0]));  // table entity is same with unit entity
            //     }
            //     all_output_entities.push_back(new_entity);//transfered owner of ptr to vector
            // }
            // xauto_ptr<xvoutput_t>output_obj(new xvoutput_t(all_output_entities, *get_output_resource()));
            // for (auto & v : all_output_entities) {  // should release entitys after make obj
            //     v->release_ref();
            // }
            // return output_obj;
            assert(false);
            return nullptr;
        }

        bool    xvtableblock_maker_t::make_output_root(xvoutput_t* output_obj) {
            // table output root = merkle(all units sign hash)
            std::vector<std::string> output_leafs = get_table_out_merkle_leafs(get_batch_units());
            if (output_leafs.empty()) {
                return true;
            }
            std::string root_hash = build_mpt_root(output_leafs);
            if (root_hash.empty()) {
                xassert(false);
                return false;
            }
            output_obj->set_root_hash(root_hash);
            return true;
        }

    }//end of namespace of base
}//end of namespace top
