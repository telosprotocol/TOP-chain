// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xlru_cache.h"
#include "xbase/xns_macro.h"

NS_BEG3(top, contract_runtime, evm)

class xtop_evm_contract_manager {
public:
    xtop_evm_contract_manager() = default;
    xtop_evm_contract_manager(xtop_evm_contract_manager const &) = delete;
    xtop_evm_contract_manager & operator=(xtop_evm_contract_manager const &) = delete;
    xtop_evm_contract_manager(xtop_evm_contract_manager &&) = default;
    xtop_evm_contract_manager & operator=(xtop_evm_contract_manager &&) = default;
    ~xtop_evm_contract_manager() = default;

    /**
     * @brief get an instance
     *
     * @return xtop_evm_contract_manager&
     */
    static xtop_evm_contract_manager * instance() {
        static auto * inst = new xtop_evm_contract_manager();
        return inst;
    }

    xbytes_t code(common::xaccount_address_t const & account) {
        xbytes_t code;
        m_code_cache.get(account, code);
        return code;
    }

    void set_code(common::xaccount_address_t const & account, xbytes_t const & code) {
        m_code_cache.put(account, code);
    }

private:
    enum {
        enum_default_code_cache_max = 256,
    };
    base::xlru_cache<common::xaccount_address_t, xbytes_t> m_code_cache{enum_default_code_cache_max};
};
using xevm_contract_manager_t = xtop_evm_contract_manager;

NS_END3