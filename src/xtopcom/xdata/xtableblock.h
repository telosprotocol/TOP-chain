// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include <vector>
#include "xvledger/xdataobj_base.hpp"
#include "xvledger/xvblock.h"
#include "xvledger/xreceiptid.h"
#include "xdata/xdata_common.h"
#include "xdata/xdatautil.h"
#include "xdata/xblock.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xlightunit.h"
#include "xdata/xfullunit.h"
#include "xvledger/xvblockbuild.h"

NS_BEG2(top, data)

class xtableblock_input_entity_t final : public xventity_face_t<xtableblock_input_entity_t, xdata_type_tableblock_input_entity> {
 public:
    xtableblock_input_entity_t() = default;
    explicit xtableblock_input_entity_t(const xblock_t* unit);
 protected:
    ~xtableblock_input_entity_t() = default;
    int32_t do_write(base::xstream_t & stream) override;
    int32_t do_read(base::xstream_t & stream) override;
 private:
    xtableblock_input_entity_t & operator = (const xtableblock_input_entity_t & other);
 public:
    virtual const std::string query_value(const std::string & key) override {return std::string();}
 public:
    const std::string &     get_unit_account() const {return m_unit_account;}
    uint64_t                get_unit_height() const {return m_unit_height;}
    const std::string &     get_unit_header_hash() const {return m_unit_header_hash;}

 private:
    std::string         m_unit_account;
    uint64_t            m_unit_height{0};
    std::string         m_unit_header_hash;
};

class xtableblock_output_entity_t final: public xventity_face_t<xtableblock_output_entity_t, xdata_type_tableblock_output_entity> {
 public:
    xtableblock_output_entity_t() = default;
    explicit xtableblock_output_entity_t(const xblock_t* unit);

 protected:
    ~xtableblock_output_entity_t();
    int32_t do_write(base::xstream_t & stream) override;
    int32_t do_read(base::xstream_t & stream) override;

 public:
    virtual const std::string query_value(const std::string & key) override {return get_merkle_leaf();}

 public:
    const std::string   get_merkle_leaf() const;
    const std::string & get_unit_sign_hash() const {return m_unit_sign_hash;}

 private:
    std::string     m_unit_sign_hash;
};

class xtable_block_para_t {
 public:
    xtable_block_para_t() = default;
    ~xtable_block_para_t() = default;
    void    add_unit(base::xvblock_t * unit) {
        unit->add_ref();
        xblock_t* block_ptr = (xblock_t*)unit;
        xblock_ptr_t auto_block_ptr;
        auto_block_ptr.attach(block_ptr);
        m_account_units.push_back(auto_block_ptr);
    }
    void    set_batch_units(const std::vector<xblock_ptr_t> & batch_units) {m_account_units = batch_units;}
    void    set_extra_data(const std::string & extra_data) {m_extra_data = extra_data;}
    void    set_property_binlog(const std::string & binlog) {m_property_binlog = binlog;}

    const std::vector<xblock_ptr_t> & get_account_units() const {return m_account_units;}
    const std::string &             get_extra_data() const {return m_extra_data;}
    const std::string &             get_property_binlog() const {return m_property_binlog;}

 private:
    std::vector<xblock_ptr_t>        m_account_units;
    std::string                      m_extra_data;
    std::string                      m_property_binlog;
};

class xtable_block_t : public xblock_t {
 protected:
    enum { object_type_value = enum_xdata_type::enum_xdata_type_max - xdata_type_table_block };
    static base::xvblock_t* create_tableblock(const std::string & account,
                                                uint64_t height,
                                                std::string last_block_hash,
                                                std::string justify_block_hash,
                                                uint64_t viewid,
                                                uint64_t clock,
                                                const std::string & last_full_block_hash,
                                                uint64_t last_full_block_height,
                                                const xtable_block_para_t & para);
    static xblockbody_para_t get_blockbody_from_para(const xtable_block_para_t & para);
    static xblock_ptr_t create_whole_unit(const std::string & header,
                                                const std::string & input,
                                                const std::string & input_res,
                                                const std::string & output,
                                                const std::string & output_res,
                                                const base::xbbuild_para_t & build_para);
 public:
    static base::xvblock_t* create_next_tableblock(const xtable_block_para_t & para, base::xvblock_t* prev_block);
 public:
    xtable_block_t();
    xtable_block_t(base::xvheader_t & header, xblockcert_t & cert, const xinput_ptr_t & input, const xoutput_ptr_t & output);
    xtable_block_t(base::xvheader_t & header, base::xvqcert_t & cert, base::xvinput_t* input, base::xvoutput_t* output);
    virtual ~xtable_block_t();
 private:
    xtable_block_t(const xtable_block_t &);
    xtable_block_t & operator = (const xtable_block_t &);
 public:
    static int32_t get_object_type() {return object_type_value;}
    static xobject_t *create_object(int type);
    void *query_interface(const int32_t _enum_xobject_type_) override;

 protected:
    void                    unpack_proposal_units(std::vector<xblock_ptr_t> & units) const;
    const std::vector<xblock_ptr_t> &      unpack_and_get_units(bool need_parent_cert) const;

 public:  // tableblock api
    std::string     tableblock_dump() const;
    void            cache_units_set_parent_cert(std::vector<xblock_ptr_t> & units, base::xvqcert_t* parent_cert) const;  // should set unit parent cert when table-block consensused
    void            create_txreceipts(std::vector<xcons_transaction_ptr_t> & sendtx_receipts, std::vector<xcons_transaction_ptr_t> & recvtx_receipts);

 public:  // implement block common api
    uint32_t        get_txs_count() const override;
    int64_t         get_pledge_balance_change_tgas() const override;
    const std::vector<xblock_ptr_t> & get_tableblock_units(bool need_parent_cert = false) const override;
    std::map<std::string, xaccount_index_t> get_units_index() const override;
    virtual bool    extract_sub_blocks(std::vector<xobject_ptr_t<base::xvblock_t>> & sub_blocks) override;
    std::string     get_property_binlog() const override {return get_output()->query_resource("bl");}  // TODO(jimmy) binlog

 private:
    xblock_ptr_t    recreate_unit_from_unit_input_output_resource(uint16_t index) const;
 private:
    mutable std::once_flag              m_once_unpack_flag;
    mutable std::once_flag              m_once_set_parent_cert_flag;
    mutable std::vector<xblock_ptr_t>   m_cache_units;  // delay load
};


NS_END2
