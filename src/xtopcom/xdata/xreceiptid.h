// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include <vector>
#include <set>
#include "xvledger/xvblock.h"
#include "xbasic/xns_macro.h"
#include "xbasic/xobject_ptr.h"
#include "xbasic/xdataobj_base.hpp"

NS_BEG2(top, data)

// TODO(jimmy) move to xvledger
// table short id
using xtable_sid_t = uint16_t;  //-[4 bit:zone#/bucket-index][7 bit:book-index][3 bit:table-index]
#define make_table_sid(zone_index, subaddr) ( (zone_index) << 10 | (subaddr) )

// receipt id relations between the two table pair
class xreceiptid_pair_t {
 public:
    xreceiptid_pair_t() = default;
    xreceiptid_pair_t(uint64_t sendid_max, uint32_t unconfirm_num, uint64_t recvid_max);

 public:
    int32_t         do_write(base::xstream_t & stream) const;
    int32_t         do_read(base::xstream_t & stream);
    std::string     dump() const;
    uint64_t        get_sendid_max() const {return m_send_id_max;}
    uint64_t        get_sendid_min() const {return m_send_id_max - m_unconfirm_num;}
    uint64_t        get_recvid_max() const {return m_recv_id_max;}
    void            inc_sendid_max() {m_send_id_max++;}

 private:
    uint64_t    m_send_id_max{0};
    uint32_t    m_unconfirm_num{0};
    uint64_t    m_recv_id_max{0};
};

// the receiptid of the current table with all other tables
class xreceiptid_pairs_t : public base::xdataunit_t {
 public:
    xreceiptid_pairs_t();
 protected:
    ~xreceiptid_pairs_t() {}
    int32_t         do_write(base::xstream_t & stream) override;
    int32_t         do_read(base::xstream_t & stream) override;
 public:
    void            add_pair(xtable_sid_t sid, const xreceiptid_pair_t & pair);
    void            add_pairs(const std::map<xtable_sid_t, xreceiptid_pair_t> & pairs);
    void            add_binlog(const xobject_ptr_t<xreceiptid_pairs_t> & binlog);
    void            clear_binlog();

    bool            find_pair(xtable_sid_t sid, xreceiptid_pair_t & pair);
    const std::map<xtable_sid_t, xreceiptid_pair_t> & get_all_pairs() const {return m_all_pairs;}
    size_t          get_size() const{return m_all_pairs.size();}

 private:
    std::map<xtable_sid_t, xreceiptid_pair_t>   m_all_pairs;
};
using xreceiptid_pairs_ptr_t = xobject_ptr_t<xreceiptid_pairs_t>;

// the state of receiptid pairs include last full state and newest binlog
class xreceiptid_state_t : public base::xdataunit_t {
 public:
    xreceiptid_state_t();
    xreceiptid_state_t(const xreceiptid_pairs_ptr_t & last_full, const xreceiptid_pairs_ptr_t & binlog);
 protected:
    ~xreceiptid_state_t() {}
    int32_t         do_write(base::xstream_t & stream) override;
    int32_t         do_read(base::xstream_t & stream) override;

 public:
    void            add_pair(xtable_sid_t sid, const xreceiptid_pair_t & pair);
    void            add_pairs(const std::map<xtable_sid_t, xreceiptid_pair_t> & pairs);
    bool            find_pair(xtable_sid_t sid, xreceiptid_pair_t & pair);
    size_t          get_full_size() const{return m_last_full->get_size();}
    size_t          get_binlog_size() const{return m_binlog->get_size();}

    void            merge_new_full();
    std::string     build_root_hash();
    void            set_binlog(const xreceiptid_pairs_ptr_t & binlog) {m_binlog = binlog;}
    const xreceiptid_pairs_ptr_t &  get_binlog() const {return m_binlog;}
 private:
    xreceiptid_pairs_ptr_t  m_last_full{nullptr};
    xreceiptid_pairs_ptr_t  m_binlog{nullptr};
};
using xreceiptid_state_ptr_t = xobject_ptr_t<xreceiptid_state_t>;

NS_END2
