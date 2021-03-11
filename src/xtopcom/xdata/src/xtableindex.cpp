// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xtableindex.h"
#include "xdata/xdata_common.h"
#include "xbasic/xversion.h"
#include "xbase/xhash.h"
#include "xutility/xhash.h"

NS_BEG2(top, data)

REG_CLS(xtable_mbt_t);

xaccount_index_t::xaccount_index_t(uint64_t height,
                                    uint64_t clock) {
    set_latest_unit_height(height);
    set_latest_unit_clock(clock);
}

xaccount_index_t::xaccount_index_t(uint64_t height,
                                    uint64_t clock,
                                    const std::string & block_hash,
                                    base::enum_xvblock_class block_class,
                                    base::enum_xvblock_type block_type,
                                    enum_xblock_consensus_type consensus_type,
                                    bool has_unconfirm_tx,
                                    bool is_account_destroy) {
    set_latest_unit_height(height);
    set_latest_unit_clock(clock);
    set_latest_unit_hash(block_hash);
    set_latest_unit_class(block_class);
    set_latest_unit_type(block_type);
    set_latest_unit_consensus_type(consensus_type);
    if (has_unconfirm_tx) {
        set_account_index_flag(enum_xaccount_index_flag_has_unconfirm_tx);
    }
    if (is_account_destroy) {
        set_account_index_flag(enum_xaccount_index_flag_account_destroy);
    }
}

int32_t xaccount_index_t::do_write(base::xstream_t & stream) const {
    KEEP_SIZE();
    stream << m_latest_unit_height;
    stream << m_latest_unit_clock;
    stream << m_latest_unit_hash;
    stream << m_account_flag;
    return CALC_LEN();
}

int32_t xaccount_index_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    stream >> m_latest_unit_height;
    stream >> m_latest_unit_clock;
    stream >> m_latest_unit_hash;
    stream >> m_account_flag;
    return CALC_LEN();
}

// [enum_xvblock_class 3bit][enum_xvblock_type 7bit][enum_xaccount_index_flag 4bit][enum_xblock_consensus_type 2bit] = 16bits
void xaccount_index_t::set_latest_unit_class(base::enum_xvblock_class _class) {
    m_account_flag = (m_account_flag & 0x3FFF) | (_class << 13);
}
void xaccount_index_t::set_latest_unit_type(base::enum_xvblock_type _type) {
    m_account_flag = (m_account_flag & 0xE03F) | (_type << 6);
}
void xaccount_index_t::set_account_index_flag(enum_xaccount_index_flag _flag) {
    enum_xaccount_index_flag old_flag = get_account_index_flag();
    enum_xaccount_index_flag new_flag = (enum_xaccount_index_flag)(old_flag | _flag);

    m_account_flag = (m_account_flag & 0xFFC3) | (new_flag << 2);
}
void xaccount_index_t::set_latest_unit_consensus_type(enum_xblock_consensus_type _type) {
    m_account_flag = (m_account_flag & 0xFFFC) | (_type);
}

bool xaccount_index_t::check_account_index_flag(enum_xaccount_index_flag _flag) const {
    enum_xaccount_index_flag copy_flags = get_account_index_flag();
    return ((copy_flags & _flag) != 0);
}

void xaccount_index_t::set_latest_unit_hash(const std::string & hash) {
    uint32_t hash32 = base::xhash32_t::digest(hash);
    xassert(hash32 != m_latest_unit_hash);
    m_latest_unit_hash = hash32;
}

bool xaccount_index_t::is_match_unit_hash(const std::string & unit_hash) const {
    uint32_t hash32 = base::xhash32_t::digest(unit_hash);
    return hash32 == m_latest_unit_hash;
}

xtable_mbt_binlog_t::xtable_mbt_binlog_t()
:base::xdataunit_t(base::xdataunit_t::enum_xdata_type_undefine) {

}
xtable_mbt_binlog_t::xtable_mbt_binlog_t(const std::map<std::string, xaccount_index_t> & changed_indexs)
:base::xdataunit_t(base::xdataunit_t::enum_xdata_type_undefine) {
    m_account_indexs = changed_indexs;
}

