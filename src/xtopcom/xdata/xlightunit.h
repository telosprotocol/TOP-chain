// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>
#include <map>
#include "xvledger/xdataobj_base.hpp"
#include "xdata/xblock.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xlightunit_info.h"

NS_BEG2(top, data)

class xtransaction_result_t {
 public:
    const   std::vector<xcons_transaction_ptr_t> & get_contract_create_txs() const {return m_contract_txs;}
    const std::string &     get_property_binlog() const {return m_property_binlog;}

    std::string dump() const;

 public:
    std::vector<xcons_transaction_ptr_t> m_contract_txs{};
    std::string                         m_property_binlog;
};

class xlightunit_block_para_t {
 public:
    xlightunit_block_para_t() = default;
    ~xlightunit_block_para_t();

 public:
    void    set_transaction_result(const xtransaction_result_t & result);
    void    set_account_unconfirm_sendtx_num(uint32_t num) {m_account_unconfirm_sendtx_num = num;}

    void    set_one_input_tx(const xtransaction_ptr_t & tx);
    void    set_one_input_tx(const xcons_transaction_ptr_t & input_tx);
    void    set_input_txs(const std::vector<xcons_transaction_ptr_t> & input_txs);

    void    set_contract_txs(const std::vector<xcons_transaction_ptr_t> & contract_txs);

 public:
    const   std::vector<xcons_transaction_ptr_t> & get_input_txs() const {return m_raw_txs;}
    const   std::vector<xcons_transaction_ptr_t> & get_contract_create_txs() const {return m_tx_result.m_contract_txs;}
    uint32_t get_account_unconfirm_sendtx_num() const {return m_account_unconfirm_sendtx_num;}
    const std::string &     get_property_binlog() const {return m_tx_result.get_property_binlog();}

 private:
    uint32_t                                m_account_unconfirm_sendtx_num{0};
    // input
    std::vector<xcons_transaction_ptr_t>    m_raw_txs;
    // output
    xtransaction_result_t                   m_tx_result;
};

class xlightunit_output_resource_t : public xbase_dataunit_t<xlightunit_output_resource_t, xdata_type_lightunit_output_resource> {
 public:
    static  const std::string   name() { return std::string("o0");}  // common output resource version#0
    std::string         get_obj_name() const override {return name();}

 public:
    xlightunit_output_resource_t() = default;
    explicit xlightunit_output_resource_t(const xlightunit_block_para_t & para);
 protected:
    virtual ~xlightunit_output_resource_t() {}

    int32_t do_write(base::xstream_t & stream) override;
    int32_t do_read(base::xstream_t & stream) override;

 public:
    uint32_t    get_unconfirm_sendtx_num() const {return m_unconfirm_sendtx_num;}

 private:
    uint32_t                            m_unconfirm_sendtx_num{0};
};
using xlightunit_output_resource_ptr_t = xobject_ptr_t<xlightunit_output_resource_t>;

class xlightunit_body_t {
 public:
    xlightunit_body_t() = default;
    void add_tx_info(const xlightunit_tx_info_ptr_t & txinfo) {
        m_tx_infos.push_back(txinfo);
    }
    void add_lightunit_output_resource(const xlightunit_output_resource_ptr_t & txoutput_resource) {
        m_tx_output_resource = txoutput_resource;
    }

    const std::vector<xlightunit_tx_info_ptr_t> & get_txs() const {return m_tx_infos;}
    bool is_empty() const {return m_tx_infos.empty();}
    const xlightunit_output_resource_ptr_t & get_txout_resource() const {return m_tx_output_resource;}

 private:
    std::vector<xlightunit_tx_info_ptr_t>   m_tx_infos;
    xlightunit_output_resource_ptr_t    m_tx_output_resource{nullptr};
};

class xlightunit_block_t : public xblock_t {
 protected:
    enum { object_type_value = enum_xdata_type::enum_xdata_type_max - xdata_type_lightunit_block };
 public:
    xlightunit_block_t(base::xvheader_t & header, base::xvqcert_t & cert, base::xvinput_t* input, base::xvoutput_t* output);

 protected:
    virtual ~xlightunit_block_t();
 private:
    xlightunit_block_t();
    xlightunit_block_t(const xlightunit_block_t &);
    xlightunit_block_t & operator = (const xlightunit_block_t &);
 public:
    static int32_t get_object_type() {return object_type_value;}
    static xobject_t *create_object(int type);
    void *query_interface(const int32_t _enum_xobject_type_) override;

 public:  // lightunit special apis
    virtual     std::string     dump_body() const;
    void                        create_txreceipts(std::vector<xcons_transaction_ptr_t> & sendtx_receipts, std::vector<xcons_transaction_ptr_t> & recvtx_receipts);
    void                        create_send_txreceipts(std::vector<xcons_transaction_ptr_t> & sendtx_receipts);
    xcons_transaction_ptr_t     create_one_txreceipt(const xtransaction_t* tx);

 public:  // override base block api
    bool                        extract_sub_txs(std::vector<base::xvtxindex_ptr> & sub_txs) override;
    const std::vector<xlightunit_tx_info_ptr_t> &   get_txs() const override;
    uint32_t                    get_txs_count() const override {return (uint32_t)get_input()->get_entitys().size();}
    uint16_t                    get_unconfirm_sendtx_num() const override {return get_tx_output_resource()->get_unconfirm_sendtx_num();}

 private:
    const xlightunit_output_resource_ptr_t &     get_tx_output_resource() const;
    void                        try_load_body() const;
    void                        load_body() const;
    const xlightunit_body_t &   get_lightunit_body() const;
    xcons_transaction_ptr_t     create_txreceipt(const xtransaction_t* tx, xlightunit_output_entity_t* txinfo);

 private:
    mutable std::once_flag      m_once_load_flag;  // cache body delay init
    mutable xlightunit_body_t   m_cache_body;
};

NS_END2
