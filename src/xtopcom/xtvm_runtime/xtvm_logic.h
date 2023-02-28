// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xtvm_engine_rs/tvm-c-api/tvm_logic_face.h"
#include "xtvm_runtime/xtvm_context.h"
#include "xtvm_runtime/xtvm_storage.h"

#include <map>

NS_BEG2(top, tvm)

class xtop_vm_logic : public tvm_logic_face {
private:
    observer_ptr<xtvm_storage_t> m_storage;
    observer_ptr<xtvm_context_t> m_context;
    std::map<uint64_t, xbytes_t> m_registers;
    xbytes_t m_return_data_value;
    xbytes_t m_call_contract_args;

public:
    xtop_vm_logic(observer_ptr<xtvm_storage_t> storage_ptr, observer_ptr<xtvm_context_t> context);

public:
    // for runtime use
    xbytes_t get_return_value() const;

public:
    // override api
    void read_register(uint64_t register_id, uint64_t ptr) override;
    uint64_t register_len(uint64_t register_id) override;
    void input(uint64_t register_id) override;
    void result(uint64_t value_len, uint64_t value_ptr) override;
    uint64_t storage_write(uint64_t key_len, uint64_t key_ptr, uint64_t value_len, uint64_t value_ptr, uint64_t register_id) override;
    uint64_t storage_read(uint64_t key_len, uint64_t key_ptr, uint64_t register_id) override;
    uint64_t storage_remove(uint64_t key_len, uint64_t key_ptr, uint64_t register_id) override;
    uint64_t gas_price() override;
    void origin_address(uint64_t register_id) override;
    uint64_t block_height() override;
    void block_coinbase(uint64_t register_id) override;
    uint64_t block_timestamp() override;
    uint64_t chain_id() override;
    void log_utf8(uint64_t len, uint64_t ptr) override;
};
using xtvm_logic_t = xtop_vm_logic;

NS_END2