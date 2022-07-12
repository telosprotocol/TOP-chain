#pragma once

#include "xbasic/xbyte_buffer.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>

namespace top {
namespace evm {

class xtop_evm_logic_face {
public:
    xtop_evm_logic_face() = default;
    xtop_evm_logic_face(xtop_evm_logic_face const &) = delete;
    xtop_evm_logic_face & operator=(xtop_evm_logic_face const &) = delete;
    xtop_evm_logic_face(xtop_evm_logic_face &&) = default;
    xtop_evm_logic_face & operator=(xtop_evm_logic_face &&) = default;
    virtual ~xtop_evm_logic_face() = default;

public:
    virtual xbytes_t get_return_value() const = 0;
    virtual std::pair<uint32_t, uint64_t> get_return_error() const = 0;

public:
    // interface to evm_import_instance:

    // register:
    virtual void read_register(uint64_t register_id, uint64_t ptr) = 0;
    virtual uint64_t register_len(uint64_t register_id) = 0;

    // context:
    virtual void sender_address(uint64_t register_id) = 0;
    virtual void input(uint64_t register_id) = 0;

    // EVM API:
    virtual uint64_t chain_id() = 0;
    virtual void block_coinbase(uint64_t register_id) = 0;
    virtual uint64_t block_height() = 0;
    virtual uint64_t block_timestamp() = 0;

    // math:
    virtual void random_seed(uint64_t register_id) = 0;
    virtual void sha256(uint64_t value_len, uint64_t value_ptr, uint64_t register_id) = 0;
    virtual void keccak256(uint64_t value_len, uint64_t value_ptr, uint64_t register_id) = 0;
    virtual void ripemd160(uint64_t value_len, uint64_t value_ptr, uint64_t register_id) = 0;

    // others:
    virtual void value_return(uint64_t value_len, uint64_t value_ptr) = 0;
    virtual void error_return(uint32_t ec, uint64_t used_gas) = 0;
    virtual void log_utf8(uint64_t len, uint64_t ptr) = 0;

    // storage:
    virtual uint64_t storage_write(uint64_t key_len, uint64_t key_ptr, uint64_t value_len, uint64_t value_ptr, uint64_t register_id) = 0;
    virtual uint64_t storage_read(uint64_t key_len, uint64_t key_ptr, uint64_t register_id) = 0;
    virtual uint64_t storage_remove(uint64_t key_len, uint64_t key_ptr, uint64_t register_id) = 0;

    // extern contract:
    virtual bool extern_contract_call(uint64_t args_len, uint64_t args_ptr) = 0;
    virtual uint64_t get_result(uint64_t register_id) = 0;
    virtual uint64_t get_error(uint64_t register_id) = 0;

    virtual void engine_return(uint64_t engine_ptr) = 0;
    virtual void executor_return(uint64_t executor_ptr) = 0;
    virtual void * engine_ptr() const = 0;
    virtual void * executor_ptr() const = 0;
};
using xevm_logic_face_t = xtop_evm_logic_face;

}  // namespace evm
}  // namespace top
