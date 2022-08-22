// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

#include "xbase/xlru_cache.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include "xbase/xns_macro.h"
#include "xcommon/xaccount_address.h"
#include "xevm_contract_runtime/xevm_sys_contract_face.h"
#include "xstatectx/xstatectx_face.h"

#include <memory>
#include <unordered_map>

NS_BEG3(top, contract_runtime, evm)

class xtop_evm_contract_manager {
public:
    xtop_evm_contract_manager();
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

    const std::unordered_map<common::xaccount_address_t, std::unique_ptr<xevm_syscontract_face_t>> & get_sys_contracts() const;

    void add_sys_contract(common::xaccount_address_t const & contract_address, std::unique_ptr<xevm_syscontract_face_t> contract);

    bool execute_sys_contract(xbytes_t const & input, observer_ptr<statectx::xstatectx_face_t> state_ctx, xbytes_t & output);

private:
    enum {
        enum_default_code_cache_max = 256,
    };
    base::xlru_cache<common::xaccount_address_t, xbytes_t> m_code_cache{enum_default_code_cache_max};

    std::unordered_map<common::xaccount_address_t, std::unique_ptr<xevm_syscontract_face_t>> m_sys_contract;
};
using xevm_contract_manager_t = xtop_evm_contract_manager;

NS_END3
