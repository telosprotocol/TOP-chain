// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "xbase/xhash.h"
#include "xbase/xcontext.h"
#include "xbase/xobject.h"
#include "xvledger/xaccountindex.h"

NS_BEG2(top, base)

REG_CLS(xtable_mbt_t);

xaccount_index_t::xaccount_index_t(uint64_t height) {
    set_latest_unit_height(height);
}

xaccount_index_t::xaccount_index_t(uint64_t height,
                                    const std::string & block_hash,
                                    base::enum_xvblock_class block_class,
                                    base::enum_xvblock_type block_type,
                                    enum_xblock_consensus_type consensus_type,
                                    bool has_unconfirm_tx,
                                    bool is_account_destroy) {
    set_latest_unit_height(height);
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
    const int32_t begin_size = stream.size();
    stream.write_compact_var(m_latest_unit_height);
    stream.write_compact_var(m_latest_unit_hash);
    stream.write_compact_var(m_account_flag);
    return (stream.size() - begin_size);
}

int32_t xaccount_index_t::do_read(base::xstream_t & stream) {
    const int32_t begin_size = stream.size();
    stream.read_compact_var(m_latest_unit_height);
    stream.read_compact_var(m_latest_unit_hash);
    stream.read_compact_var(m_account_flag);
    return (begin_size - stream.size());
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
    uint16_t hash16 = static_cast<uint16_t>(base::xhash32_t::digest(hash) & 0xFFFF);
    m_latest_unit_hash = hash16;
}

bool xaccount_index_t::is_match_unit_hash(const std::string & hash) const {
    uint16_t hash16 = static_cast<uint16_t>(base::xhash32_t::digest(hash) & 0xFFFF);
    return hash16 == m_latest_unit_hash;
}

std::string xaccount_index_t::dump() const {
    char local_param_buf[128];
    xprintf(local_param_buf,sizeof(local_param_buf),"{height=%" PRIu64 ",hash=%u,flag=0x%x}",
        m_latest_unit_height, m_latest_unit_hash, m_account_flag);
    return std::string(local_param_buf);
}

xtable_mbt_binlog_t::xtable_mbt_binlog_t()
:base::xdataunit_t(base::xdataunit_t::enum_xdata_type_undefine) {

}
xtable_mbt_binlog_t::xtable_mbt_binlog_t(const std::map<std::string, xaccount_index_t> & changed_indexs)
:base::xdataunit_t(base::xdataunit_t::enum_xdata_type_undefine) {
    m_account_indexs = changed_indexs;
}

int32_t xtable_mbt_binlog_t::do_write(base::xstream_t & stream) {
    const int32_t begin_size = stream.size();
    const uint32_t count = (uint32_t)m_account_indexs.size();
    xassert(count != 0);
    stream.write_compact_var(count);
    for (auto & v : m_account_indexs) {
        stream.write_compact_var(v.first);
        v.second.do_write(stream);
    }
    return (stream.size() - begin_size);
}
int32_t xtable_mbt_binlog_t::do_read(base::xstream_t & stream) {
    const int32_t begin_size = stream.size();
    uint32_t count;
    stream.read_compact_var(count);
    for (uint32_t i = 0; i < count; i++) {
        std::string account;
        stream.read_compact_var(account);
        xaccount_index_t index;
        index.do_read(stream);
        m_account_indexs[account] = index;
    }
    xassert(m_account_indexs.size() == count);
    return (begin_size - stream.size());
}

void xtable_mbt_binlog_t::set_account_index(const std::string & account, const xaccount_index_t & index) {
#ifdef DEBUG
    auto iter = m_account_indexs.find(account);
    if (iter != m_account_indexs.end()) {
        auto & old_index = iter->second;
        if (old_index.get_latest_unit_height() > index.get_latest_unit_height()) {
            xassert(false);
            return;
        }
    }
#endif
    m_account_indexs[account] = index;
}

void xtable_mbt_binlog_t::set_accounts_index(const std::map<std::string, xaccount_index_t> & changed_indexs) {
    for (auto & v : changed_indexs) {
        set_account_index(v.first, v.second);
    }
}

