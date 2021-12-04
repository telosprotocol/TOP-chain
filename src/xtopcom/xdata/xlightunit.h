// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>
#include <map>
#include "xvledger/xdataobj_base.hpp"
#include "xvledger/xvtxindex.h"
#include "xdata/xblock.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xlightunit_info.h"

NS_BEG2(top, data)

class xtransaction_result_t {
 public:
    const std::string &     get_property_binlog() const {return m_property_binlog;}
 public:
    std::vector<xcons_transaction_ptr_t> m_contract_txs{};  // no use now
    std::string                         m_property_binlog;
    std::string                         m_full_state;
};

class xlightunit_block_para_t {
 public:
    xlightunit_block_para_t() = default;
    ~xlightunit_block_para_t();

 public:
    void    set_account_unconfirm_sendtx_num(uint32_t num) {m_account_unconfirm_sendtx_num = num;}

    void    set_one_input_tx(const xtransaction_ptr_t & tx);
    void    set_one_input_tx(const xcons_transaction_ptr_t & input_tx);
    void    set_input_txs(const std::vector<xcons_transaction_ptr_t> & input_txs);
    void    set_unchange_txs(const std::vector<xcons_transaction_ptr_t> & unchange_txs);

    void    set_fullstate_bin(const std::string & fullstate) {m_fullstate_bin = fullstate;}
    void    set_binlog(const std::string & binlog) {m_property_binlog = binlog;}

 public:
    const   std::vector<xcons_transaction_ptr_t> & get_input_txs() const {return m_raw_txs;}
    const   std::vector<xcons_transaction_ptr_t> & get_unchange_txs() const {return m_unchange_txs;}
    std::vector<xcons_transaction_ptr_t> const & get_succ_txs() const;
    uint32_t get_account_unconfirm_sendtx_num() const {return m_account_unconfirm_sendtx_num;}
    const std::string &     get_property_binlog() const {return m_property_binlog;}
    const std::string &     get_fullstate_bin() const {return m_fullstate_bin;}

 private:
    uint32_t                                m_account_unconfirm_sendtx_num{0};
    // input
    std::vector<xcons_transaction_ptr_t>    m_raw_txs;
    std::vector<xcons_transaction_ptr_t>    m_unchange_txs;
    std::vector<xcons_transaction_ptr_t>    m_succ_txs;
    std::string                             m_fullstate_bin;
    std::string                             m_property_binlog;
};

class xlightunit_body_t {
 public:
    xlightunit_body_t() = default;
    void add_tx_info(const xlightunit_tx_info_ptr_t & txinfo) {
        m_tx_infos.push_back(txinfo);
    }
    const std::vector<xlightunit_tx_info_ptr_t> & get_txs() const {return m_tx_infos;}
    bool is_empty() const {return m_tx_infos.empty();}

 private:
    std::vector<xlightunit_tx_info_ptr_t>   m_tx_infos;
};

class xlightunit_block_t : public xblock_t {
 public:
    static  const std::string   unconfirm_tx_num_name() { return std::string("i0");}  // input resource #0
 protected:
    enum { object_type_value = enum_xdata_type::enum_xdata_type_max - xdata_type_lightunit_block };
 public:
    xlightunit_block_t();
    xlightunit_block_t(base::xvheader_t & header, base::xvqcert_t & cert, base::xvinput_t* input, base::xvoutput_t* output);

 protected:
    virtual ~xlightunit_block_t();
 private:
    xlightunit_block_t(const xlightunit_block_t &);
    xlightunit_block_t & operator = (const xlightunit_block_t &);
    void parse_to_json_v1(xJson::Value & root);
    void parse_to_json_v2(xJson::Value & root);
 public:
    static int32_t get_object_type() {return object_type_value;}
    static xobject_t *create_object(int type);
    void *query_interface(const int32_t _enum_xobject_type_) override;
    virtual void parse_to_json(xJson::Value & root, const std::string & rpc_version) override;

 public:  // lightunit special apis
    virtual     std::string     dump_body() const;
 public:  // override base block api
    bool                        extract_sub_txs(std::vector<base::xvtxindex_ptr> & sub_txs) override;
    const std::vector<xlightunit_tx_info_ptr_t> get_txs() const override;
    uint32_t                    get_txs_count() const override {return (uint32_t)get_input()->get_entitys().size();}
    uint32_t                    get_unconfirm_sendtx_num() const override;

 private:
    void                        try_load_body() const;
    void                        load_body() const;
    const xlightunit_body_t &   get_lightunit_body() const;

 private:
    mutable std::once_flag      m_once_load_flag;  // cache body delay init
    mutable xlightunit_body_t   m_cache_body;
};

NS_END2
