#pragma once
#include "stdint.h"
#include "xevm_runtime/evm_logic.h"

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

    top::evm::xtop_evm_logic m_vm_logic{nullptr, top::evm::xevm_context_t{{}, {}, {}}};

public:
    void set_evm_logic(top::evm::xtop_evm_logic & vm_logic);
    top::evm::xtop_evm_logic & get_vm_logic_ref();

public:
    // register:
    void read_register(uint64_t register_id, uint64_t ptr);
    uint64_t register_len(uint64_t register_id);

    // context:
    // void current_account_id(uint64_t register_id);
    void signer_account_id(uint64_t register_id);
    void predecessor_account_id(uint64_t register_id);
    void input(uint64_t register_id);
    void account_balance(uint64_t balance_ptr);
    // math:
    void random_seed(uint64_t register_id);
    void sha256(uint64_t value_len, uint64_t value_ptr, uint64_t register_id);
    void keccak256(uint64_t value_len, uint64_t value_ptr, uint64_t register_id);
    void ripemd160(uint64_t value_len, uint64_t value_ptr, uint64_t register_id);

    // others:
    void value_return(uint64_t value_len, uint64_t value_ptr);
    void log_utf8(uint64_t len, uint64_t ptr);

    // storage:
    uint64_t storage_write(uint64_t key_len, uint64_t key_ptr, uint64_t value_len, uint64_t value_ptr, uint64_t register_id);
    uint64_t storage_read(uint64_t key_len, uint64_t key_ptr, uint64_t register_id);
    uint64_t storage_remove(uint64_t key_len, uint64_t key_ptr, uint64_t register_id);
};

}  // namespace evm
}  // namespace top