bool xtable_mbt_binlog_t::get_account_index(const std::string & account, xaccount_index_t & index) const {
    auto iter = m_account_indexs.find(account);
    if (iter != m_account_indexs.end()) {
        index = iter->second;
        return true;
    }
    return false;
}

std::string xtable_mbt_binlog_t::build_binlog_hash() {
    // base::xstream_t stream(top::base::xcontext_t::instance());
    // int32_t size = do_write(stream);
    // xassert(size > 0);
    // uint256_t hash256 = utl::xsha2_256_t::digest((const char*)stream.data(), stream.size());
    // std::string hash = std::string(reinterpret_cast<char*>(hash256.data()), hash256.size());
    // return hash;
    // TODO(jimmy)
    std::string bin_str;
    serialize_to_string(bin_str);
    return xcontext_t::instance().hash(bin_str, enum_xhash_type_sha2_256);
}


xtable_mbt_bucket_node_t::xtable_mbt_bucket_node_t()
:base::xdataunit_t(base::xdataunit_t::enum_xdata_type_undefine) {
}

int32_t xtable_mbt_bucket_node_t::do_write_without_hash(base::xstream_t & stream) const {
    const int32_t begin_size = stream.size();
    const uint32_t count = (uint32_t)m_account_indexs.size();
    xassert(count != 0);
    stream.write_compact_var(count);
    for (auto & v : m_account_indexs) {
        stream.write_compact_var(v.first);
        v.second.do_write(stream);
    }
    return (stream.size() - begin_size);
}

int32_t xtable_mbt_bucket_node_t::do_write(base::xstream_t & stream) {
    const int32_t begin_size = stream.size();
    do_write_without_hash(stream);
    return (stream.size() - begin_size);
}
int32_t xtable_mbt_bucket_node_t::do_read(base::xstream_t & stream) {
    const int32_t begin_size = stream.size();
    uint32_t count;
    stream.read_compact_var(count);
    for (uint32_t i = 0; i < count; i++) {
        std::string account;
        stream.read_compact_var(account);
        xaccount_index_t index;
        index.do_read(stream);
        m_account_indexs[account] = index;
    }
    xassert(m_account_indexs.size() == count);
    return (begin_size - stream.size());
}

void    xtable_mbt_bucket_node_t::set_account_index(const std::string & account, xaccount_index_t index) {
#ifdef DEBUG
    auto iter = m_account_indexs.find(account);
    if (iter != m_account_indexs.end()) {
        auto & old_index = iter->second;
        if (old_index.get_latest_unit_height() > index.get_latest_unit_height()) {
            xassert(false);
            return;
        }
    }
#endif
    m_account_indexs[account] = index;
}
bool    xtable_mbt_bucket_node_t::get_account_index(const std::string & account, xaccount_index_t & index) const {
    auto iter = m_account_indexs.find(account);
    if (iter != m_account_indexs.end()) {
        index = iter->second;
        return true;
    }
    return false;
}

