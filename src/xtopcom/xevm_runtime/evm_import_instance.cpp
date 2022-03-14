#include "xevm_runtime/evm_import_instance.h"

#include "stdint.h"
namespace top {
namespace evm {

evm_import_instance * evm_import_instance::instance() {
    static evm_import_instance ins;
    return &ins;
}

void evm_import_instance::set_evm_logic(top::evm::xtop_evm_logic & vm_logic) {
    m_vm_logic = std::move(vm_logic);
}

top::evm::xtop_evm_logic & evm_import_instance::get_vm_logic_ref() {
    return m_vm_logic;
}

// register:
void evm_import_instance::read_register(uint64_t register_id, uint64_t ptr) {
    m_vm_logic.read_register(register_id, ptr);
    return;
}
uint64_t evm_import_instance::register_len(uint64_t register_id) {
    return m_vm_logic.register_len(register_id);
}

// context:
// void evm_import_instance::current_account_id(uint64_t register_id) {
//     m_vm_logic.current_account_id(register_id);
//     return;
// }
// void evm_import_instance::signer_account_id(uint64_t register_id) {
//     m_vm_logic.signer_account_id(register_id);
//     return;
// }
void evm_import_instance::predecessor_account_id(uint64_t register_id) {
    m_vm_logic.predecessor_account_id(register_id);
    return;
}
void evm_import_instance::input(uint64_t register_id) {
    m_vm_logic.input(register_id);
    return;
}
// void evm_import_instance::account_balance(uint64_t balance_ptr) {
//     m_vm_logic.account_balance(balance_ptr);
//     return;
// }
// math:
void evm_import_instance::random_seed(uint64_t register_id) {
    m_vm_logic.random_seed(register_id);
    return;
}
void evm_import_instance::sha256(uint64_t value_len, uint64_t value_ptr, uint64_t register_id) {
    m_vm_logic.sha256(value_len, value_ptr, register_id);
    return;
}
void evm_import_instance::keccak256(uint64_t value_len, uint64_t value_ptr, uint64_t register_id) {
    m_vm_logic.keccak256(value_len, value_ptr, register_id);
    return;
}
void evm_import_instance::ripemd160(uint64_t value_len, uint64_t value_ptr, uint64_t register_id) {
    m_vm_logic.ripemd160(value_len, value_ptr, register_id);
    return;
}

// others:
void evm_import_instance::value_return(uint64_t value_len, uint64_t value_ptr) {
    return m_vm_logic.value_return(value_len, value_ptr);
}
void evm_import_instance::log_utf8(uint64_t len, uint64_t ptr) {
    m_vm_logic.log_utf8(len, ptr);
    return;
}

// storage:
uint64_t evm_import_instance::storage_write(uint64_t key_len, uint64_t key_ptr, uint64_t value_len, uint64_t value_ptr, uint64_t register_id) {
    return m_vm_logic.storage_write(key_len, key_ptr, value_len, value_ptr, register_id);
}
uint64_t evm_import_instance::storage_read(uint64_t key_len, uint64_t key_ptr, uint64_t register_id) {
    return m_vm_logic.storage_read(key_len, key_ptr, register_id);
}
uint64_t evm_import_instance::storage_remove(uint64_t key_len, uint64_t key_ptr, uint64_t register_id) {
    return m_vm_logic.storage_remove(key_len, key_ptr, register_id);
}

extern "C" {
// # Registers #
void evm_read_register(uint64_t register_id, uint64_t ptr) {
    evm_import_instance::instance()->read_register(register_id, ptr);
    return;
}
uint64_t evm_register_len(uint64_t register_id) {
    return evm_import_instance::instance()->register_len(register_id);
}
// # Context API #
// evm_void current_account_id(uint64_t register_id) {
//     evm_import_instance::instance()->current_account_id(register_id);
//     return;
// }
// evm_void signer_account_id(uint64_t register_id) {
//     evm_import_instance::instance()->signer_account_id(register_id);
//     return;
// }
void evm_predecessor_account_id(uint64_t register_id) {
    evm_import_instance::instance()->predecessor_account_id(register_id);
    return;
}
void evm_input(uint64_t register_id) {
    evm_import_instance::instance()->input(register_id);
    return;
}

// // # Economics API #
// evm_void account_balance(uint64_t balance_ptr) {
//     evm_import_instance::instance()->account_balance(balance_ptr);
//     return;
// }

// ############
// # Math API #
// ############
void evm_random_seed(uint64_t register_id) {
    evm_import_instance::instance()->random_seed(register_id);
    return;
}
void evm_sha256(uint64_t value_len, uint64_t value_ptr, uint64_t register_id) {
    evm_import_instance::instance()->sha256(value_len, value_ptr, register_id);
    return;
}
void evm_keccak256(uint64_t value_len, uint64_t value_ptr, uint64_t register_id) {
    evm_import_instance::instance()->keccak256(value_len, value_ptr, register_id);
    return;
}
void evm_ripemd160(uint64_t value_len, uint64_t value_ptr, uint64_t register_id) {
    evm_import_instance::instance()->ripemd160(value_len, value_ptr, register_id);
    return;
}
uint64_t evm_ecrecover(uint64_t hash_len, uint64_t hash_ptr, uint64_t sig_len, uint64_t sig_ptr, uint64_t v, uint64_t malleability_flag, uint64_t register_id) {
    return 1;
}
// # Miscellaneous API #
void evm_value_return(uint64_t value_len, uint64_t value_ptr) {
    return evm_import_instance::instance()->value_return(value_len, value_ptr);
}
void evm_log_utf8(uint64_t len, uint64_t ptr) {
    evm_import_instance::instance()->log_utf8(len, ptr);
    return;
}
// # Storage API #
uint64_t evm_storage_write(uint64_t key_len, uint64_t key_ptr, uint64_t value_len, uint64_t value_ptr, uint64_t register_id) {
    return evm_import_instance::instance()->storage_write(key_len, key_ptr, value_len, value_ptr, register_id);
}
uint64_t evm_storage_read(uint64_t key_len, uint64_t key_ptr, uint64_t register_id) {
    return evm_import_instance::instance()->storage_read(key_len, key_ptr, register_id);
}
uint64_t evm_storage_remove(uint64_t key_len, uint64_t key_ptr, uint64_t register_id) {
    return evm_import_instance::instance()->storage_remove(key_len, key_ptr, register_id);
}
}

}  // namespace evm
}  // namespace top