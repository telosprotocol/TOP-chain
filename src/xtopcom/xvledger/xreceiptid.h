// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include "xbase/xdata.h"
#include "xbase/xobject_ptr.h"
#include "xbase/xns_macro.h"
#include "xvledger/xvaccount.h"

NS_BEG2(top, base)

// receipt id relations between the two table pair
class xreceiptid_pair_t {
 public:
    xreceiptid_pair_t();
    xreceiptid_pair_t(uint64_t sendid, uint64_t confirmid, uint64_t recvid);
    ~xreceiptid_pair_t();
    xreceiptid_pair_t(const xreceiptid_pair_t& copy) = delete;
 public:
    int32_t         do_write(base::xstream_t & stream) const;
    int32_t         do_read(base::xstream_t & stream);
    int32_t         serialize_to(std::string & bin_data) const;
    int32_t         serialize_from(const std::string & bin_data);

    std::string     dump() const;
    uint64_t        get_sendid_max() const {return m_send_id_max;}
    uint64_t        get_confirmid_max() const {return m_send_id_max - m_unconfirm_num;}
    uint64_t        get_recvid_max() const {return m_recv_id_max;}
    uint32_t        get_unconfirm_num() const {return m_unconfirm_num;}

 public:
    void            inc_sendid_max() {m_send_id_max++;}
    void            set_sendid_max(uint64_t value);
    void            set_confirmid_max(uint64_t value);
    void            set_recvid_max(uint64_t value);

 private:
    uint64_t    m_send_id_max{0};
    uint32_t    m_unconfirm_num{0};
    uint64_t    m_recv_id_max{0};
};

// the receiptid of the current table with all other tables
class xreceiptid_pairs_t {
 public:
    xreceiptid_pairs_t();
    ~xreceiptid_pairs_t();
 public:
    void            add_pair(xtable_shortid_t sid, const xreceiptid_pair_t & pair);
    void            clear_binlog();
    void            set_sendid_max(xtable_shortid_t sid, uint64_t value);
    void            set_confirmid_max(xtable_shortid_t sid, uint64_t value);
    void            set_recvid_max(xtable_shortid_t sid, uint64_t value);
    std::string     dump() const;

 public:
    bool            find_pair(xtable_shortid_t sid, xreceiptid_pair_t & pair);
    const std::map<xtable_shortid_t, xreceiptid_pair_t> & get_all_pairs() const {return m_all_pairs;}
    size_t          get_size() const{return m_all_pairs.size();}

 private:
    std::map<xtable_shortid_t, xreceiptid_pair_t>   m_all_pairs;
};
using xreceiptid_pairs_ptr_t = std::shared_ptr<xreceiptid_pairs_t>;

// the state of receiptid pairs include last full state and newest binlog
class xreceiptid_state_t {
 public:
    xreceiptid_state_t();
    ~xreceiptid_state_t() {}

 public:
    void        add_pair(xtable_shortid_t sid, const xreceiptid_pair_t & pair);
    bool        find_pair(xtable_shortid_t sid, xreceiptid_pair_t & pair);
    uint32_t    get_unconfirm_tx_num() const;  // just for debug
    void        update_unconfirm_tx_num();

 public: // just for block build
    void        clear_pair_modified();
    bool        find_pair_modified(xtable_shortid_t sid, xreceiptid_pair_t & pair);
    void        add_pair_modified(xtable_shortid_t sid, const xreceiptid_pair_t & pair);

 private:
    xreceiptid_pairs_ptr_t  m_binlog{nullptr};
    uint32_t                m_unconfirm_tx_num{0};
 private:
    xreceiptid_pairs_ptr_t  m_modified_binlog{nullptr};  // for block maker cache
};
using xreceiptid_state_ptr_t = std::shared_ptr<xreceiptid_state_t>;


class xreceiptid_check_t {
 public:
    void        set_sendid(xtable_shortid_t sid, uint64_t value);
    void        set_recvid(xtable_shortid_t sid, uint64_t value);
    void        set_confirmid(xtable_shortid_t sid, uint64_t value);
    uint64_t    get_sendid_max(xtable_shortid_t sid);
    uint64_t    get_recvid_max(xtable_shortid_t sid);
    uint64_t    get_confirmid_max(xtable_shortid_t sid);

    const std::map<xtable_shortid_t, std::set<uint64_t>> &  get_sendids() const {return m_sendids;}
    const std::map<xtable_shortid_t, std::set<uint64_t>> &  get_recvids() const {return m_recvids;}
    const std::map<xtable_shortid_t, std::set<uint64_t>> &  get_confirmids() const {return m_confirmids;}

    bool        check_contious(const xreceiptid_state_ptr_t & receiptid_state) const;
    void        update_state(const xreceiptid_state_ptr_t & receiptid_state) const;

    std::string     dump() const;

 private:
    bool    check_receiptids_contious(const std::set<uint64_t> & ids, uint64_t begin_id) const;

 private:
    std::map<xtable_shortid_t, std::set<uint64_t>>  m_sendids;
    std::map<xtable_shortid_t, std::set<uint64_t>>  m_recvids;
    std::map<xtable_shortid_t, std::set<uint64_t>>  m_confirmids;
};

NS_END2
