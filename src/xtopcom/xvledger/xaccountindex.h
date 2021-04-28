// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include "xbase/xobject_ptr.h"
#include "xbase/xns_macro.h"
#include "xvledger/xvblock.h"
#include "xvledger/xdataobj_base.hpp"

NS_BEG2(top, base)

// [enum_xvblock_class 3bit][enum_xvblock_type 7bit][enum_xaccount_index_flag 4bit][enum_xblock_consensus_type 2bit] = 16bits
// the account flag for checking need sync or load firstly 4bit
enum enum_xaccount_index_flag {
    enum_xaccount_index_flag_account_destroy  = 0x01,  // the account has been destroyed
    enum_xaccount_index_flag_has_unconfirm_tx = 0x02,  // the account has unconfirm send tx
    enum_xaccount_index_flag_res1             = 0x04,  // reserved
    enum_xaccount_index_flag_res2             = 0x08,  // reserved
};
// the latest block consensus flag 2bit
enum enum_xblock_consensus_type {
    enum_xblock_consensus_flag_authenticated  = 0,
    enum_xblock_consensus_flag_locked         = 1,
    enum_xblock_consensus_flag_committed      = 2,
    enum_xblock_consensus_flag_res            = 3,
};

// account index info is the index info of account unit blockchain
class xaccount_index_t {
 public:
    xaccount_index_t() = default;
    xaccount_index_t(uint64_t height);
    xaccount_index_t(uint64_t height,
                     const std::string & block_hash,
                     enum_xvblock_class block_class,
                     enum_xvblock_type block_type,
                     enum_xblock_consensus_type consensus_type,
                     bool has_unconfirm_tx,
                     bool is_account_destroy);

    bool operator == (const xaccount_index_t &other) const {
        if (m_latest_unit_height == other.m_latest_unit_height
            && m_latest_unit_hash == other.m_latest_unit_hash
            && m_account_flag == other.m_account_flag) {
            return true;
        }
        return false;
    }

    int32_t do_write(base::xstream_t & stream) const;
    int32_t do_read(base::xstream_t & stream);
    std::string     dump() const;

 public:
    uint64_t                get_latest_unit_height() const {return m_latest_unit_height;}
    uint16_t                get_latest_unit_hash16() const {return m_latest_unit_hash;}
    bool                    is_match_unit_hash(const std::string & unit_hash) const;
    bool                    is_has_unconfirm_tx() const {return check_account_index_flag(enum_xaccount_index_flag_has_unconfirm_tx);}
    bool                    is_account_destroy() const {return check_account_index_flag(enum_xaccount_index_flag_account_destroy);}

    // [enum_xvblock_class 3bit][enum_xvblock_type 7bit][enum_xaccount_index_flag 4bit][enum_xblock_consensus_type 2bit] = 16bits
    base::enum_xvblock_class    get_latest_unit_class() const {return (base::enum_xvblock_class)((m_account_flag >> 13) & 0x07);}
    base::enum_xvblock_type     get_latest_unit_type() const {return (base::enum_xvblock_type)((m_account_flag >> 6) & 0x7F);}
    enum_xaccount_index_flag    get_account_index_flag() const {return (enum_xaccount_index_flag)((m_account_flag >> 2) & 0x0F);}
    enum_xblock_consensus_type  get_latest_unit_consensus_type() const {return (enum_xblock_consensus_type)((m_account_flag) & 0x03);}
    bool                        check_account_index_flag(enum_xaccount_index_flag _flag) const;

 public:
    // [enum_xvblock_class 3bit][enum_xvblock_type 7bit][enum_xaccount_index_flag 4bit][enum_xblock_consensus_type 2bit] = 16bits
    void                    set_latest_unit_class(base::enum_xvblock_class _class);
    void                    set_latest_unit_type(base::enum_xvblock_type _type);
    void                    set_account_index_flag(enum_xaccount_index_flag _flag);
    void                    set_latest_unit_consensus_type(enum_xblock_consensus_type _type);
    void                    set_latest_unit_height(uint64_t _height) {m_latest_unit_height = _height;}
    void                    set_latest_unit_hash(const std::string & hash);

