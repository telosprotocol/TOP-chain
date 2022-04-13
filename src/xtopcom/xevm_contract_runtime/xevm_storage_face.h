#pragma once

#include "xbase/xns_macro.h"
#include "xbasic/xbyte_buffer.h"

#include <cstdint>
#include <vector>

NS_BEG3(top, contract_runtime, evm)

class xtop_evm_storage_face {
public:
    xtop_evm_storage_face() = default;
    xtop_evm_storage_face(xtop_evm_storage_face const &) = delete;
    xtop_evm_storage_face & operator=(xtop_evm_storage_face const &) = delete;
    xtop_evm_storage_face(xtop_evm_storage_face &&) = default;
    xtop_evm_storage_face & operator=(xtop_evm_storage_face &&) = default;
    virtual ~xtop_evm_storage_face() = default;

    virtual xbytes_t storage_get(xbytes_t const & key) = 0;
    virtual void storage_set(xbytes_t const & key, xbytes_t const & value) = 0;
    virtual void storage_remove(xbytes_t const & key) = 0;
};

using xevm_storage_face_t = xtop_evm_storage_face;

NS_END3
