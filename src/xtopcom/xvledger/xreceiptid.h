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
#include "xstatistic/xstatistic.h"

NS_BEG2(top, base)

// receipt id relations between the two table pair
class xreceiptid_pair_t : public xstatistic::xstatistic_obj_face_t {
 public:
    xreceiptid_pair_t();
    xreceiptid_pair_t(uint64_t sendid, uint64_t confirmid, uint64_t recvid, uint64_t send_rsp_id, uint64_t confirm_rsp_id);
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

    uint64_t        get_send_rsp_id_max() const {return m_send_rsp_id_max;}
    uint64_t        get_confirm_rsp_id_max() const {return m_send_rsp_id_max - m_unconfirm_rsp_num;}
    uint32_t        get_unconfirm_rsp_num() const {return m_unconfirm_rsp_num;}
    bool            all_confirmed_as_sender() const {return m_unconfirm_rsp_num == 0;}

 public:
    void            set_sendid_max(uint64_t value);
    void            set_confirmid_max(uint64_t value);
    void            set_recvid_max(uint64_t value);
    void            set_send_rsp_id_max(uint64_t value);
    void            set_confirm_rsp_id_max(uint64_t value);

    virtual int32_t get_class_type() const override {return xstatistic::enum_statistic_receiptid_pair;}
private:
    virtual size_t get_object_size_real() const override {return sizeof(*this);}

 private:
    uint64_t    m_send_id_max{0};
    uint32_t    m_unconfirm_num{0};
    uint64_t    m_recv_id_max{0};
    uint64_t    m_send_rsp_id_max{0};
    uint32_t    m_unconfirm_rsp_num{0};
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
    void            set_send_rsp_id_max(xtable_shortid_t sid, uint64_t value);
    void            set_confirm_rsp_id_max(xtable_shortid_t sid, uint64_t value);
    std::string     dump() const;

 public:
    bool            find_pair(xtable_shortid_t sid, xreceiptid_pair_t & pair) const;
    const std::map<xtable_shortid_t, xreceiptid_pair_t> & get_all_pairs() const {return m_all_pairs;}
    size_t          get_size() const{return m_all_pairs.size();}

    int32_t         get_object_size_real() const;

 private:
    std::map<xtable_shortid_t, xreceiptid_pair_t>   m_all_pairs;
};
using xreceiptid_pairs_ptr_t = std::shared_ptr<xreceiptid_pairs_t>;

// the state of receiptid pairs include last full state and newest binlog
class xreceiptid_state_t {
 public:
    xreceiptid_state_t();
    xreceiptid_state_t(xtable_shortid_t tableid, uint64_t height);
    ~xreceiptid_state_t() {}

 public:
    void        add_pair(xtable_shortid_t sid, const xreceiptid_pair_t & pair);
    bool        find_pair(xtable_shortid_t sid, xreceiptid_pair_t & pair) const;
    void        update_unconfirm_tx_num();
    void        set_tableid_and_height(xtable_shortid_t tableid, uint64_t height);

    uint32_t            get_unconfirm_tx_num() const;  // just for debug    
    uint32_t            get_unconfirm_rsp_tx_num() const;
    xtable_shortid_t    get_self_tableid() const {return m_self_tableid;}
    uint64_t            get_block_height() const {return m_height;}

 public: // just for block build
    void        clear_pair_modified();
    bool        find_pair_modified(xtable_shortid_t sid, xreceiptid_pair_t & pair) const;
    void        add_pair_modified(xtable_shortid_t sid, const xreceiptid_pair_t & pair);
    const xreceiptid_pairs_ptr_t & get_all_receiptid_pairs() const {return m_binlog;}

    int32_t     get_object_size_real() const;

 private:
    xreceiptid_pairs_ptr_t  m_binlog{nullptr};
    uint32_t                m_unconfirm_tx_num{0};
    uint32_t                m_unconfirm_rsp_tx_num{0};
    xtable_shortid_t        m_self_tableid{0};
    uint64_t                m_height{0};
 private:
    xreceiptid_pairs_ptr_t  m_modified_binlog{nullptr};  // for block maker cache
};
using xreceiptid_state_ptr_t = std::shared_ptr<xreceiptid_state_t>;