 private:
    uint64_t        m_latest_unit_height{0};
    uint16_t        m_latest_unit_hash{0};
    uint16_t        m_account_flag{0};  // [enum_xvblock_class 3bit][enum_xvblock_type 7bit][enum_xaccount_index_flag 4bit][enum_xblock_consensus_type 2bit] = 16bits
};

// the binlog of table mbt index state from last full-tableblock
class xtable_mbt_binlog_t : public xbase_dataunit_t<xtable_mbt_binlog_t, xdata_type_accountindex_binlog> {
 public:
    xtable_mbt_binlog_t();
    xtable_mbt_binlog_t(const std::map<std::string, xaccount_index_t> & changed_indexs);
 protected:
    ~xtable_mbt_binlog_t() = default;

    int32_t do_write(base::xstream_t & stream) override;
    int32_t do_read(base::xstream_t & stream) override;

 public:
    void            set_accounts_index(const std::map<std::string, xaccount_index_t> & changed_indexs);
    void            set_account_index(const std::string & account, const xaccount_index_t & index);
    std::string     build_binlog_hash();
    void            clear_binlog();

 public:
    const std::map<std::string, xaccount_index_t> &     get_accounts_index() const {return m_account_indexs;}
    size_t                                              get_account_size() const {return m_account_indexs.size();}
    bool                                                get_account_index(const std::string & account, xaccount_index_t & index) const;

 public:
    void            set_height(uint64_t height) {m_height = height;}
    uint64_t        get_height() const {return m_height;}

 private:
    std::map<std::string, xaccount_index_t>   m_account_indexs;

 private:
    uint64_t                                  m_height{0};  // the state of height
};

using xtable_mbt_binlog_ptr_t = xobject_ptr_t<xtable_mbt_binlog_t>;

// mbt is the merkle bucket tree, which has some buckets node and a root node
class xtable_mbt_bucket_node_t : public base::xdataunit_t {
 public:
    xtable_mbt_bucket_node_t();

 protected:
    ~xtable_mbt_bucket_node_t() = default;
    int32_t do_write(base::xstream_t & stream) override;
    int32_t do_read(base::xstream_t & stream) override;
    int32_t do_write_without_hash(base::xstream_t & stream) const;

 public:
    void                    set_account_index(const std::string & account, xaccount_index_t index);
    std::string             build_bucket_hash();
 public:
    bool                    get_account_index(const std::string & account, xaccount_index_t & index) const;
    size_t                  get_account_size() const {return m_account_indexs.size();}
    const std::map<std::string, xaccount_index_t> & get_account_indexs() const {return m_account_indexs;}
 private:
    std::string             calc_bucket_hash() const;
 private:
    std::map<std::string, xaccount_index_t>   m_account_indexs;
};

using xtable_mbt_bucket_node_ptr_t = xobject_ptr_t<xtable_mbt_bucket_node_t>;

class xtable_mbt_root_node_t : public base::xdataunit_t {
 public:
    xtable_mbt_root_node_t();
    explicit xtable_mbt_root_node_t(const std::map<uint16_t, std::string> & bucket_hashs);
    xtable_mbt_root_node_t(const std::map<uint16_t, std::string> & bucket_hashs, const std::string & root_hash);

 protected:
    ~xtable_mbt_root_node_t() = default;
    int32_t do_write(base::xstream_t & stream) override;
    int32_t do_read(base::xstream_t & stream) override;

 public:
    bool            check_root_node() const;  // check if root node is correct
    std::string     build_root_hash();  // calc root hash from all buckets hash

 public:
    const std::string &                     get_root_hash() const {return m_root_hash;}
    const std::map<uint16_t, std::string> & get_bucket_hashs() const {return m_bucket_hashs;}
    std::string                             get_bucket_hash(uint16_t bucket);

 private:
    std::string             calc_root_hash() const;

 private:
    std::string                         m_root_hash;
    std::map<uint16_t, std::string>     m_bucket_hashs;
};

using xtable_mbt_root_node_ptr_t = xobject_ptr_t<xtable_mbt_root_node_t>;

