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
            if (!is_xip2_empty(_para.m_auditor)) {  // optional
                _qcert->set_auditor(_para.m_auditor);
                _qcert->set_consensus_flag(base::enum_xconsensus_flag_audit_cert);
            }
            _qcert->set_justify_cert_hash(_para.m_justify_cert_hash);
            _qcert->set_parent_height(_para.m_parent_height);
            set_qcert(_qcert.get());
        }

        void xvblockbuild_t::init_header(const xbbuild_para_t & _para) {
            if (get_header() != nullptr) {
                xassert(false);
                return;
            }
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
            set_header(_header.get());
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



        //----------------------------------------xvblockmaker_t-------------------------------------//
        xvblockmaker_t::xvblockmaker_t() {
            //create it first
            m_input_resource  = new xstrmap_t();
            m_output_resource = new xstrmap_t();
        }
        xvblockmaker_t::xvblockmaker_t(base::xvheader_t* header) {
            set_header(header);
            m_input_resource  = new xstrmap_t();
            m_output_resource = new xstrmap_t();
        }

        xvblockmaker_t::~xvblockmaker_t() {
            if (m_input_resource != nullptr) {
                m_input_resource->release_ref();
            }
            if (m_output_resource != nullptr) {
                m_output_resource->release_ref();
            }
        }

        bool xvblockmaker_t::set_input_entity(const std::vector<xvaction_t> & actions) {
            if (m_primary_input_entity != nullptr) {  // only allow init once
                xassert(false);
                return false;
            }
            if (get_header()->get_block_class() == base::enum_xvblock_class_nil) {
                xassert(false);
                return false;
            }

            m_primary_input_entity = new base::xvinentity_t(actions);
            return true;
        }
        bool xvblockmaker_t::set_output_entity(const std::string & state_bin, const std::string & binlog_bin) {
            if (m_primary_output_entity != nullptr) {  // only allow init once
                xassert(false);
                return false;
            }
            if (get_header()->get_block_class() == base::enum_xvblock_class_nil) {
                xassert(false);
                return false;
            }

            std::string state_hash;
            if (!state_bin.empty()) {
                state_hash = base::xcontext_t::instance().hash(state_bin, get_qcert()->get_crypto_hash_type());
            }
            std::string binlog_hash;
            if (!binlog_bin.empty()) {
                binlog_hash = base::xcontext_t::instance().hash(binlog_bin, get_qcert()->get_crypto_hash_type());
            }

            m_primary_output_entity = new base::xvoutentity_t(state_hash, binlog_hash);
            return true;
        }

        bool    xvblockmaker_t::set_input_resource(const std::string & key, const std::string & value) {
            m_input_resource->set(key, value);
            return true;
        }

        bool    xvblockmaker_t::set_output_resource(const std::string & key, const std::string & value) {
            m_output_resource->set(key, value);
            return true;
        }
        bool    xvblockmaker_t::set_output_resource_state(const std::string & value) {
            return set_output_resource(xvoutput_t::res_binlog_key_name(), value);  // TODO(jimmy) state key same with binlog key
        }
        bool    xvblockmaker_t::set_output_resource_binlog(const std::string & value) {
            return set_output_resource(xvoutput_t::res_binlog_key_name(), value);
        }

        std::vector<std::string> xvblockmaker_t::get_input_merkle_leafs(xvinput_t* input) {
            const std::vector<xvaction_t> & actions = input->get_primary_entity()->get_actions();
            if (actions.empty()) {
                return {};
            }
            std::vector<std::string> leafs;
            for (auto & it : actions) {
                std::string action_bin;
                it.serialize_to(action_bin);
                xassert(!action_bin.empty());
                leafs.push_back(action_bin);
                // TODO(jimmy) need calc hash as leaf
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

        bool    xvblockmaker_t::make_input(xvinput_t* input_obj) {
            //build action' mpt tree,and assign mpt root as input root hash
            std::vector<std::string> leafs = get_input_merkle_leafs(input_obj);
            if (!leafs.empty()) {
                std::string root_hash = build_mpt_root(leafs);
                if (root_hash.empty()) {
                    xassert(false);
                    return false;
                }
                input_obj->set_root_hash(root_hash);
            }
            set_input(input_obj);
            return true;
        }

        bool    xvblockmaker_t::build_input() {
            if (get_input() != nullptr) {  // only allow once
                return true;
            }

            if (get_header()->get_block_class() == base::enum_xvblock_class_nil  // nil block no need
                || get_input_entity() == nullptr) {  // entity must init
                xassert(false);
                return false;
            }

            // init input object
            std::vector<xventity_t*> _entities;
            _entities.emplace_back(get_input_entity());
            xauto_ptr<xvinput_t>input_obj(new xvinput_t(_entities,*get_input_resource()));
            make_input(input_obj.get());
            xdbg("xvblockmaker_t::build_input,done for block(%s),root_empty=%d",get_header()->dump().c_str(),input_obj->get_root_hash().empty());
            return true;
        }

        bool    xvblockmaker_t::make_output(xvoutput_t* output_obj) {
            // output qcert root no need set now
            set_output(output_obj);
            return true;
        }

        bool    xvblockmaker_t::build_output() {
            if (get_output() != nullptr) {  // only allow once
                return true;
            }

            if (get_header()->get_block_class() == base::enum_xvblock_class_nil  // nil block no need
                || get_output_entity() == nullptr) {  // entity must init
                xassert(false);
                return false;
            }

            // init output object
            std::vector<xventity_t*> _entities;
            _entities.emplace_back(get_output_entity());
            xauto_ptr<xvoutput_t>output_obj(new xvoutput_t(_entities,*get_output_resource()));
            make_output(output_obj.get());
            xdbg("xvblockmaker_t::build_output,done for block(%s),root_empty=%d",get_header()->dump().c_str(),output_obj->get_root_hash().empty());
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
            return _block_ptr;
        }

        //----------------------------------------xtable_unit_resource_t-------------------------------------//
        xtable_unit_resource_t::xtable_unit_resource_t(xvblock_t* _block) {
            _block->get_header()->serialize_to_string(m_unit_header);
            _block->get_input()->serialize_to_string(m_unit_input);
            _block->get_output()->serialize_to_string(m_unit_output);
            m_unit_input_resources = _block->get_input()->get_resources_data();
            m_unit_output_resources = _block->get_output()->get_resources_data();
            m_unit_justify_hash = _block->get_cert()->get_justify_cert_hash();
        }

        int32_t xtable_unit_resource_t::do_write(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            stream.write_compact_var(m_unit_header);
            stream.write_compact_var(m_unit_input);
            stream.write_compact_var(m_unit_output);
            stream.write_compact_var(m_unit_input_resources);
            stream.write_compact_var(m_unit_output_resources);
            stream.write_compact_var(m_unit_justify_hash);
            return (stream.size() - begin_size);
        }

        int32_t xtable_unit_resource_t::do_read(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            stream.read_compact_var(m_unit_header);
            stream.read_compact_var(m_unit_input);
            stream.read_compact_var(m_unit_output);
            stream.read_compact_var(m_unit_input_resources);
            stream.read_compact_var(m_unit_output_resources);
            stream.read_compact_var(m_unit_justify_hash);
            return (begin_size - stream.size());
        }

        //----------------------------------------xvtableblock_maker_t-------------------------------------//
        xvtableblock_maker_t::xvtableblock_maker_t() {

        }
        xvtableblock_maker_t::~xvtableblock_maker_t() {

        }
        bool xvtableblock_maker_t::set_batch_units(const std::vector<xobject_ptr_t<xvblock_t>> & _batch_units) {
            if (!m_batch_units.empty()) {
                xassert(false);
                return false;
            }
            m_batch_units = _batch_units;

            uint32_t count = (uint32_t)_batch_units.size();
            for (uint32_t index = 0; index < count; index++) {
                auto & _unit = _batch_units[index];

                xauto_ptr<xtable_unit_resource_t> _unit_res = new xtable_unit_resource_t(_unit.get());
                std::string _res_key = "u" + base::xstring_utl::tostring(index);
                std::string _res_value;
                _unit_res->serialize_to_string(_res_value);
                set_input_resource(_res_key, _res_value);
            }
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
        std::vector<std::string> xvtableblock_maker_t::get_table_in_merkle_leafs(const std::vector<xobject_ptr_t<xvblock_t>> & _batch_units) {
            // build merkle root. table input root = merkle(all units input root)
            std::vector<std::string> input_leafs;
            for (auto & _unit : _batch_units) {
                std::string leaf = get_table_in_merkle_leaf(_unit.get());
                if (!leaf.empty()) {
                    input_leafs.push_back(leaf);
                }
            }
            return input_leafs;
        }
        std::string xvtableblock_maker_t::get_table_in_merkle_leaf(base::xvblock_t* _unit) {
            return _unit->get_input_root_hash();
        }

        bool xvtableblock_maker_t::units_set_parent_cert(std::vector<xobject_ptr_t<xvblock_t>> & units, base::xvqcert_t* parent_cert) {
            std::vector<std::string> out_leafs = get_table_out_merkle_leafs(units);
            std::string parent_cert_bin;
            parent_cert->serialize_to_string(parent_cert_bin);
            for (auto & _unit : units) {
                if (!_unit->get_cert()->get_extend_cert().empty()) { // already set extend cert
                    xassert(false);
                    return true;
                }
                std::string _leaf = get_table_out_merkle_leaf(_unit.get());
                base::xmerkle_path_256_t path;
                bool ret = xvblockmaker_t::calc_merkle_path(out_leafs, _leaf, path);
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

        xauto_ptr<xtable_unit_resource_t> xvtableblock_maker_t::query_unit_resource(const base::xvblock_t* _tableblock, uint32_t index) {
            std::string _res_key = "u" + base::xstring_utl::tostring(index);
            std::string _res_value = _tableblock->get_input()->query_resource(_res_key);
            xassert(!_res_value.empty());

            xauto_ptr<xtable_unit_resource_t> _unit_res = new xtable_unit_resource_t();
            _unit_res->serialize_from_string(_res_value);
            return _unit_res;
        }

        bool xvtableblock_maker_t::build_input() {
            if (get_input() != nullptr) {  // only allow once
                xassert(false);
                return false;
            }

            if (get_header()->get_block_class() == base::enum_xvblock_class_nil  // nil block no need
                || get_input_entity() == nullptr
                || get_batch_units().empty()) {  // entity must init
                xassert(false);
                return false;
            }

            // #1 init input object with primary entity and batch units entity, with unit resource
            std::vector<xventity_t*> all_input_entities;
            all_input_entities.push_back(get_input_entity());
            xauto_ptr<xvinput_t>input_obj(new xvinput_t(all_input_entities, *get_input_resource()));
            set_input(input_obj.get());

            // #2 build merkle root. table input root = merkle(all units input root)
            std::vector<std::string> input_leafs = get_table_in_merkle_leafs(get_batch_units());
            if (!input_leafs.empty()) {
                std::string root_hash = build_mpt_root(input_leafs);
                if (root_hash.empty()) {
                    xassert(false);
                    return false;
                }
                input_obj->set_root_hash(root_hash);
            }
            xdbg("xvtableblock_maker_t::build_input,done for block(%s),units_count=%zu,root_empty=%d",
                get_header()->dump().c_str(),get_batch_units().size(),input_obj->get_root_hash().empty());
            return true;
        }

        bool xvtableblock_maker_t::build_output() {
            if (get_output() != nullptr) {  // only allow once
                xassert(false);
                return false;
            }

            if (get_header()->get_block_class() == base::enum_xvblock_class_nil  // nil block no need
                || get_output_entity() == nullptr
                || get_batch_units().empty()) {  // entity must init
                xassert(false);
                return false;
            }

            // #1 init input object with primary entity and batch units entity, with unit resource
            std::vector<xventity_t*> all_output_entities;
            all_output_entities.push_back(get_output_entity());
            xauto_ptr<xvoutput_t>output_obj(new xvoutput_t(all_output_entities, *get_output_resource()));
            set_output(output_obj.get());

            // #2 build merkle root. table output root = merkle(all units sign hash)
            std::vector<std::string> output_leafs = get_table_out_merkle_leafs(get_batch_units());
            std::string root_hash = build_mpt_root(output_leafs);
            if (root_hash.empty()) {
                xassert(false);
                return false;
            }
            output_obj->set_root_hash(root_hash);
            xdbg("xvtableblock_maker_t::build_output,done for block(%s),units_count=%zu,root_empty=%d",
                get_header()->dump().c_str(),get_batch_units().size(),output_obj->get_root_hash().empty());
            return true;
        }

    }//end of namespace of base
}//end of namespace top
