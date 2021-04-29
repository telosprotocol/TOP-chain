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
#include "xdata/xblockchain.h"
#include "xdata/xlightunit_info.h"

NS_BEG2(top, data)

class xtransaction_result_t {
 public:
    int64_t get_balance_change() const {return m_balance_change;}
    int64_t get_pledge_balance_change_disk() const {return m_pledge_balance_change.disk;}
    int64_t get_pledge_balance_change_tgas() const {return m_pledge_balance_change.tgas;}
    int64_t get_pledge_balance_change_vote() const {return m_pledge_balance_change.vote;}
    int64_t get_lock_change_balance() const {return m_lock_balance_change;}
    int64_t get_lock_change_tgas() const {return m_lock_tgas_change;}
    int64_t get_unvote_num_change() const {return m_unvote_num_change;}
    const   std::vector<xcons_transaction_ptr_t> & get_contract_create_txs() const {return m_contract_txs;}
    const   std::map<std::string, std::string> & get_props_hash() const {return m_props;}
    const   xnative_property_t & get_native_property() const {return m_native_property;}
    const xproperty_log_ptr_t & get_property_log() const {return m_prop_log;}

    std::string dump() const;

 public:
    int64_t                             m_balance_change{};
    xpledge_balance_change              m_pledge_balance_change;
    int64_t                             m_lock_balance_change{0};
    int64_t                             m_lock_tgas_change{0};
    int64_t                             m_unvote_num_change{0};
    std::map<std::string, std::string>  m_props;
    std::vector<xcons_transaction_ptr_t> m_contract_txs{};
    data::xnative_property_t            m_native_property;
    xproperty_log_ptr_t                 m_prop_log{nullptr};
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

    void    set_balance_change(int64_t change) {m_tx_result.m_balance_change = change;}
    void    set_pledge_balance_change(int64_t tgas_change, int64_t disk_change, int64_t vote_change);
    void    set_lock_change(int64_t deposit_change, int64_t tgas_change);
    void    set_unvote_num_change(int64_t change) {m_tx_result.m_unvote_num_change = change;}
    void    set_propertys_change(const std::map<std::string, std::string> & props) {m_tx_result.m_props = props;}
    void    set_contract_txs(const std::vector<xcons_transaction_ptr_t> & contract_txs);
    void    set_native_property(const xnative_property_t & native) {return m_tx_result.m_native_property.copy_modify_objs_only(native);}
    void    set_property_log(const xproperty_log_ptr_t & binlog);

 public:
    const   std::vector<xcons_transaction_ptr_t> & get_input_txs() const {return m_raw_txs;}
    int64_t get_balance_change() const {return m_tx_result.m_balance_change;}
    int64_t get_pledge_balance_change_disk() const {return m_tx_result.m_pledge_balance_change.disk;}
    int64_t get_pledge_balance_change_tgas() const {return m_tx_result.m_pledge_balance_change.tgas;}
    int64_t get_pledge_balance_change_vote() const {return m_tx_result.m_pledge_balance_change.vote;}
    int64_t get_lock_change_balance() const {return m_tx_result.m_lock_balance_change;}
    //int64_t get_lock_change_balance() const {return m_tx_result.m_lock_balance_change;}
    int64_t get_lock_change_tgas() const {return m_tx_result.m_lock_tgas_change;}
    int64_t get_unvote_num_change() const {return m_tx_result.m_unvote_num_change;}
    const   std::vector<xcons_transaction_ptr_t> & get_contract_create_txs() const {return m_tx_result.m_contract_txs;}
    const   std::map<std::string, std::string> & get_props_hash() const {return m_tx_result.m_props;}
    const   xnative_property_t & get_native_property() const {return m_tx_result.m_native_property;}
    const xproperty_log_ptr_t & get_property_log() const {return m_tx_result.m_prop_log;}
    uint32_t get_account_unconfirm_sendtx_num() const {return m_account_unconfirm_sendtx_num;}

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
    uint16_t    get_unconfirm_sendtx_num() const {return m_unconfirm_sendtx_num;}
    bool        is_prev_sendtx_confirmed() const {return m_prev_sendtx_confirmed_flag == 1 ? true : false;}
    int64_t     get_balance_change() const {return m_balance_change;}
    int64_t     get_pledge_balance_change_disk() const {return m_pledge_balance_change.disk;}
    int64_t     get_pledge_balance_change_tgas() const {return m_pledge_balance_change.tgas;}
    int64_t     get_pledge_balance_change_vote() const {return m_pledge_balance_change.vote;}
    int64_t     get_lock_change_balance() const {return m_lock_balance_change;}
    int64_t     get_lock_change_tgas() const {return m_lock_tgas_change;}
    int64_t     get_unvote_num_change() const {return m_unvote_num_change;}
    std::string get_property_hash(const std::string & prop_name) const;
    const       std::map<std::string, std::string> & get_property_hash_map() const {return m_property_hash;}
    const       xnative_property_t & get_native_property() const {return m_native_property;}
    const xaccount_binlog_ptr_t & get_property_log() const {return m_binlog;}

