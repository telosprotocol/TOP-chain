// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbasic/xmemory.hpp"
#include "xbasic/xlru_cache_specialize.h"
#include "xstatestore/xstatestore_base.h"
#include "xstatestore/xtablestate_ext.h"
#include "xstatestore/xstatehub.h"
#include "xdata/xtable_bstate.h"
#include "xdata/xunit_bstate.h"

NS_BEG2(top, statestore)


class xunitstate_cache_base_t {
public:
    virtual data::xunitstate_ptr_t  get_unitstate(std::string const& account, uint64_t height, std::string const& block_hash) const = 0;
    virtual void set_unitstate(std::string const& block_hash, data::xunitstate_ptr_t const& unitstate) = 0;
};

class xcontract_unitstates_t {
    enum {
        enum_keep_num = 4,
    };
public:
    xcontract_unitstates_t(std::string const& block_hash, data::xunitstate_ptr_t const& unitstate);
    data::xunitstate_ptr_t  get_unitstate(uint64_t height, std::string const& block_hash) const;
    void set_unitstate(std::string const& block_hash, data::xunitstate_ptr_t const& unitstate);
private:
    mutable std::map<uint64_t, std::map<std::string, xobject_ptr_t<base::xvbstate_t>>> m_uintstates;
};


class xunitstate_cache_contract_t : public xunitstate_cache_base_t {
public:
    virtual data::xunitstate_ptr_t  get_unitstate(std::string const& account, uint64_t height, std::string const& block_hash) const override;
    virtual void set_unitstate(std::string const& block_hash, data::xunitstate_ptr_t const& unitstate) override;
private:
    mutable std::map<std::string, xcontract_unitstates_t> m_account_uintstates_map;
};

class xunitstate_cache_normal_t : public xunitstate_cache_base_t {
public:
    xunitstate_cache_normal_t(size_t lru_size);
    virtual data::xunitstate_ptr_t  get_unitstate(std::string const& account, uint64_t height, std::string const& block_hash) const override;
    virtual void set_unitstate(std::string const& block_hash, data::xunitstate_ptr_t const& unitstate) override;
private:
    mutable basic::xlru_cache_t<std::string, std::pair<std::string, xobject_ptr_t<base::xvbstate_t>>, threading::xdummy_mutex_t> m_unitstate_lru;
};

class xstatestore_cache_t {
protected:
    enum {
        enum_max_unit_state_lru_cache_normal = 2000,
        enum_max_unit_state_lru_cache_evm = 30000,
    };

public:
    xstatestore_cache_t(base::enum_xchain_zone_index zone_idx);
public:
    data::xunitstate_ptr_t  get_unitstate(std::string const& account, uint64_t height, std::string const& block_hash) const;
    xtablestate_ext_ptr_t const&    get_latest_connectted_tablestate() const;
    xtablestate_ext_ptr_t   get_tablestate(uint64_t height, std::string const& block_hash) const;

public:
    void    set_unitstate(std::string const& block_hash, data::xunitstate_ptr_t const& unitstate);    
    void    set_tablestate(uint64_t height, std::string const& block_hash, xtablestate_ext_ptr_t const& tablestate, bool is_commit);
    void    set_latest_connected_tablestate(uint64_t height, xtablestate_ext_ptr_t const& tablestate);
    xtablestate_ext_ptr_t   get_prev_tablestate(uint64_t height, std::string const& block_hash) const;
private:
    xtablestate_ext_ptr_t   get_tablestate_inner(uint64_t height, std::string const& block_hash) const;

    xtablestate_ext_ptr_t    m_latest_connectted_tablestate{nullptr};
    std::shared_ptr<xunitstate_cache_base_t> m_unitstate_cache{nullptr};
    mutable std::mutex m_unitstates_cache_mutex;
    std::map<uint64_t, std::map<std::string, xtablestate_ext_ptr_t>>  m_table_states;
};

class xstatestore_dbaccess_t {
 public:
    void    write_table_bstate(common::xtable_address_t const& address, data::xtablestate_ptr_t const& tablestate, const std::string & block_hash, std::error_code & ec) const;
    void    write_unit_bstate(data::xunitstate_ptr_t const& unitstate, const std::string & block_hash, std::error_code & ec) const;
    void    unit_bstate_to_kv(data::xunitstate_ptr_t const& unitstate, const std::string & block_hash, std::map<std::string, std::string> & batch_kvs, bool store_pruneable_kv, std::error_code & ec) const;
    void    batch_write_unit_bstate(const std::map<std::string, std::string> & batch_kvs, std::error_code & ec) const;

 public:
    data::xtablestate_ptr_t     read_table_bstate(common::xtable_address_t const& address, uint64_t height, const std::string & block_hash) const;
    data::xunitstate_ptr_t      read_unit_bstate(common::xaccount_address_t const& address, uint64_t height, const std::string & block_hash) const;

 private:
    xstatestore_base_t          m_statestore_base;
};


// access state from cache or db
class xstatestore_accessor_t {
 public:
    xstatestore_accessor_t(common::xtable_address_t const& address);
public:
    xtablestate_ext_ptr_t       get_latest_connectted_table_state() const;
    xtablestate_ext_ptr_t       read_table_bstate_from_cache(common::xtable_address_t const& address, uint64_t height, const std::string & block_hash) const;
    xtablestate_ext_ptr_t       read_table_bstate_from_db(common::xtable_address_t const& address, base::xvblock_t* block) const;
    xtablestate_ext_ptr_t       read_table_bstate(common::xtable_address_t const& address, base::xvblock_t* block) const;
    xtablestate_ext_ptr_t       read_table_bstate_for_account_index(common::xtable_address_t const& address, base::xvblock_t* block) const;
    data::xunitstate_ptr_t      read_unit_bstate(common::xaccount_address_t const& address, uint64_t height, const std::string & block_hash) const;

    void    write_table_bstate_to_db(common::xtable_address_t const& address, std::string const& block_hash, data::xtablestate_ptr_t const& tablestate, std::error_code & ec);
    void    write_table_bstate_to_cache(common::xtable_address_t const& address, uint64_t height, std::string const& block_hash, xtablestate_ext_ptr_t const& state, bool is_commit);
    void    write_unitstate_to_db(data::xunitstate_ptr_t const& unitstate, const std::string & block_hash, std::error_code & ec);
    void    write_unitstate_to_cache(data::xunitstate_ptr_t const& unitstate, const std::string & block_hash);
    void    unit_bstate_to_kv(data::xunitstate_ptr_t const& unitstate, const std::string & block_hash, std::map<std::string, std::string> & batch_kvs, bool store_pruneable_kv, std::error_code & ec) const;
    void    batch_write_unit_bstate(const std::map<std::string, std::string> & batch_kvs, std::error_code & ec) const;
private:
    xtablestate_ext_ptr_t read_table_bstate_from_db_inner(common::xtable_address_t const& address, base::xvblock_t* block, bool bstate_must) const;
private:
    xstatestore_cache_t         m_state_cache;
    xvstatehub_ptr_t            m_statehub;
    xstatestore_dbaccess_t      m_dbaccess;
    xstatestore_base_t          m_store_base;
};

NS_END2