int32_t xtable_mbt_binlog_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    uint32_t count = m_account_indexs.size();
    xassert(count != 0);
    stream << count;
    for (auto & v : m_account_indexs) {
        stream << v.first;
        v.second.do_write(stream);
    }
    return CALC_LEN();
}
int32_t xtable_mbt_binlog_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    uint32_t count;
    stream >> count;
    for (uint32_t i = 0; i < count; i++) {
        std::string account;
        stream >> account;
        xaccount_index_t index;
        index.do_read(stream);
        m_account_indexs[account] = index;
    }
    xassert(m_account_indexs.size() == count);
    return CALC_LEN();
}

void xtable_mbt_binlog_t::set_account_index(const std::string & account, const xaccount_index_t & index) {
    m_account_indexs[account] = index;
}

void xtable_mbt_binlog_t::set_accounts_index(const std::map<std::string, xaccount_index_t> & changed_indexs) {
    for (auto & v : changed_indexs) {
        m_account_indexs[v.first] = v.second;
    }
}

bool xtable_mbt_binlog_t::get_unit_index(const std::string & account, xaccount_index_t & index) const {
    auto iter = m_account_indexs.find(account);
    if (iter != m_account_indexs.end()) {
        index = iter->second;
        return true;
    }
    return false;
}

std::string xtable_mbt_binlog_t::build_binlog_hash() {
    base::xstream_t stream(top::base::xcontext_t::instance());
    int32_t size = do_write(stream);
    xassert(size > 0);
    uint256_t hash256 = utl::xsha2_256_t::digest((const char*)stream.data(), stream.size());
    std::string hash = std::string(reinterpret_cast<char*>(hash256.data()), hash256.size());
    return hash;
}


xtable_mbt_bucket_node_t::xtable_mbt_bucket_node_t()
:base::xdataunit_t(base::xdataunit_t::enum_xdata_type_undefine) {
}

int32_t xtable_mbt_bucket_node_t::do_write_without_hash(base::xstream_t & stream) const {
    KEEP_SIZE();
    uint32_t count = m_unit_indexs.size();
    xassert(count != 0);
    stream << count;
    for (auto & v : m_unit_indexs) {
        stream << v.first;
        v.second.do_write(stream);
    }
    return CALC_LEN();
}

int32_t xtable_mbt_bucket_node_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    stream << m_bucket_hash;
    do_write_without_hash(stream);
    return CALC_LEN();
}
int32_t xtable_mbt_bucket_node_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    stream >> m_bucket_hash;
    uint32_t count;
    stream >> count;
    for (uint32_t i = 0; i < count; i++) {
        std::string account;
        stream >> account;
        xaccount_index_t index;
        index.do_read(stream);
        m_unit_indexs[account] = index;
    }
    xassert(m_unit_indexs.size() == count);
    return CALC_LEN();
}

void    xtable_mbt_bucket_node_t::set_unit_index(const std::string & account, xaccount_index_t index) {
    m_unit_indexs[account] = index;
    m_bucket_hash.clear();
}
bool    xtable_mbt_bucket_node_t::get_unit_index(const std::string & account, xaccount_index_t & index) const {
    auto iter = m_unit_indexs.find(account);
    if (iter != m_unit_indexs.end()) {
        index = iter->second;
        return true;
    }
    return false;
}

// should check bucket after sync
bool xtable_mbt_bucket_node_t::check_bucket() const {
    std::string new_bucket_hash = calc_bucket_hash();
    return m_bucket_hash == new_bucket_hash;
}

std::string xtable_mbt_bucket_node_t::build_bucket_hash() {
    if (m_bucket_hash.empty()) {
        m_bucket_hash = calc_bucket_hash();
    }
    return m_bucket_hash;
}
std::string xtable_mbt_bucket_node_t::calc_bucket_hash() const {
    base::xstream_t stream(top::base::xcontext_t::instance());
    int32_t size = do_write_without_hash(stream);
    xassert(size > 0);
    uint256_t hash256 = utl::xsha2_256_t::digest((const char*)stream.data(), stream.size());
    std::string bucket_hash = std::string(reinterpret_cast<char*>(hash256.data()), hash256.size());
    return bucket_hash;
}


