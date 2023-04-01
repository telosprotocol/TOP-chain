// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xcommon/xaccount_address.h"
#include "xdata/xtable_bstate.h"
#include "xstatestore/xstatestore_base.h"
#include "xstatestore/xstatestore_resource.h"

#include <string>

NS_BEG2(top, statestore)

class xaccounts_prune_info_t {
public:
    void insert_from_tableblock(base::xvblock_t * table_block);
    const std::map<std::string, uint64_t> & get_prune_info() const;

private:
    void get_account_indexs(base::xvblock_t * table_block, base::xaccount_indexs_t & account_indexs) const;
private:
    std::map<std::string, uint64_t> m_prune_info;   // key:account, value:max delete unit height
};

class xtablestate_and_offdata_prune_info_t {
public:
    void insert_from_tableblock(base::xvblock_t * table_block);
    const std::vector<std::string> & get_tablestate_prune_keys() const;

private:
    std::vector<std::string> m_tablestate_keys;
};

class xstatestore_prune_t : public std::enable_shared_from_this<xstatestore_prune_t> {
public:
    xstatestore_prune_t(common::xtable_address_t const & table_addr, std::shared_ptr<xstatestore_resources_t> para);

public:
    void on_table_block_executed(uint64_t exec_height);

// protected:
private:
    void prune_imp(uint64_t exec_height);
    void unitstate_prune_batch(const xaccounts_prune_info_t & accounts_prune_info);
    common::xtable_address_t const & get_account() const {return m_table_addr;}
    bool need_prune(uint64_t exec_height);
    bool get_prune_section(uint64_t exec_height, uint64_t & from_height, uint64_t & to_height, uint64_t & lowest_keep_height);
    void set_pruned_height(uint64_t pruned_height);

private:
    void init();
    // virtual uint64_t prune_exec(uint64_t from_height, uint64_t to_height) = 0;
    uint64_t prune_exec_storage(uint64_t from_height, uint64_t to_height);
    uint64_t prune_exec_storage_and_cons(uint64_t from_height, uint64_t to_height);
    uint64_t prune_exec_cons(uint64_t from_height, uint64_t to_height, uint64_t exec_height, uint64_t lowest_keep_height);

private:
    mutable std::mutex m_prune_lock;
    common::xtable_address_t m_table_addr;
    base::xvaccount_t           m_table_vaddr; // TODO(jimmy) refactor
    uint64_t m_pruned_height{0};
    xstatestore_base_t m_statestore_base;
    std::shared_ptr<xstatestore_resources_t> m_para;
};

// class xstatestore_prune_archive_t : public xstatestore_prune_t {
// public:
//     xstatestore_prune_archive_t(common::xaccount_address_t const & table_addr) : xstatestore_prune_t(table_addr) {
// }

// private:
//     virtual uint64_t prune_exec(uint64_t from_height, uint64_t to_height) override;
// };

// class xstatestore_prune_archive_cons_t : public xstatestore_prune_t {
// public:
//     xstatestore_prune_archive_cons_t(common::xaccount_address_t const & table_addr) : xstatestore_prune_t(table_addr) {
// }

// private:
//     virtual uint64_t prune_exec(uint64_t from_height, uint64_t to_height) override;
// };

// class xstatestore_prune_cons_t : public xstatestore_prune_t {
// public:
//     xstatestore_prune_cons_t(common::xaccount_address_t const & table_addr) : xstatestore_prune_t(table_addr) {
// }

// private:
//     virtual uint64_t prune_exec(uint64_t from_height, uint64_t to_height) override;
// };

NS_END2
