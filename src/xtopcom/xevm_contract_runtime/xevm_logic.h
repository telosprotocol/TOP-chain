// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xevm_contract_runtime/xevm_context.h"
#include "xevm_contract_runtime/xevm_contract_manager.h"
#include "xevm_contract_runtime/xevm_storage_face.h"
#include "xevm_runner/evm_logic_face.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>

NS_BEG3(top, contract_runtime, evm)

class xtop_evm_logic : public top::evm::xevm_logic_face_t {
public:
    xtop_evm_logic(std::unique_ptr<xevm_storage_face_t> storage_ptr,
                   observer_ptr<statectx::xstatectx_face_t> state_ctx,
                   observer_ptr<evm_runtime::xevm_context_t> const & context,
                   observer_ptr<xevm_contract_manager_t> const & contract_manager);
    xtop_evm_logic(xtop_evm_logic const &) = delete;
    xtop_evm_logic & operator=(xtop_evm_logic const &) = delete;
    xtop_evm_logic(xtop_evm_logic &&) = default;
    xtop_evm_logic & operator=(xtop_evm_logic &&) = default;
    ~xtop_evm_logic() override = default;

private:
    std::unique_ptr<xevm_storage_face_t> m_storage_ptr;
    observer_ptr<statectx::xstatectx_face_t> m_state_ctx;
    observer_ptr<evm_runtime::xevm_context_t> m_context;
    observer_ptr<xevm_contract_manager_t> m_contract_manager;
    std::map<uint64_t, xbytes_t> m_registers;
    xbytes_t m_return_data_value;
    std::pair<uint32_t, uint64_t> m_return_error_value;
    xbytes_t m_call_contract_args;
    xbytes_t m_result_ok;
    xbytes_t m_result_err;

    void * m_engine_ptr{nullptr};
    void * m_executor_ptr{nullptr};

public:
    // for runtime
    xbytes_t get_return_value() const override;
    std::pair<uint32_t, uint64_t> get_return_error() const override;

public:
    // interface to evm_import_instance:

    // register:
    void read_register(uint64_t register_id, uint64_t ptr) override;
    uint64_t register_len(uint64_t register_id) override;

    // context:
    void sender_address(uint64_t register_id) override;
    void input(uint64_t register_id) override;

    // EVM API:
    uint64_t chain_id() override;
    void block_coinbase(uint64_t register_id) override;
    uint64_t block_height() override;
    uint64_t block_timestamp() override;

    // math:
    void random_seed(uint64_t register_id) override;
    void sha256(uint64_t value_len, uint64_t value_ptr, uint64_t register_id) override;
    void keccak256(uint64_t value_len, uint64_t value_ptr, uint64_t register_id) override;
    void ripemd160(uint64_t value_len, uint64_t value_ptr, uint64_t register_id) override;

    // others:
    void value_return(uint64_t value_len, uint64_t value_ptr) override;
    void error_return(uint32_t ec, uint64_t used_gas) override;
    void log_utf8(uint64_t len, uint64_t ptr) override;

    // storage:
    uint64_t storage_write(uint64_t key_len, uint64_t key_ptr, uint64_t value_len, uint64_t value_ptr, uint64_t register_id) override;
    uint64_t storage_read(uint64_t key_len, uint64_t key_ptr, uint64_t register_id) override;
    uint64_t storage_remove(uint64_t key_len, uint64_t key_ptr, uint64_t register_id) override;

    // extern contract:
    bool extern_contract_call(uint64_t args_len, uint64_t args_ptr) override;
    uint64_t get_result(uint64_t register_id) override;
    uint64_t get_error(uint64_t register_id) override;

    void engine_return(uint64_t engine_ptr) override;
    void executor_return(uint64_t executor_ptr) override;
    void * engine_ptr() const override;
    void * executor_ptr() const override;

private:
    // inner api
    std::string get_utf8_string(uint64_t len, uint64_t ptr);
    void internal_write_register(uint64_t register_id, xbytes_t const & context_input);
    xbytes_t get_vec_from_memory_or_register(uint64_t offset, uint64_t len);
    void memory_set_slice(uint64_t offset, xbytes_t buf);
    xbytes_t memory_get_vec(uint64_t offset, uint64_t len);
    xbytes_t internal_read_register(uint64_t register_id);
};
using xevm_logic_t = xtop_evm_logic;

NS_END3
