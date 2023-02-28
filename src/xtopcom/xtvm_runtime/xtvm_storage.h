// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xhex.h"
#include "xcommon/xaccount_address.h"
#include "xstatectx/xstatectx_face.h"

#include <cassert>

NS_BEG2(top, tvm)
class xtop_vm_storage {
private:
    // storage_ctx
    statectx::xstatectx_face_ptr_t m_statectx;

public:
    // constructor:
    xtop_vm_storage(statectx::xstatectx_face_ptr_t const statectx) : m_statectx{statectx} {
    }

public:
    xbytes_t storage_get(xbytes_t const & key);
    void storage_set(xbytes_t const & key, xbytes_t const & value);
    void storage_remove(xbytes_t const & key);

private:
    enum class storage_key_prefix : uint8_t {
        Nonce = 1,
        Balance = 2,
        Code = 3,
        Storage = 4,
    };

    class xtop_vm_storage_key {
        friend class xtop_vm_storage;

    private:
        storage_key_prefix m_key_type;
        xbytes_t m_address_bytes;
        xbytes_t m_storage_key;  // optional
        bool m_has_optional_storage_key{false};

    private:
        // can only be constructed from method `decode_storage_key`
        xtop_vm_storage_key() {
        }

    public:
        inline storage_key_prefix key_type() const {
            return m_key_type;
        }

        inline bool is_key_type(storage_key_prefix const & key_prefix) const {
            return key_type() == key_prefix;
        }

        inline bool has_storage_key() const {
            return m_has_optional_storage_key;
        }

        std::string storage_key() const {
            assert(m_has_optional_storage_key);
            return top::to_string(m_storage_key);
        }

        common::xaccount_address_t t8_address() const {
            return common::xaccount_address_t::build_from(top::to_hex(m_address_bytes.begin(), m_address_bytes.end(), "T80000"));
        }
    };
    using xtvm_storage_key_t = xtop_vm_storage_key;

protected:
    xtvm_storage_key_t decode_storage_key(xbytes_t const & storage_key) {
        assert(storage_key.size() == 22 || storage_key.size() == 54);
        xtvm_storage_key_t result;
        assert(storage_key.at(0) == 1);  // VERSION::V1 = 0x01 as u8;
        result.m_key_type = static_cast<storage_key_prefix>(storage_key.at(1));
        result.m_address_bytes.resize(20, xbyte_t{0});
        std::copy(storage_key.begin() + 2, storage_key.begin() + 22, result.m_address_bytes.begin());
        if (storage_key.size() == 54) {
            result.m_has_optional_storage_key = true;
            result.m_storage_key.resize(32, xbyte_t{0});
            std::copy(storage_key.begin() + 22, storage_key.end(), result.m_storage_key.begin());
        }
        return result;
    }
};
using xtvm_storage_t = xtop_vm_storage;
NS_END2