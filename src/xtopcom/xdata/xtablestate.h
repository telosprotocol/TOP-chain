// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbase/xobject_ptr.h"
#include "xvledger/xdataobj_base.hpp"
#include "xvledger/xaccountindex.h"
#include "xvledger/xvboffdata.h"
#include "xvledger/xreceiptid.h"

NS_BEG2(top, data)

class xtablestate_t : public base::xdataunit_t {
 protected:
    static XINLINE_CONSTEXPR char const * XTABLE_PROPERTY_ACCOUNTINDEX          = "0";
    static XINLINE_CONSTEXPR char const * XTABLE_PROPERTY_RECEIPTID             = "1";
 public:
    xtablestate_t();
    xtablestate_t(const std::string & full_data, uint64_t full_height, const std::string & binlog_data, uint64_t binlog_height);
    xtablestate_t(const xobject_ptr_t<base::xvboffdata_t> & full_data, uint64_t full_height, const std::string & binlog_data, uint64_t binlog_height);
 protected:
    ~xtablestate_t() {}
    int32_t do_write(base::xstream_t & stream) override;
    int32_t do_read(base::xstream_t & stream) override;

 private://not implement those private construction
    xtablestate_t(xtablestate_t &&);
    xtablestate_t(const xtablestate_t &);
    xtablestate_t & operator = (const xtablestate_t & other);

 public:  // apis for object change
    std::string serialize_to_full_data_string() const;
    std::string serialize_to_binlog_data_string() const;
    xobject_ptr_t<base::xvboffdata_t>   get_block_full_data() const;

    void        merge_new_full();
    std::string build_root_hash();
    uint64_t    get_full_height() const {return m_full_height;}
    uint64_t    get_binlog_height() const {return m_binlog_height;}
    uint64_t    get_height() const {return m_binlog_height;}

    bool        execute_block(base::xvblock_t* block);
    xobject_ptr_t<xtablestate_t>   clone();

 public:  // apis for user query
    void        set_account_index(const std::string & account, const base::xaccount_index_t & account_index);
    bool        get_account_index(const std::string & account, base::xaccount_index_t & account_index);
    uint64_t    get_account_size() const { return m_accountindex_state->get_account_size(); }
    bool        find_receiptid_pair(base::xtable_shortid_t sid, base::xreceiptid_pair_t & pair);
    const base::xreceiptid_state_ptr_t &        get_receiptid_state() const {return m_receiptid_state;}
    const base::xtable_mbt_new_state_ptr_t &    get_accountindex_state() const {return m_accountindex_state;}

 protected:
    bool        execute_lighttable(base::xvblock_t* block);
    bool        execute_fulltable(base::xvblock_t* block);
    bool        serialize_from_full_offdata(const std::string & full_offdata);
    bool        serialize_from_binlog(const std::string & binlog);
    bool        set_block_full_data(const xobject_ptr_t<base::xvboffdata_t>  & full_data);
    bool        set_block_binlog_data(const xobject_ptr_t<base::xvboffdata_t>  & binlog_data);
    void        set_full_height(uint64_t height) {m_full_height = height;}
    void        set_binlog_height(uint64_t height) {m_binlog_height = height;}
    xobject_ptr_t<base::xvboffdata_t>   get_block_binlog_data() const;
    xobject_ptr_t<base::xvboffdata_t>   clone_block_full_data() const;

 private:
    base::xtable_mbt_new_state_ptr_t    m_accountindex_state{nullptr};
    base::xreceiptid_state_ptr_t        m_receiptid_state{nullptr};

    // xobject_ptr_t<base::xvboffdata_t>   m_last_full{nullptr};  // TODO(jimmy)
    // xobject_ptr_t<base::xvboffdata_t>   m_binlog{nullptr};
 private:  // local members
    uint64_t                        m_full_height{0};
    uint64_t                        m_binlog_height{0};
};

using xtablestate_ptr_t = xobject_ptr_t<xtablestate_t>;

NS_END2
