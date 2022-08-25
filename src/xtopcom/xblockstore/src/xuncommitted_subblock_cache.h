// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbkstoreutl.h"

#include <map>
#include <string>
#include <vector>

namespace top {
namespace store {
class xblock_match_base_t {
public:
    virtual bool is_match(base::xvbindex_t * block) const = 0;
};

class xblock_match_unconditional_t : public xblock_match_base_t {
public:
    virtual bool is_match(base::xvbindex_t * block) const override {
        return true;
    }
};

class xblock_match_by_height_t : public xblock_match_base_t {
public:
    xblock_match_by_height_t(uint64_t height) : m_height(height) {
    }

public:
    virtual bool is_match(base::xvbindex_t * block) const override {
        return (block->get_height() == m_height);
    }

private:
    uint64_t m_height;
};

class xblock_match_by_height_viewid_t : public xblock_match_base_t {
public:
    xblock_match_by_height_viewid_t(uint64_t height, uint64_t viewid) : m_height(height), m_viewid(viewid) {
    }

public:
    virtual bool is_match(base::xvbindex_t * block) const override {
        return (block->get_height() == m_height && block->get_viewid() == m_viewid);
    }

private:
    uint64_t m_height;
    uint64_t m_viewid;
};

class xblock_match_by_height_hash_t : public xblock_match_base_t {
public:
    xblock_match_by_height_hash_t(uint64_t height, const std::string & hash) : m_height(height), m_hash(hash) {
    }

public:
    virtual bool is_match(base::xvbindex_t * block) const override {
        return (block->get_height() == m_height && block->get_block_hash() == m_hash);
    }

private:
    uint64_t m_height;
    std::string m_hash;
};

class xuncommitted_subblock_cache_t {
public:
    bool update_height(uint64_t height);
    const std::map<uint64_t, std::map<std::string, base::xvbindex_t *>> & get_lock_cache() const;
    const std::map<uint64_t, std::map<std::string, base::xvbindex_t *>> & get_cert_cache() const;
    void add_blocks(const std::vector<base::xvblock_ptr_t> & cert_blocks, const std::vector<base::xvblock_ptr_t> & lock_blocks);
    base::xblock_vector load_blocks_object(const base::xvaccount_t & account, uint64_t height) const;
    base::xvbindex_vector load_blocks_index(const base::xvaccount_t & account, const uint64_t height) const;
    base::xauto_ptr<base::xvbindex_t> load_block_index(const base::xvaccount_t & account, const xblock_match_base_t & match_func) const;
    base::xauto_ptr<base::xvblock_t> load_block_object(const base::xvaccount_t & account, const xblock_match_base_t & match_func) const;

private:
    void add_blocks_to_cache(const std::vector<base::xvblock_ptr_t> & blocks, std::map<uint64_t, std::map<std::string, base::xvbindex_t *>> & block_cache);
    std::vector<base::xvbindex_t *> get_block_indexs_from_cache(const std::string & account,
                                                                const std::map<uint64_t, std::map<std::string, base::xvbindex_t *>> & block_cache,
                                                                uint64_t height) const;
    base::xvbindex_t * get_block_index_from_cache(const std::string & account,
                                                  const std::map<uint64_t, std::map<std::string, base::xvbindex_t *>> & block_cache,
                                                  const xblock_match_base_t & match_func) const;

private:
    std::map<uint64_t, std::map<std::string, base::xvbindex_t *>> m_lock_cache;  // < view#, <account#,block*> > sort from lower to higher
    std::map<uint64_t, std::map<std::string, base::xvbindex_t *>> m_cert_cache;  // < view#, <account#,block*> > sort from lower to higher
    uint64_t m_cert_height{0};
};

};  // namespace store
};  // namespace top