std::string xtable_mbt_bucket_node_t::build_bucket_hash() {
    return calc_bucket_hash();
}
std::string xtable_mbt_bucket_node_t::calc_bucket_hash() const {
    base::xstream_t stream(top::base::xcontext_t::instance());
    int32_t size = do_write_without_hash(stream);
    xassert(size > 0);
    uint64_t hash = base::xhash64_t::digest((const char*)stream.data(), stream.size());
    std::string bucket_hash = std::to_string(hash);
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
    const int32_t begin_size = stream.size();
    stream.write_compact_var(m_root_hash);
    stream.write_compact_map(m_bucket_hashs);
    return (stream.size() - begin_size);
}
int32_t xtable_mbt_root_node_t::do_read(base::xstream_t & stream) {
    const int32_t begin_size = stream.size();
    stream.read_compact_var(m_root_hash);
    stream.read_compact_map(m_bucket_hashs);
    return (begin_size - stream.size());
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
    stream.write_compact_map(m_bucket_hashs);
    std::string bin_str((char*)stream.data(), stream.size());
    // uint256_t hash256 = utl::xsha2_256_t::digest((const char*)stream.data(), stream.size());
    // std::string root_hash = std::string(reinterpret_cast<char*>(hash256.data()), hash256.size());
    // return root_hash;
    // TODO(jimmy)
    return xcontext_t::instance().hash(bin_str, enum_xhash_type_sha2_256);
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

void xtable_mbt_t::set_account_index(const std::string & account, const xaccount_index_t & info) {
    uint16_t account_index = account_to_index(account);
    auto iter = m_buckets.find(account_index);
    if (iter != m_buckets.end()) {
        auto & bucket = iter->second;
        bucket->set_account_index(account, info);
    } else {
        xtable_mbt_bucket_node_ptr_t bucket = make_object_ptr<xtable_mbt_bucket_node_t>();
        m_buckets[account_index] = bucket;
        bucket->set_account_index(account, info);
    }
}
void xtable_mbt_t::set_accounts_index_info(const std::map<std::string, xaccount_index_t> & accounts_info, bool is_build_root_hash) {
    xassert(accounts_info.size() > 0);
    for (auto & v : accounts_info) {
        set_account_index(v.first, v.second);
    }
    // calc tree root hash
    if (is_build_root_hash) {
        build_root_hash();
    }
}

int32_t xtable_mbt_t::do_write(base::xstream_t & stream) {
    const int32_t begin_size = stream.size();
    stream.write_compact_var(m_root_hash);
    const uint32_t count = (uint32_t)m_buckets.size();
    stream.write_compact_var(count);
    for (auto & bucket : m_buckets) {
        stream.write_compact_var(bucket.first);
        bucket.second->serialize_to(stream);
    }
    return (stream.size() - begin_size);
}
int32_t xtable_mbt_t::do_read(base::xstream_t & stream) {
    const int32_t begin_size = stream.size();
    stream.read_compact_var(m_root_hash);
    uint32_t count;
    stream.read_compact_var(count);
    for (uint32_t i = 0; i < count; i++) {
        uint16_t key;
        stream.read_compact_var(key);
        xtable_mbt_bucket_node_ptr_t value = make_object_ptr<xtable_mbt_bucket_node_t>();
        value->serialize_from(stream);
        m_buckets[key] = value;
    }
    return (begin_size - stream.size());
}

xtable_mbt_root_node_ptr_t xtable_mbt_t::get_root_node() const {
    std::map<uint16_t, std::string> bucket_hashs;
    for (auto & bucket : m_buckets) {
        bucket_hashs[bucket.first] = bucket.second->build_bucket_hash();
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
    xtable_mbt_root_node_ptr_t root_node = get_root_node();
    return root_node->check_root_node();
}

bool xtable_mbt_t::get_account_index(const std::string & account, xaccount_index_t & index) const {
    uint16_t account_index = account_to_index(account);
    auto iter = m_buckets.find(account_index);
    if (iter != m_buckets.end()) {
        const xtable_mbt_bucket_node_ptr_t & bucket = iter->second;
        return bucket->get_account_index(account, index);
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

xtable_mbt_new_state_t::xtable_mbt_new_state_t() {
    m_last_full_state = make_object_ptr<xtable_mbt_t>();
    m_newest_binlog_state = make_object_ptr<xtable_mbt_binlog_t>();
}

bool xtable_mbt_new_state_t::get_account_index(const std::string & account, xaccount_index_t & account_index) {
    // firstly, cache index binlog and try to find account index from binlog
    if (m_newest_binlog_state->get_account_index(account, account_index)) {
        xdbg("JIMMY xtable_mbt_new_state_t::get_account_index binlog account=%s,height=%ld,state_height=%ld",
            account.c_str(), account_index.get_latest_unit_height(), m_newest_binlog_state->get_height());
        return true;
    }
    // secondly, cache last full index and try to find accout index from last full index
    m_last_full_state->get_account_index(account, account_index);
    xdbg("JIMMY xtable_mbt_new_state_t::get_account_index state account=%s,height=%ld,state_height=%ld",
        account.c_str(), account_index.get_latest_unit_height(), m_last_full_state->get_height());
    return true;
}

NS_END2