class xids_check_t {
public:
    void set_id(xtable_shortid_t sid, uint64_t value);
    uint32_t size() const;
    bool check_continuous(const xreceiptid_state_ptr_t & receiptid_state) const;
    void get_modified_pairs(const base::xreceiptid_state_ptr_t & receiptid_state, xreceiptid_pairs_ptr_t & modified_pairs) const;
   //  void dump(std::stringstream & ss) const;
private:
    static bool check_receiptids_contious(const std::set<uint64_t> & ids, uint64_t begin_id);
    virtual uint64_t get_begin_id(xreceiptid_pair_t & pair) const = 0;
    virtual void set_receiptid_max(xreceiptid_pair_t & pair, uint64_t receiptid) const = 0;
private:
    std::map<xtable_shortid_t, std::set<uint64_t>>  m_ids;
};

class xsendids_check_t : public xids_check_t {
private:
   uint64_t get_begin_id(xreceiptid_pair_t & pair) const override;
   void set_receiptid_max(xreceiptid_pair_t & pair, uint64_t receiptid) const override;
};

class xrecvids_check_t : public xids_check_t {
private:
   uint64_t get_begin_id(xreceiptid_pair_t & pair) const override;
   void set_receiptid_max(xreceiptid_pair_t & pair, uint64_t receiptid) const override;
};

class xconfirmids_check_t : public xids_check_t {
private:
   uint64_t get_begin_id(xreceiptid_pair_t & pair) const override;
   void set_receiptid_max(xreceiptid_pair_t & pair, uint64_t receiptid) const override;
};

class xsend_rsp_ids_check_t : public xids_check_t {
private:
   uint64_t get_begin_id(xreceiptid_pair_t & pair) const override;
   void set_receiptid_max(xreceiptid_pair_t & pair, uint64_t receiptid) const override;
};

class xconfirm_rsp_ids_check_t : public xids_check_t {
private:
   uint64_t get_begin_id(xreceiptid_pair_t & pair) const override;
   void set_receiptid_max(xreceiptid_pair_t & pair, uint64_t receiptid) const override;
};

class xreceiptid_check_t {
public:
    void        set_sendid(xtable_shortid_t sid, uint64_t value);
    void        set_recvid(xtable_shortid_t sid, uint64_t value);
    void        set_confirmid(xtable_shortid_t sid, uint64_t value);
    void        set_send_rsp_id(xtable_shortid_t sid, uint64_t value);
    void        set_confirm_rsp_id(xtable_shortid_t sid, uint64_t value);

    uint32_t    sendids_size() const {return m_sendids.size();}
    uint32_t    recvids_size() const {return m_recvids.size();}
    uint32_t    confirmids_size() const {return m_confirmids.size();}
    uint32_t    send_rsp_ids_size() const {return m_send_rsp_ids.size();}
    uint32_t    confirm_rsp_ids_size() const {return m_confirm_rsp_ids.size();}

    bool        check_continuous(const xreceiptid_state_ptr_t & receiptid_state) const;

    xreceiptid_pairs_ptr_t get_modified_pairs(const base::xreceiptid_state_ptr_t & receiptid_state) const;

   //  std::string     dump() const;

 private:
    xsendids_check_t  m_sendids;
    xrecvids_check_t  m_recvids;
    xconfirmids_check_t  m_confirmids;
    xsend_rsp_ids_check_t  m_send_rsp_ids;
    xconfirm_rsp_ids_check_t  m_confirm_rsp_ids;
};


struct xreceiptid_unfinish_info_t {
    uint64_t          source_height;
    uint64_t          target_height;
    xtable_shortid_t  source_id;
    xtable_shortid_t  target_id;
    uint32_t          unrecv_num{0};
    uint32_t          unconfirm_num{0};
};

class xreceiptid_all_table_states {
public:
    void    add_table_receiptid_state(xtable_shortid_t sid, const base::xreceiptid_state_ptr_t & receiptid_state);
    std::vector<xreceiptid_unfinish_info_t> get_unfinish_info() const;

protected:
    bool  get_two_table_pair(xtable_shortid_t table1, xtable_shortid_t table2, xreceiptid_pair_t & pair, uint64_t & height) const;

private:
    std::map<xtable_shortid_t, xreceiptid_state_ptr_t>  m_all_states;
};

NS_END2
