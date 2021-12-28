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
    void    set_txs(const std::vector<xlightunit_tx_info_ptr_t> & txs_info) {m_txs = txs_info;}
    void    set_extra_data(const std::string & extra_data) {m_extra_data = extra_data;}
    void    set_property_binlog(const std::string & binlog) {m_property_binlog = binlog;}
    void    set_fullstate_bin(const std::string & fullstate) {m_fullstate_bin = fullstate;}
    void    set_tgas_balance_change(const int64_t amount) {m_tgas_balance_change = amount;}
    void    set_property_hashs(const std::map<std::string, std::string> & hashs) {m_property_hashs = hashs;}

    const std::vector<xblock_ptr_t> & get_account_units() const {return m_account_units;}
    const std::vector<xlightunit_tx_info_ptr_t> & get_txs() const {return m_txs;}
    const std::string &             get_extra_data() const {return m_extra_data;}
    const std::string &             get_property_binlog() const {return m_property_binlog;}
    const std::string &             get_fullstate_bin() const {return m_fullstate_bin;}
    int64_t                         get_tgas_balance_change() const {return m_tgas_balance_change;}
    const std::map<std::string, std::string> &  get_property_hashs() const {return m_property_hashs;}

 private:
    std::vector<xblock_ptr_t>        m_account_units;
    std::string                      m_extra_data;
    std::string                      m_property_binlog;
    std::string                      m_fullstate_bin;
    int64_t                          m_tgas_balance_change{0};
    std::map<std::string, std::string> m_property_hashs;  // need set to table-action for property receipt
    std::vector<xlightunit_tx_info_ptr_t> m_txs;
};

class xtable_block_t : public xblock_t {
 protected:
    enum { object_type_value = enum_xdata_type::enum_xdata_type_max - xdata_type_table_block };
 public:
    xtable_block_t();
    xtable_block_t(base::xvheader_t & header, base::xvqcert_t & cert, base::xvinput_t* input, base::xvoutput_t* output);
    virtual ~xtable_block_t();
 private:
    xtable_block_t(const xtable_block_t &);
    xtable_block_t & operator = (const xtable_block_t &);
    void parse_to_json_v1(xJson::Value & root);
    void parse_to_json_v2(xJson::Value & root);
 public:
    static int32_t get_object_type() {return object_type_value;}
    static xobject_t *create_object(int type);
    void *query_interface(const int32_t _enum_xobject_type_) override;
    virtual void parse_to_json(xJson::Value & root, const std::string & rpc_version) override;
    virtual std::vector<base::xvaction_t> get_tx_actions() const;
    virtual std::vector<xvheader_ptr_t> get_sub_block_headers() const;

 public:  // tableblock api
    std::string     tableblock_dump() const;

 public:  // implement block common api
    uint32_t        get_txs_count() const override;
    int64_t         get_pledge_balance_change_tgas() const override;
    virtual bool    extract_sub_blocks(std::vector<xobject_ptr_t<base::xvblock_t>> & sub_blocks) override;
    virtual bool    extract_one_sub_block(uint32_t entity_id, const std::string & extend_cert, const std::string & extend_data, xobject_ptr_t<xvblock_t> & sub_block) override;
    void update_txs_by_actions(const std::vector<base::xvaction_t> & actions, std::vector<xlightunit_tx_info_ptr_t> & txs) const;
    const std::vector<xlightunit_tx_info_ptr_t> get_txs() const override;
    bool extract_sub_txs(std::vector<base::xvtxindex_ptr> & sub_txs) override;

};


NS_END2
