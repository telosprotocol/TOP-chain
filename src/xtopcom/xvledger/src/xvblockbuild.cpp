// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xvledger/xvblockbuild.h"

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
            m_consensus_flag = enum_xconsensus_flag(0);
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
            m_consensus_flag = enum_xconsensus_flag(0);
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
            m_consensus_flag = base::enum_xconsensus_flag_extend_cert;
        }
        void xbbuild_para_t::set_table_cert_para(uint64_t _clock, uint32_t _viewtoken, uint64_t _viewid, const xvip2_t & _validator, const xvip2_t & _auditor, uint64_t _drand_height,
                                    const std::string & _justify_hash) {
            set_default_qcert();
            m_clock = _clock;
            m_viewtoken = _viewtoken;
            m_viewid = _viewid;
            m_validator = _validator;
            m_auditor = _auditor;
            m_drand_height = _drand_height;
            m_justify_cert_hash = _justify_hash;
        }


        //----------------------------------------xvblockbuild_t-------------------------------------//
        xvblockbuild_t::xvblockbuild_t() {

        }

        xvblockbuild_t::xvblockbuild_t(base::xvheader_t* header, base::xvqcert_t* cert, base::xvinput_t* input, base::xvoutput_t* output) {
            header->add_ref();
            m_header = header;
            cert->add_ref();
            m_qcert = cert;
            input->add_ref();
            m_input_ptr = input;
            output->add_ref();
            m_output_ptr = output;
        }
        xvblockbuild_t::xvblockbuild_t(base::xvheader_t* header, base::xvinput_t* input, base::xvoutput_t* output) {
            header->add_ref();
            m_header = header;
            input->add_ref();
            m_input_ptr = input;
            output->add_ref();
            m_output_ptr = output;
        }
        xvblockbuild_t::xvblockbuild_t(base::xvheader_t* header) {
            header->add_ref();
            m_header = header;
        }

        xvblockbuild_t::~xvblockbuild_t() {
            if (m_header != nullptr) {
                m_header->release_ref();
            }
            if (m_qcert != nullptr) {
                m_qcert->release_ref();
            }
            if (m_input_ptr != nullptr) {
                m_input_ptr->release_ref();
            }
            if (m_output_ptr != nullptr) {
                m_output_ptr->release_ref();
            }
            for (auto & v : m_input_entitys) {
                v->release_ref();
            }
            m_input_entitys.clear();
            for (auto & v : m_output_entitys) {
                v->release_ref();
            }
            m_output_entitys.clear();
            if (m_input_res != nullptr) {
                m_input_res->release_ref();
            }
            if (m_output_res != nullptr) {
                m_output_res->release_ref();
            }
        }

        void xvblockbuild_t::init_qcert(const xbbuild_para_t & _para) {
            if (m_header == nullptr) {  // must init header firstly
                xassert(false);
                return;
            }
            if (m_qcert != nullptr) {
                xassert(false);
                return;
            }
            m_qcert = new base::xvqcert_t();
            auto key_curve_type = get_key_curve_type_from_account(m_header->get_account());
            m_qcert->set_crypto_key_type(key_curve_type);
            m_qcert->set_crypto_hash_type(_para.m_hash_type);
            m_qcert->set_drand(_para.m_drand_height);
            m_qcert->set_consensus_type(_para.m_consensus_type);
            m_qcert->set_consensus_threshold(_para.m_consensus_threshold);
            m_qcert->set_consensus_flag(_para.m_consensus_flag);
            m_qcert->set_crypto_sign_type(_para.m_sign_scheme);
            m_qcert->set_clock(_para.m_clock);
            m_qcert->set_viewid(_para.m_viewid);
            m_qcert->set_viewtoken(_para.m_viewtoken);
            m_qcert->set_validator(_para.m_validator);
            if (!is_xip2_empty(_para.m_auditor)) {  // optional
                m_qcert->set_auditor(_para.m_auditor);
                m_qcert->set_consensus_flag(base::enum_xconsensus_flag_audit_cert);
            }
            m_qcert->set_justify_cert_hash(_para.m_justify_cert_hash);
            m_qcert->set_parent_height(_para.m_parent_height);
        }

        void xvblockbuild_t::init_header(const xbbuild_para_t & _para) {
            if (m_header != nullptr) {
                xassert(false);
                return;
            }
            m_header = new base::xvheader_t();
            m_header->set_chainid(_para.m_chainid);
            m_header->set_account(_para.m_account);
            m_header->set_height(_para.m_height);
            m_header->set_block_level(_para.m_level);
            m_header->set_weight(1);
            m_header->set_last_block_hash(_para.m_last_block_hash);
            m_header->set_block_class(_para.m_class);
            m_header->set_block_type(_para.m_type);
            m_header->set_last_full_block(_para.m_last_full_block_hash, _para.m_last_full_block_height);
            m_header->set_extra_data(_para.m_extra_data);
        }

        void xvblockbuild_t::init_header_qcert(const xbbuild_para_t & _para) {
            init_header(_para);
            init_qcert(_para);
        }

        void xvblockbuild_t::init_input(xvinput_t* input) {
            xassert(input != nullptr);
            xassert(m_input_ptr == nullptr);
            input->add_ref();
            m_input_ptr = input;
        }

        void xvblockbuild_t::init_output(xvoutput_t* output) {
            xassert(output != nullptr);
            xassert(m_output_ptr == nullptr);
            output->add_ref();
            m_output_ptr = output;
        }

        base::enum_xvchain_key_curve xvblockbuild_t::get_key_curve_type_from_account(const std::string & account) {
            base::enum_xvchain_key_curve  key_curve_type;
            base::enum_vaccount_addr_type addr_type = base::xvaccount_t::get_addrtype_from_account(account);
            switch (addr_type) {
            case base::enum_vaccount_addr_type_secp256k1_user_account:
            case base::enum_vaccount_addr_type_secp256k1_user_sub_account:
            case base::enum_vaccount_addr_type_native_contract:
            case base::enum_vaccount_addr_type_custom_contract:
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

        void xvblockbuild_t::init_input(const std::vector<base::xventity_t*> & entitys, xstrmap_t & resource_obj) {
            m_input_ptr = new base::xvinput_t(entitys, resource_obj);
        }

        void xvblockbuild_t::init_output(const std::vector<base::xventity_t*> & entitys, xstrmap_t & resource_obj) {
            m_output_ptr = new base::xvoutput_t(entitys, resource_obj);
        }

        void xvblockbuild_t::add_input_entity(base::xventity_t* entity) {
            entity->add_ref();
            m_input_entitys.push_back(entity);
        }
        void xvblockbuild_t::add_output_entity(base::xventity_t* entity) {
            entity->add_ref();
            m_output_entitys.push_back(entity);
        }
        void xvblockbuild_t::add_input_resource(const std::string & key, const std::string & value) {
            m_input_res->set(key, value);
        }
        void xvblockbuild_t::add_output_resource(const std::string & key, const std::string & value) {
            m_output_res->set(key, value);
        }

        void xvblockbuild_t::create_default_input_output() {
            if (m_input_ptr == nullptr) {
                xassert(!m_input_entitys.empty());
                m_input_ptr = new base::xvinput_t(m_input_entitys, *m_input_res);
            }
            if (m_output_ptr == nullptr) {
                xassert(!m_output_entitys.empty());
                m_output_ptr = new base::xvoutput_t(m_output_entitys, *m_output_res);
            }
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

    }//end of namespace of base
}//end of namespace top
