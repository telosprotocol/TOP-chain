// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include "xvledger/xvblock.h"
#include "xdata/xdata_common.h"
#include "xdata/xdatautil.h"
#include "xdata/xblock.h"

NS_BEG2(top, data)

struct xfullunit_block_para_t {
    void    set_account_unconfirm_sendtx_num(uint32_t num) {m_account_unconfirm_sendtx_num = num;}
    void    set_input_txs(const std::vector<xcons_transaction_ptr_t> & input_txs);
    void    set_unchange_txs(const std::vector<xcons_transaction_ptr_t> & unchange_txs);

    void    set_fullstate_bin(const std::string & fullstate) {m_fullstate_bin = fullstate;}
    void    set_binlog(const std::string & binlog) {m_property_binlog = binlog;}

    const   std::vector<xcons_transaction_ptr_t> & get_input_txs() const {return m_raw_txs;}
    const   std::vector<xcons_transaction_ptr_t> & get_unchange_txs() const {return m_unchange_txs;}
    std::vector<xcons_transaction_ptr_t> const & get_succ_txs() const;
    uint32_t get_account_unconfirm_sendtx_num() const {return m_account_unconfirm_sendtx_num;}
    const std::string &     get_property_binlog() const {return m_property_binlog;}
    const std::string &     get_fullstate_bin() const {return m_fullstate_bin;}
    
    std::string                         m_property_snapshot;
    uint64_t                            m_first_unit_height{0};
    std::string                         m_first_unit_hash;

    uint32_t                                m_account_unconfirm_sendtx_num{0};
    // input
    std::vector<xcons_transaction_ptr_t>    m_raw_txs;
    std::vector<xcons_transaction_ptr_t>    m_unchange_txs;
    std::vector<xcons_transaction_ptr_t>    m_succ_txs;
    std::string                             m_fullstate_bin;
    std::string                             m_property_binlog;
};
class xfullunit_block_t : public xblock_t {
 protected:
    enum { object_type_value = enum_xdata_type::enum_xdata_type_max - xdata_type_fullunit_block };
 public:
    xfullunit_block_t();
    xfullunit_block_t(base::xvheader_t & header, base::xvqcert_t & cert, base::xvinput_t* input, base::xvoutput_t* output);
 protected:
    virtual ~xfullunit_block_t();
 private:
    xfullunit_block_t(const xfullunit_block_t &);
    xfullunit_block_t & operator = (const xfullunit_block_t &);
 public:
    static int32_t get_object_type() {return object_type_value;}
    static xobject_t *create_object(int type);
    void *query_interface(const int32_t _enum_xobject_type_) override;
    virtual void parse_to_json(xJson::Value & root, const std::string & rpc_version) override;
};


NS_END2
