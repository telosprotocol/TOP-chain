// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>
#include "xbase/xobject_ptr.h"
#include "xbase/xbase.h"
#include "xvledger/xvblock.h"
#include "xvledger/xmerkle.hpp"

namespace top
{
    namespace base
    {
        class xvblockbuild_t;


        class xbbuild_para_t {
            friend xvblockbuild_t;
        public:
            // genesis block init
            xbbuild_para_t();
            //for genesis
            xbbuild_para_t(enum_xchain_id chainid, const std::string & account, enum_xvblock_level _level, enum_xvblock_class _class, const std::string & last_block_hash);
            //for non genesis
            xbbuild_para_t(enum_xchain_id chainid, const std::string & account, uint64_t height, enum_xvblock_class _class, enum_xvblock_level _level, enum_xvblock_type _type, const std::string & last_block_hash, const std::string & last_full_block_hash);
            xbbuild_para_t(base::xvblock_t* prev_block, enum_xvblock_class _class, enum_xvblock_type _type);

            void    set_basic_cert_para(uint64_t _clock, uint32_t _viewtoken, uint64_t _viewid, const xvip2_t & _validator);
            void    set_unit_cert_para(uint64_t _clock, uint32_t _viewtoken, uint64_t _viewid, const xvip2_t & _validator, const xvip2_t & _auditor, uint64_t _drand_height,
                                        uint64_t _parent_height, const std::string & _justify_hash);
            void    set_table_cert_para(uint64_t _clock, uint32_t _viewtoken, uint64_t _viewid, const xvip2_t & _validator, const xvip2_t & _auditor, uint64_t _drand_height,
                                        const std::string & _justify_hash);
            // some optional parameters
            void    set_extra_data(const std::string & _extra_data) {m_extra_data = _extra_data;}

        private:
            void    set_default_qcert();
            void    set_genesis_qcert();
            void    set_header_para(base::xvblock_t* prev_block, enum_xvblock_class _class, enum_xvblock_type _type);

        protected:
            // header paras
            uint32_t            m_chainid{0};
            std::string         m_account;
            uint64_t            m_height{0};
            enum_xvblock_level  m_level;
            enum_xvblock_class  m_class;
            enum_xvblock_type   m_type;
            std::string         m_last_block_hash;
            std::string         m_last_full_block_hash;
            uint64_t            m_last_full_block_height{0};

            // qcert paras
            uint64_t            m_clock{0};
            uint32_t            m_viewtoken{0};
            uint64_t            m_viewid{0};
            xvip2_t             m_validator;
            xvip2_t             m_auditor;
            uint64_t            m_drand_height{0};
            std::string         m_extra_data;
            std::string         m_justify_cert_hash;
            uint64_t            m_parent_height{0};
            enum_xconsensus_type        m_consensus_type;
            enum_xconsensus_threshold   m_consensus_threshold;
            enum_xconsensus_flag        m_consensus_flag;
            enum_xvchain_sign_scheme    m_sign_scheme;
            enum_xhash_type             m_hash_type;
        };

        class xvblockbuild_t {
        public:
            static bool calc_output_merkle_path(xvoutput_t* output, const std::string & leaf, xmerkle_path_256_t& hash_path);
            static bool calc_input_merkle_path(xvinput_t* output, const std::string & leaf, xmerkle_path_256_t& hash_path);
            static std::vector<std::string>    get_input_merkle_leafs(const std::vector<xventity_t*> & _entitys);
            static std::vector<std::string>    get_output_merkle_leafs(const std::vector<xventity_t*> & _entitys);
        public:
            xvblockbuild_t();  // genesis genesis construct
            xvblockbuild_t(base::xvheader_t* header, base::xvqcert_t* cert, base::xvinput_t* input, base::xvoutput_t* output);
            xvblockbuild_t(base::xvheader_t* header, base::xvinput_t* input, base::xvoutput_t* output);
            xvblockbuild_t(base::xvheader_t* header);
            virtual ~xvblockbuild_t();

        public:
            base::xauto_ptr<base::xvblock_t> build_new_block();
            virtual base::xauto_ptr<base::xvblock_t> create_new_block() = 0;
            void    init_qcert(const xbbuild_para_t & _para);
            void    init_header(const xbbuild_para_t & _para);

        protected:
            xvheader_t*     get_header() const {return m_header;}
            xvqcert_t*      get_qcert() const {return m_qcert;}
            xvinput_t*      get_input() const {return m_input_ptr;}
            xvoutput_t*     get_output() const {return m_output_ptr;}

        protected:
            void            init_header_qcert(const xbbuild_para_t & _para);
            bool            init_input(const std::vector<xventity_t*> & entitys, xstrmap_t* resource_obj);
            bool            init_output(const std::vector<xventity_t*> & entitys, xstrmap_t* resource_obj);
            base::enum_xvblock_level        get_block_level_from_account(const std::string & account);
            base::enum_xvblock_type         get_block_type_from_empty_block(const std::string & account);

        private:
            bool            make_input(const std::vector<xventity_t*> & entitys, xstrmap_t* resource_obj);
            bool            make_output(const std::vector<xventity_t*> & entitys, xstrmap_t* resource_obj);
            void            set_block_flags(xvblock_t* block);
            base::enum_xvchain_key_curve    get_key_curve_type_from_account(const std::string & account);
            const std::vector<base::xventity_t*> &  get_input_entitys() const {return m_input_entitys;}
            const std::vector<base::xventity_t*> &  get_output_entitys() const {return m_output_entitys;}
            base::xstrmap_t*                        get_input_res() const {return m_input_res;}
            base::xstrmap_t*                        get_output_res() const {return m_output_res;}

        private:
            xvheader_t*                     m_header{nullptr};
            xvqcert_t*                      m_qcert{nullptr};
            xvinput_t*                      m_input_ptr{nullptr};
            xvoutput_t*                     m_output_ptr{nullptr};
            std::vector<base::xventity_t*>  m_input_entitys;
            std::vector<base::xventity_t*>  m_output_entitys;
            base::xstrmap_t*                m_input_res{nullptr};
            base::xstrmap_t*                m_output_res{nullptr};
        };

    }//end of namespace of base
}//end of namespace top
