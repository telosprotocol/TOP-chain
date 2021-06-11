// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>
#include "xbase/xobject_ptr.h"
#include "xbase/xbase.h"
#include "xvledger/xdataobj_base.hpp"
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
            xvblockbuild_t();  // genesis genesis construct
            xvblockbuild_t(base::xvheader_t* header, base::xvqcert_t* cert);
            xvblockbuild_t(base::xvheader_t* header);
            virtual ~xvblockbuild_t();

        public:
            virtual base::xauto_ptr<base::xvblock_t> create_new_block() = 0;
            void    init_qcert(const xbbuild_para_t & _para);
            void    init_header(const xbbuild_para_t & _para);

        public:
            xvheader_t*     get_header() const {return m_header_ptr;}
            xvqcert_t*      get_qcert() const {return m_qcert_ptr;}
            xvinput_t*      get_input() const {return m_input_ptr;}
            xvoutput_t*     get_output() const {return m_output_ptr;}
            xvblock_t*      get_block() const {return m_block_ptr;}

        protected:
            void            init_header_qcert(const xbbuild_para_t & _para);
            void            set_header(xvheader_t* _header);
            void            set_qcert(xvqcert_t* _qcert);
            void            set_input(xvinput_t* _input);
            void            set_output(xvoutput_t* _output);
            void            set_block(xvblock_t* _block);
            std::string     build_mpt_root(const std::vector<std::string> & elements);
            void            set_block_flags(xvblock_t* block);

            base::enum_xvblock_level        get_block_level_from_account(const std::string & account);
            base::enum_xvblock_type         get_block_type_from_empty_block(const std::string & account);
        private:

            base::enum_xvchain_key_curve    get_key_curve_type_from_account(const std::string & account);

        private:
            xvblock_t*                      m_block_ptr{nullptr};     //new block
            xvheader_t*                     m_header_ptr{nullptr};
            xvqcert_t*                      m_qcert_ptr{nullptr};
            xvinput_t*                      m_input_ptr{nullptr};
            xvoutput_t*                     m_output_ptr{nullptr};
        };


        class xvblockmaker_t : public xvblockbuild_t {
        public:
            static bool calc_merkle_path(const std::vector<std::string> & leafs, const std::string & leaf, xmerkle_path_256_t& hash_path);
            static bool calc_input_merkle_path(xvinput_t* input, const std::string & leaf, xmerkle_path_256_t& hash_path);
            static std::vector<std::string>    get_input_merkle_leafs(xvinput_t* input);
        public:
            xvblockmaker_t();
            xvblockmaker_t(base::xvheader_t* header);
            virtual ~xvblockmaker_t();
        public:
            bool    set_input_entity(const std::vector<xvaction_t> & actions);
            bool    set_output_entity(const std::string & state_bin, const std::string & binlog_bin);
            bool    set_input_resource(const std::string & key, const std::string & value);
            bool    set_output_resource_state(const std::string & value);
            bool    set_output_resource_binlog(const std::string & value);
            bool    make_input(xvinput_t* input_obj);
            bool    make_output(xvoutput_t* output_obj);

            virtual base::xauto_ptr<base::xvblock_t> build_new_block();
            virtual base::xauto_ptr<base::xvblock_t> create_new_block() = 0;
        protected:
            virtual bool    build_input();
            virtual bool    build_output();

        protected://internal use only
            inline xstrmap_t*           get_input_resource()  const {return m_input_resource;}
            inline xstrmap_t*           get_output_resource() const {return m_output_resource;}
            inline xvinentity_t*        get_input_entity()    const {return m_primary_input_entity;}
            inline xvoutentity_t*       get_output_entity()   const {return m_primary_output_entity;}

        private:
            bool    set_output_resource(const std::string & key, const std::string & value);
        private:
            xstrmap_t*                  m_input_resource{nullptr}; //resource to hold input 'big data
            xstrmap_t*                  m_output_resource{nullptr};//resource to hold output ' big data

            xvinentity_t*               m_primary_input_entity{nullptr}; //#0 is primary input entity
            xvoutentity_t*              m_primary_output_entity{nullptr};//#0 is primary output entity
        };


        class xtable_unit_resource_t : public xbase_dataunit_t<xtable_unit_resource_t, xdata_type_tableblock_unit_res> {
        public:
            xtable_unit_resource_t() = default;
            xtable_unit_resource_t(xvblock_t* _block);
        protected:
            int32_t do_write(base::xstream_t & stream) override;
            int32_t do_read(base::xstream_t & stream) override;

        public:
            const std::string & get_unit_header() const {return m_unit_header;}
            const std::string & get_unit_input() const {return m_unit_input;}
            const std::string & get_unit_input_resources() const {return m_unit_input_resources;}
            const std::string & get_unit_output() const {return m_unit_output;}
            const std::string & get_unit_output_resources() const {return m_unit_output_resources;}
            const std::string & get_unit_justify_hash() const {return m_unit_justify_hash;}
        private:
            std::string     m_unit_header;
            std::string     m_unit_input;
            std::string     m_unit_output;
            std::string     m_unit_input_resources;
            std::string     m_unit_output_resources;
            std::string     m_unit_justify_hash;
        };

        // xvtablemaker is just for batch units maker
        class xvtableblock_maker_t : public xvblockmaker_t {
        public:
            static std::vector<std::string> get_table_out_merkle_leafs(const std::vector<xobject_ptr_t<xvblock_t>> & _batch_units);
            static std::string              get_table_out_merkle_leaf(base::xvblock_t* _unit);
            static std::vector<std::string> get_table_in_merkle_leafs(const std::vector<xobject_ptr_t<xvblock_t>> & _batch_units);
            static std::string              get_table_in_merkle_leaf(base::xvblock_t* _unit);
            static bool                     units_set_parent_cert(std::vector<xobject_ptr_t<xvblock_t>> & units, base::xvqcert_t* parent_cert);
            static xauto_ptr<xtable_unit_resource_t> query_unit_resource(const base::xvblock_t* _tableblock, uint32_t index);
        public:
            xvtableblock_maker_t();
            virtual ~xvtableblock_maker_t();
        public:
            bool    set_batch_units(const std::vector<xobject_ptr_t<xvblock_t>> & _batch_units);

            virtual bool    build_input() override;
            virtual bool    build_output() override;

        protected:
            const std::vector<xobject_ptr_t<xvblock_t>> & get_batch_units() const {return m_batch_units;}
        private:
            std::vector<xobject_ptr_t<xvblock_t>>   m_batch_units;
        };

    }//end of namespace of base
}//end of namespace top
