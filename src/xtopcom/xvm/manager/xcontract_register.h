// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <unordered_map>
#include "xbase/xns_macro.h"
#include "xbase/xlock.h"
#include "xvm/xcontract/xcontract_base.h"

NS_BEG2(top, contract)

using namespace top::xvm::xcontract;

class xcontract_register_t {
public:
    /**
     * @brief Destroy the xcontract register t object
     *
     */
    virtual ~xcontract_register_t() {
        for(auto& pair : m_map) {
            delete pair.second;
        }
        m_map.clear();
    }

    /**
     * @brief add the contract instance object
     *
     * @tparam T
     * @param contract_addr
     * @param network_id
     */
    template<typename T>
    void add(common::xaccount_address_t const & contract_addr, common::xnetwork_id_t const & network_id) {
        static_assert(std::is_base_of<xcontract_base, T>::value, "must subclass of xcontract_base");

        m_rwlock.lock_write();
        m_map[contract_addr]= new T{ network_id };
        m_rwlock.release_write();
    }

    /**
     * @brief Get the contract object
     *
     * @param contract_addr
     * @return xcontract_base*
     */
    xcontract_base* get_contract(common::xaccount_address_t const & contract_addr) {
        xcontract_base* contract{};
        m_rwlock.lock_read();
        auto it = m_map.find(contract_addr);
        if (it != m_map.end()) {
            contract = it->second;
        }
        m_rwlock.release_read();
        return contract;
    }

protected:
    base::xrwlock_t m_rwlock;
    std::unordered_map<common::xaccount_address_t, xcontract_base*> m_map; // key is the contract address
};

NS_END2