xtable_mbt_root_node_t::xtable_mbt_root_node_t()
:base::xdataunit_t(base::xdataunit_t::enum_xdata_type_undefine) {
}

xtable_mbt_root_node_t::xtable_mbt_root_node_t(const std::map<uint16_t, std::string> & bucket_hashs)
:base::xdataunit_t(base::xdataunit_t::enum_xdata_type_undefine) {
    m_bucket_hashs = bucket_hashs;
}

xtable_mbt_root_node_t::xtable_mbt_root_node_t(const std::map<uint16_t, std::string> & bucket_hashs, const std::string & root_hash)
:base::xdataunit_t(base::xdataunit_t::enum_xdata_type_undefine) {
    m_bucket_hashs = bucket_hashs;
    m_root_hash = root_hash;
}

int32_t xtable_mbt_root_node_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    stream << m_root_hash;
    stream << m_bucket_hashs;
    return CALC_LEN();
}
int32_t xtable_mbt_root_node_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    stream >> m_root_hash;
    stream >> m_bucket_hashs;
    return CALC_LEN();
}

std::string xtable_mbt_root_node_t::get_bucket_hash(uint16_t bucket) {
    auto iter = m_bucket_hashs.find(bucket);
    if (iter != m_bucket_hashs.end()) {
        return iter->second;
    }
    return {};
}

std::string xtable_mbt_root_node_t::build_root_hash() {
    if (m_root_hash.empty()) {
        m_root_hash = calc_root_hash();
    }
    return m_root_hash;
}

std::string xtable_mbt_root_node_t::calc_root_hash() const {
    base::xstream_t stream(top::base::xcontext_t::instance());
    stream << m_bucket_hashs;
    uint256_t hash256 = utl::xsha2_256_t::digest((const char*)stream.data(), stream.size());
    std::string root_hash = std::string(reinterpret_cast<char*>(hash256.data()), hash256.size());
    return root_hash;
}

bool xtable_mbt_root_node_t::check_root_node() const {
    std::string new_root_hash = calc_root_hash();
    return m_root_hash == new_root_hash;
}


xobject_ptr_t<xtable_mbt_t> xtable_mbt_t::build_tree(const xtable_mbt_root_node_ptr_t & root_node,
                                                     const std::map<uint16_t, xtable_mbt_bucket_node_ptr_t> & bucket_nodes) {
    xtable_mbt_ptr_t tree = make_object_ptr<xtable_mbt_t>(bucket_nodes);
    std::string root = tree->build_root_hash();
    if (root == root_node->get_root_hash()) {
        return tree;
    }
    xwarn("xtable_mbt_t::build_tree fail-root hash not match");
    return nullptr;
}

xobject_ptr_t<xtable_mbt_t> xtable_mbt_t::build_new_tree(const xobject_ptr_t<xtable_mbt_t> & last_tree,
                                                      const xtable_mbt_binlog_ptr_t & binlog) {
    // clone a new tree
    std::string tree_str;
    int32_t ret = last_tree->serialize_to_string(tree_str);
    xassert(ret > 0);
    xtable_mbt_ptr_t new_tree = make_object_ptr<xtable_mbt_t>();
    ret = new_tree->serialize_from_string(tree_str);
    xassert(ret > 0);
    new_tree->set_accounts_index_info(binlog->get_accounts_index());
    return new_tree;
}

xtable_mbt_t::xtable_mbt_t() {

}

xtable_mbt_t::xtable_mbt_t(const std::map<uint16_t, xtable_mbt_bucket_node_ptr_t> & bucket_nodes) {
    m_buckets = bucket_nodes;
}

uint16_t xtable_mbt_t::account_to_index(const std::string & account) const {
    uint32_t account_hash = base::xhash32_t::digest(account);
    uint16_t account_index = account_hash % enum_tableindex_bucket_num;
    return account_index;
}

