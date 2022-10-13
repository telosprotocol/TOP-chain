// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbasic/xmemory.hpp"
#include "xstatestore/xstatestore_base.h"
#include "xstatestore/xtablestate_ext.h"
#include "xdata/xtable_bstate.h"
#include "xdata/xunit_bstate.h"

NS_BEG2(top, statestore)

class xstatestore_cache_t {
protected:
    enum
    {
        enum_max_table_state_lru_cache_max     = 8, //max table state lru cache count
        enum_max_unit_state_lru_cache_max      = 32, //max unit state lru cache count
    };
public:
    xstatestore_cache_t();
public:
    xtablestate_ext_ptr_t   get_tablestate(std::string const& block_hash) const;
    data::xunitstate_ptr_t  get_unitstate(std::string const& block_hash) const;
    xtablestate_ext_ptr_t   get_latest_connectted_tablestate() const;

public:
    void    set_latest_connectted_tablestate(xtablestate_ext_ptr_t const& tablestate) const;
    void    set_tablestate(std::string const& block_hash, xtablestate_ext_ptr_t const& tablestate) const;
    void    set_unitstate(std::string const& block_hash, data::xunitstate_ptr_t const& unitstate) const;

private:
   // TODO(jimmy) it is better to use non-lock cache
   mutable std::mutex m_mutex;
   mutable xtablestate_ext_ptr_t    m_latest_connectted_tablestate{nullptr};
   mutable base::xlru_cache<std::string, xtablestate_ext_ptr_t> m_tablestate_cache;  //tablestate cache
   mutable base::xlru_cache<std::string, data::xunitstate_ptr_t> m_unitstate_cache;  //unitstate cache
};


class xstatestore_dbaccess_t {
 public:
    void    write_table_bstate(common::xaccount_address_t const& address, data::xtablestate_ptr_t const& tablestate, const std::string & block_hash, std::error_code & ec) const;
    void    write_unit_bstate(data::xunitstate_ptr_t const& unitstate, const std::string & block_hash, std::error_code & ec) const;

 public:
    data::xtablestate_ptr_t     read_table_bstate(common::xaccount_address_t const& address, uint64_t height, const std::string & block_hash) const;
    data::xunitstate_ptr_t      read_unit_bstate(common::xaccount_address_t const& address, uint64_t height, const std::string & block_hash) const;

 private:
    xstatestore_base_t          m_statestore_base;
};


// access state from cache or db
class xstatestore_accessor_t {
 public:
    xstatestore_accessor_t(common::xaccount_address_t const& address);
public:
    xtablestate_ext_ptr_t       get_latest_connectted_table_state() const;
    xtablestate_ext_ptr_t       read_table_bstate_from_cache(common::xaccount_address_t const& address, uint64_t height, const std::string & block_hash) const;
    xtablestate_ext_ptr_t       read_table_bstate_from_db(common::xaccount_address_t const& address, base::xvblock_t* block) const;
    xtablestate_ext_ptr_t       read_table_bstate(common::xaccount_address_t const& address, base::xvblock_t* block) const;
    data::xunitstate_ptr_t      read_unit_bstate(common::xaccount_address_t const& address, uint64_t height, const std::string & block_hash) const;

    void    set_latest_connectted_tablestate(xtablestate_ext_ptr_t const& tablestate) const;
    void    write_table_bstate_to_db(common::xaccount_address_t const& address, std::string const& block_hash, data::xtablestate_ptr_t const& tablestate, std::error_code & ec) const;
    void    write_table_bstate_to_cache(common::xaccount_address_t const& address, std::string const& block_hash, xtablestate_ext_ptr_t const& state) const;
    void    write_unitstate_to_db(data::xunitstate_ptr_t const& unitstate, const std::string & block_hash, std::error_code & e) const;
    void    write_unitstate_to_cache(data::xunitstate_ptr_t const& unitstate, const std::string & block_hash) const;

private:
    common::xaccount_address_t  m_table_addr;
    xstatestore_cache_t         m_state_cache;
    xstatestore_dbaccess_t      m_dbaccess;
    xstatestore_base_t          m_store_base;
};

NS_END2
