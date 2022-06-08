#pragma once
#include "stdint.h"
#include "xevm_runner/evm_logic_face.h"

namespace top {
namespace evm {

class evm_import_instance {
public:
    static evm_import_instance * instance();

private:
    evm_import_instance() {
    }
    ~evm_import_instance() {
    }

    std::unique_ptr<xevm_logic_face_t> m_vm_logic{nullptr};

    // top::contract_runtime::evm::xtop_evm_logic m_vm_logic{nullptr, nullptr};

public:
    void set_evm_logic(std::unique_ptr<top::evm::xevm_logic_face_t> vm_logic_ptr);

    // todo delete this
    // not allow to change logic after set it.
    xevm_logic_face_t * get_vm_logic_ref();

    xbytes_t get_return_value();
    std::pair<uint32_t, uint64_t> get_return_error();

public:
    // register:
    void read_register(uint64_t register_id, uint64_t ptr);
    uint64_t register_len(uint64_t register_id);

    // context:
    void sender_address(uint64_t register_id);
    void input(uint64_t register_id);

    // EVM API:
    uint64_t evm_chain_id();
    void evm_block_coinbase(uint64_t register_id);
    uint64_t evm_block_height();
    uint64_t evm_block_timestamp();

    // math:
    void random_seed(uint64_t register_id);
    void sha256(uint64_t value_len, uint64_t value_ptr, uint64_t register_id);
    void keccak256(uint64_t value_len, uint64_t value_ptr, uint64_t register_id);
    void ripemd160(uint64_t value_len, uint64_t value_ptr, uint64_t register_id);

    // others:
    void value_return(uint64_t value_len, uint64_t value_ptr);
    void error_return(uint32_t ec, uint64_t used_gas);
    void log_utf8(uint64_t len, uint64_t ptr);

    // storage:
    uint64_t storage_write(uint64_t key_len, uint64_t key_ptr, uint64_t value_len, uint64_t value_ptr, uint64_t register_id);
    uint64_t storage_read(uint64_t key_len, uint64_t key_ptr, uint64_t register_id);
    uint64_t storage_remove(uint64_t key_len, uint64_t key_ptr, uint64_t register_id);

    // extern contract:
    bool extern_contract_call(uint64_t args_len, uint64_t args_ptr);
    uint64_t get_result(uint64_t register_id);
    uint64_t get_error(uint64_t register_id);

    // contract bridge:
    void call_erc20(uint64_t input_len, uint64_t input_ptr, uint64_t target_gas, uint64_t address_len, uint64_t address_ptr, bool is_static, uint64_t register_id);
};

}  // namespace evm
}  // namespace top
