// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbasic/xmemory.hpp"
#include "xbasic/xlru_cache_specialize.h"
#include "xbasic/xthreading/xdummy_mutex.h"
#include "xdata/xunit_bstate.h"

NS_BEG2(top, statestore)

class xaccount_state_cache_face_t {
public:
    virtual xobject_ptr_t<base::xvbstate_t> get_bstate(uint64_t height, std::string const& unit_hash) const = 0;
    virtual void set_bstate(std::string const& unit_hash, xobject_ptr_t<base::xvbstate_t> const& bstate) = 0;  
};
using xaccount_state_cache_face_ptr_t = std::shared_ptr<xaccount_state_cache_face_t>;

class xaccount_state_cache_t : public xaccount_state_cache_face_t {
public:    
    xaccount_state_cache_t(std::string const& unit_hash, xobject_ptr_t<base::xvbstate_t> const& bstate);
    ~xaccount_state_cache_t();

    xobject_ptr_t<base::xvbstate_t> get_bstate(uint64_t height, std::string const& unit_hash) const override;
    void set_bstate(std::string const& unit_hash, xobject_ptr_t<base::xvbstate_t> const& bstate) override;

private:
    std::string m_unit_hash;
    xobject_ptr_t<base::xvbstate_t> m_bstate{nullptr};
};

class xcontract_state_cache_t : public xaccount_state_cache_face_t {
public:    
    enum
    {
        enum_max_state_count    = 5,
    };
    xcontract_state_cache_t(std::string const& unit_hash, xobject_ptr_t<base::xvbstate_t> const& bstate);
    ~xcontract_state_cache_t();

    xobject_ptr_t<base::xvbstate_t> get_bstate(uint64_t height, std::string const& unit_hash) const override;
    void set_bstate(std::string const& unit_hash, xobject_ptr_t<base::xvbstate_t> const& bstate) override;

private:
    std::map<uint64_t, std::pair<std::string, xobject_ptr_t<base::xvbstate_t>>> m_states;
};

class xunitstate_cache_t {
public:
    enum
    {
        enum_max_unit_state_lru_cache_shard    = 700,
        enum_max_unit_state_lru_cache_evm      = 40000,
    };
public:
    xunitstate_cache_t(uint32_t cache_size);

    xobject_ptr_t<base::xvbstate_t>  get_unitstate(std::string const& address, uint64_t height, std::string const& unit_hash) const;
    void set_unitstate(std::string const& unit_hash, xobject_ptr_t<base::xvbstate_t> const& bstate);
    void clear();
    size_t size() const;

private:
    mutable basic::xlru_cache_t<std::string, xaccount_state_cache_face_ptr_t, threading::xdummy_mutex_t> m_unitstate_cache;
    mutable std::mutex m_mutex;
};
using xunitstate_cache_ptr_t = std::shared_ptr<xunitstate_cache_t>;

NS_END2