// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xevm_contract_runtime/xevm_storage_base.h"
#include "xcommon/xtoken_metadata.h"

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

#include "xstatectx/xstatectx_face.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

NS_BEG3(top, contract_runtime, evm)

class xtop_evm_storage : public xevm_storage_base_t {
public:
    explicit xtop_evm_storage(statectx::xstatectx_face_ptr_t const statectx, const std::string &token_type) : m_statectx{statectx} {
        common::xsymbol_t _symbol(token_type);
        m_token_id = common::token_id(_symbol);
    } 
    xtop_evm_storage(xtop_evm_storage const &) = delete;
    xtop_evm_storage & operator=(xtop_evm_storage const &) = delete;
    xtop_evm_storage(xtop_evm_storage &&) = default;
    xtop_evm_storage & operator=(xtop_evm_storage &&) = default;
    ~xtop_evm_storage() = default;

    xbytes_t storage_get(xbytes_t const & key) override;
    void storage_set(xbytes_t const & key, xbytes_t const & value) override;
    void storage_remove(xbytes_t const & key) override;

private:
    statectx::xstatectx_face_ptr_t m_statectx;
    common::xtoken_id_t            m_token_id;
};
using xevm_storage = xtop_evm_storage;

NS_END3