void xtable_mbt_t::set_unit_index(const std::string & account, const xaccount_index_t & info) {
    uint16_t account_index = account_to_index(account);
    xtable_mbt_bucket_node_ptr_t bucket;
    auto iter = m_buckets.find(account_index);
    if (iter != m_buckets.end()) {
        bucket = iter->second;
    } else {
        bucket = make_object_ptr<xtable_mbt_bucket_node_t>();
        m_buckets[account_index] = bucket;
    }
    bucket->set_unit_index(account, info);
}
void xtable_mbt_t::set_accounts_index_info(const std::map<std::string, xaccount_index_t> & accounts_info, bool is_build_root_hash) {
    xassert(accounts_info.size() > 0);
    std::map<uint16_t, xtable_mbt_bucket_node_ptr_t>    modify_buckets;
    for (auto & v : accounts_info) {
        uint16_t account_index = account_to_index(v.first);
        xtable_mbt_bucket_node_ptr_t bucket;
        auto iter = modify_buckets.find(account_index);
        if (iter != modify_buckets.end()) {
            bucket = iter->second;
        } else {
            bucket = make_object_ptr<xtable_mbt_bucket_node_t>();
            modify_buckets[account_index] = bucket;
        }
        bucket->set_unit_index(v.first, v.second);
    }

    // update modify buckets
    for (auto & modify_bucket : modify_buckets) {
        m_buckets[modify_bucket.first] = modify_bucket.second;
    }

    // calc tree root hash
    if (is_build_root_hash) {
        build_root_hash();
    }
}

int32_t xtable_mbt_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    stream << m_root_hash;
    uint32_t count = m_buckets.size();
    stream << count;
    for (auto & bucket : m_buckets) {
        stream << bucket.first;
        bucket.second->serialize_to(stream);
    }
    return CALC_LEN();
}
int32_t xtable_mbt_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    stream >> m_root_hash;
    uint32_t count;
    stream >> count;
    for (uint32_t i = 0; i < count; i++) {
        uint16_t key;
        stream >> key;
        xtable_mbt_bucket_node_ptr_t value = make_object_ptr<xtable_mbt_bucket_node_t>();
        value->serialize_from(stream);
        m_buckets[key] = value;
    }
    return CALC_LEN();
}

xtable_mbt_root_node_ptr_t xtable_mbt_t::get_root_node() const {
    std::map<uint16_t, std::string> bucket_hashs;
    for (auto & bucket : m_buckets) {
        bucket_hashs[bucket.first] = bucket.second->get_bucket_hash();
    }
    xtable_mbt_root_node_ptr_t root_node = make_object_ptr<xtable_mbt_root_node_t>(bucket_hashs, m_root_hash);
    return root_node;
}

xtable_mbt_bucket_node_ptr_t xtable_mbt_t::get_bucket_node(uint16_t bucket_index) const {
    auto iter = m_buckets.find(bucket_index);
    if (iter != m_buckets.end()) {
        return iter->second;
    }
    return nullptr;
}

std::string xtable_mbt_t::build_root_hash() {
    std::map<uint16_t, std::string> bucket_hashs;
    for (auto & bucket : m_buckets) {
        bucket_hashs[bucket.first] = bucket.second->build_bucket_hash();
    }
    xtable_mbt_root_node_ptr_t root_node = make_object_ptr<xtable_mbt_root_node_t>(bucket_hashs);
    m_root_hash = root_node->build_root_hash();
    return m_root_hash;
}

bool xtable_mbt_t::check_tree() const {
    bool ret;
    for (auto & bucket : m_buckets) {
        ret = bucket.second->check_bucket();
        if (!ret) {
            xassert(0);
            return ret;
        }
    }
    xtable_mbt_root_node_ptr_t root_node = get_root_node();
    return root_node->check_root_node();
}

bool xtable_mbt_t::get_unit_index(const std::string & account, xaccount_index_t & index) const {
    uint16_t account_index = account_to_index(account);
    auto iter = m_buckets.find(account_index);
    if (iter != m_buckets.end()) {
        const xtable_mbt_bucket_node_ptr_t & bucket = iter->second;
        return bucket->get_unit_index(account, index);
    }
    return false;
}

size_t xtable_mbt_t::get_account_size() const {
    size_t count = 0;
    for (auto & bucket : m_buckets) {
        count += bucket.second->get_account_size();
    }
    return count;
}

NS_END2