 private:
    int64_t                             m_balance_change{0};
    xpledge_balance_change              m_pledge_balance_change;
    int64_t                             m_lock_balance_change{0};
    int64_t                             m_lock_tgas_change{0};
    int64_t                             m_unvote_num_change{0};
    data::xnative_property_t            m_native_property;

    std::map<std::string, std::string>  m_property_hash;
    uint16_t                            m_unconfirm_sendtx_num{0};
    uint8_t                             m_prev_sendtx_confirmed_flag{0};
    xaccount_binlog_ptr_t               m_binlog{nullptr};
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
    static base::xvblock_t* create_lightunit(const std::string & account,
                                                uint64_t height,
                                                std::string last_block_hash,
                                                std::string justify_block_hash,
                                                uint64_t viewid,
                                                uint64_t clock,
                                                const std::string & last_full_block_hash,
                                                uint64_t last_full_block_height,
                                                const xlightunit_block_para_t & para);
    static xblockbody_para_t get_blockbody_from_para(const xlightunit_block_para_t & para);
 public:
    static base::xvblock_t* create_genesis_lightunit(const std::string & account,
                                                     const xtransaction_ptr_t & genesis_tx,
                                                     const xtransaction_result_t & result);
    static base::xvblock_t* create_next_lightunit(const xlightunit_block_para_t & para, base::xvblock_t* prev_block);
    static base::xvblock_t* create_next_lightunit(const xlightunit_block_para_t & para, xblockchain2_t* chain);
    static base::xvblock_t* create_next_lightunit(const xinput_ptr_t & input, const xoutput_ptr_t & output, base::xvblock_t* prev_block);

    xlightunit_block_t(base::xvheader_t & header, xblockcert_t & cert, const xinput_ptr_t & input, const xoutput_ptr_t & output);

 protected:
    xlightunit_block_t(base::xvheader_t & header, xblockcert_t & cert);
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
    const std::map<std::string, std::string> &      get_property_hash_map() const override {return get_tx_output_resource()->get_property_hash_map();}
    std::string                 get_property_hash(const std::string & prop_name) const override {return get_tx_output_resource()->get_property_hash(prop_name);}
    const xnative_property_t &  get_native_property() const override {return get_tx_output_resource()->get_native_property();}
    xaccount_binlog_t*          get_property_log() const override {return get_tx_output_resource()->get_property_log().get();}
    int64_t                     get_pledge_balance_change_tgas() const override {return get_tx_output_resource()->get_pledge_balance_change_tgas();}
    uint32_t                    get_txs_count() const override {return (uint32_t)get_input()->get_entitys().size();}
    int64_t                     get_balance_change() const override { return get_tx_output_resource()->get_balance_change();}
    int64_t                     get_burn_balance_change() const override;
    int64_t                     get_pledge_disk_change() const {return get_tx_output_resource()->get_pledge_balance_change_disk();}
    int64_t                     get_pledge_vote_change() const {return get_tx_output_resource()->get_pledge_balance_change_vote();}
    int64_t                     get_lock_tgas_change() const {return get_tx_output_resource()->get_lock_change_tgas();}
    int64_t                     get_lock_balance_change() const {return get_tx_output_resource()->get_lock_change_balance();}
    int64_t                     get_unvote_num_change() const {return get_tx_output_resource()->get_unvote_num_change();}
    uint16_t                    get_unconfirm_sendtx_num() const override {return get_tx_output_resource()->get_unconfirm_sendtx_num();}
    bool                        get_send_trans_info(uint64_t & latest_number, uint256_t & lastest_hash) const;
    bool                        get_recv_trans_info(uint64_t & total_number, uint256_t & latest_hash) const;
    bool                        is_prev_sendtx_confirmed() const override { return get_tx_output_resource()->is_prev_sendtx_confirmed();}

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