// xtable_mbt_t is used for table index state, which includes all accounts' unit index info.
class xtable_mbt_t : public xbase_dataunit_t<xtable_mbt_t, xdata_type_table_mbt> {
 private:
    enum { enum_tableindex_bucket_num = 16 };
 public:
    static xobject_ptr_t<xtable_mbt_t> build_tree(const xtable_mbt_root_node_ptr_t & root_node,
                                                               const std::map<uint16_t, xtable_mbt_bucket_node_ptr_t> & bucket_nodes);
    static xobject_ptr_t<xtable_mbt_t> build_new_tree(const xobject_ptr_t<xtable_mbt_t> & last_tree,
                                                      const xtable_mbt_binlog_ptr_t & binlog);
 public:
    xtable_mbt_t();
    explicit xtable_mbt_t(const std::map<uint16_t, xtable_mbt_bucket_node_ptr_t> & bucket_nodes);

 protected:
    ~xtable_mbt_t() = default;

    int32_t do_write(base::xstream_t & stream) override;
    int32_t do_read(base::xstream_t & stream) override;

 public:
    void                            set_accounts_index_info(const std::map<std::string, xaccount_index_t> & accounts_info, bool is_build_root_hash = true);  // must set together
    std::string                     build_root_hash();
    void                            add_binlog(const xtable_mbt_binlog_ptr_t & binlog);
 public:
    xtable_mbt_root_node_ptr_t          get_root_node();
    xtable_mbt_bucket_node_ptr_t        get_bucket_node(uint16_t bucket_index) const;
    size_t                              get_account_size() const;
    size_t                              get_bucket_size() const {return m_buckets.size();}
    size_t                              get_max_bucket_size() const {return enum_tableindex_bucket_num;}
    bool                                get_account_index(const std::string & account, xaccount_index_t & index) const;
    // const std::string &                 get_root_hash() const {return m_root_hash;}
    const std::map<uint16_t, xtable_mbt_bucket_node_ptr_t> & get_buckets() const {return m_buckets;}

 public:
    void            set_height(uint64_t height) {m_height = height;}
    uint64_t        get_height() const {return m_height;}

 private:
    void            set_account_index(const std::string & account, const xaccount_index_t & info);
    uint16_t        account_to_index(const std::string & account) const;

 private:
    // std::string                                         m_root_hash;
    std::map<uint16_t, xtable_mbt_bucket_node_ptr_t>    m_buckets;

 private:
    uint64_t                                            m_height{0};  // the state of height
};

using xtable_mbt_ptr_t = xobject_ptr_t<xtable_mbt_t>;

// the newest mbt state include last full state and the newest binlog.
class xtable_mbt_new_state_t : public base::xdataunit_t {
 public:
    xtable_mbt_new_state_t();
    xtable_mbt_new_state_t(const xtable_mbt_ptr_t & last_mbt, const xtable_mbt_binlog_ptr_t & binlog);

 protected:
    ~xtable_mbt_new_state_t() {}
    int32_t         do_write(base::xstream_t & stream) override;
    int32_t         do_read(base::xstream_t & stream) override;

 public:
    void        set_last_full_state(const xtable_mbt_ptr_t & last_mbt) {m_last_full_state = last_mbt;}
    void        set_binlog(const xtable_mbt_binlog_ptr_t & binlog) {m_newest_binlog_state = binlog;}
    void        clear_binlog() {m_newest_binlog_state = make_object_ptr<xtable_mbt_binlog_t>();}
    void        merge_new_full();

 public:
    const xtable_mbt_ptr_t &        get_last_full_state() const {return m_last_full_state;}
    const xtable_mbt_binlog_ptr_t & get_binlog() const {return m_newest_binlog_state;}
    bool                            get_account_index(const std::string & account, xaccount_index_t & account_index);
    uint64_t                        get_account_size() const { return m_last_full_state->get_account_size(); }

 private:
    xtable_mbt_ptr_t            m_last_full_state{nullptr};
    xtable_mbt_binlog_ptr_t     m_newest_binlog_state{nullptr};
};
using xtable_mbt_new_state_ptr_t = xobject_ptr_t<xtable_mbt_new_state_t>;

NS_END2
