#include "xevm_runner/evm_import_instance.h"

#include <cassert>
#include <cassert>

namespace top {
namespace evm {

evm_import_instance * evm_import_instance::instance() {
    static evm_import_instance ins;
    return &ins;
}

std::shared_ptr<top::evm::xevm_logic_face_t> evm_import_instance::current_vm_logic() const {
    std::shared_ptr<top::evm::xevm_logic_face_t> vm_logic{nullptr};
    auto current_thread_id_hash = std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    // xdbg("evm_import_instance::current_vm_logic %s", current_thread_id_hash.c_str());
    m_rwlock.lock_read();
    assert(m_vm_logic_dict.find(current_thread_id_hash) != m_vm_logic_dict.end());
    vm_logic = m_vm_logic_dict.at(current_thread_id_hash);
    m_rwlock.release_read();
    assert(vm_logic != nullptr);
    return vm_logic;
}

std::shared_ptr<top::evm::xevm_logic_face_t> evm_import_instance::current_vm_logic(std::error_code & ec) const {
    std::shared_ptr<top::evm::xevm_logic_face_t> vm_logic{nullptr};
    auto current_thread_id_hash = std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    // xdbg("evm_import_instance::current_vm_logic %s", current_thread_id_hash.c_str());
    m_rwlock.lock_read();
    if (m_vm_logic_dict.find(current_thread_id_hash) == m_vm_logic_dict.end()) {
        m_rwlock.release_read();

        assert(!ec);
        ec = make_error_code(std::errc::result_out_of_range);
        return nullptr;
    }

    vm_logic = m_vm_logic_dict.at(current_thread_id_hash);
    m_rwlock.release_read();
    assert(vm_logic != nullptr);
    return vm_logic;
}

void evm_import_instance::add_evm_logic(std::shared_ptr<top::evm::xevm_logic_face_t> vm_logic_ptr) {
    auto current_thread_id_hash = std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    xdbg("evm_import_instance::add_evm_logic %s", current_thread_id_hash.c_str());
    m_rwlock.lock_write();
    assert(m_vm_logic_dict.find(current_thread_id_hash) == m_vm_logic_dict.end());
    m_vm_logic_dict.insert({current_thread_id_hash, vm_logic_ptr});
    m_rwlock.release_write();
}

void evm_import_instance::remove_evm_logic() {
    auto current_thread_id_hash = std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    xdbg("evm_import_instance::remove_evm_logic %s", current_thread_id_hash.c_str());
    m_rwlock.lock_write();
    assert(m_vm_logic_dict.find(current_thread_id_hash) != m_vm_logic_dict.end());
    m_vm_logic_dict.erase(current_thread_id_hash);
    m_rwlock.release_write();
}

xbytes_t evm_import_instance::get_return_value() {
    return current_vm_logic()->get_return_value();
}

std::pair<uint32_t, uint64_t> evm_import_instance::get_return_error() {
    return current_vm_logic()->get_return_error();
}

// register:
void evm_import_instance::read_register(uint64_t register_id, uint64_t ptr) {
    current_vm_logic()->read_register(register_id, ptr);
    return;
}
uint64_t evm_import_instance::register_len(uint64_t register_id) {
    return current_vm_logic()->register_len(register_id);
}

void evm_import_instance::sender_address(uint64_t register_id) {
    current_vm_logic()->sender_address(register_id);
    return;
}
void evm_import_instance::input(uint64_t register_id) {
    current_vm_logic()->input(register_id);
    return;
}

// # EVM API #
uint64_t evm_import_instance::evm_chain_id() {
    return current_vm_logic()->chain_id();
}
void evm_import_instance::evm_block_coinbase(uint64_t register_id) {
    return current_vm_logic()->block_coinbase(register_id);
}
uint64_t evm_import_instance::evm_block_height() {
    return current_vm_logic()->block_height();
}
uint64_t evm_import_instance::evm_block_timestamp() {
    return current_vm_logic()->block_timestamp();
}

// math:
void evm_import_instance::random_seed(uint64_t register_id) {
    current_vm_logic()->random_seed(register_id);
    return;
}
void evm_import_instance::sha256(uint64_t value_len, uint64_t value_ptr, uint64_t register_id) {
    current_vm_logic()->sha256(value_len, value_ptr, register_id);
    return;
}
void evm_import_instance::keccak256(uint64_t value_len, uint64_t value_ptr, uint64_t register_id) {
    current_vm_logic()->keccak256(value_len, value_ptr, register_id);
    return;
}
void evm_import_instance::ripemd160(uint64_t value_len, uint64_t value_ptr, uint64_t register_id) {
    current_vm_logic()->ripemd160(value_len, value_ptr, register_id);
    return;
}

// others:
void evm_import_instance::value_return(uint64_t value_len, uint64_t value_ptr) {
    return current_vm_logic()->value_return(value_len, value_ptr);
}
void evm_import_instance::error_return(uint32_t ec, uint64_t used_gas) {
    current_vm_logic()->error_return(ec, used_gas);
    return;
}
void evm_import_instance::log_utf8(uint64_t len, uint64_t ptr) {
    current_vm_logic()->log_utf8(len, ptr);
    return;
}

// storage:
uint64_t evm_import_instance::storage_write(uint64_t key_len, uint64_t key_ptr, uint64_t value_len, uint64_t value_ptr, uint64_t register_id) {
    return current_vm_logic()->storage_write(key_len, key_ptr, value_len, value_ptr, register_id);
}
uint64_t evm_import_instance::storage_read(uint64_t key_len, uint64_t key_ptr, uint64_t register_id) {
    return current_vm_logic()->storage_read(key_len, key_ptr, register_id);
}
uint64_t evm_import_instance::storage_remove(uint64_t key_len, uint64_t key_ptr, uint64_t register_id) {
    return current_vm_logic()->storage_remove(key_len, key_ptr, register_id);
}

// extern contract:
bool evm_import_instance::extern_contract_call(uint64_t args_len, uint64_t args_ptr) {
    return current_vm_logic()->extern_contract_call(args_len, args_ptr);
}
uint64_t evm_import_instance::get_result(uint64_t register_id) {
    return current_vm_logic()->get_result(register_id);
}
uint64_t evm_import_instance::get_error(uint64_t register_id) {
    return current_vm_logic()->get_error(register_id);
}

void * evm_import_instance::engine_ptr() const {
    std::error_code ec;
    auto const logic = current_vm_logic(ec);
    if (logic == nullptr) {
        return nullptr;
    }

    return logic->engine_ptr();
}

void * evm_import_instance::executor_ptr() const {
    std::error_code ec;
    auto const logic = current_vm_logic(ec);
    if (logic == nullptr) {
        return nullptr;
    }

    return logic->executor_ptr();
}

void evm_import_instance::engine_return(uint64_t engine_ptr) {
    current_vm_logic()->engine_return(engine_ptr);
}
void evm_import_instance::executor_return(uint64_t executor_ptr) {
    current_vm_logic()->executor_return(executor_ptr);
}

/// =======================================
/// RUST CALL C
/// =======================================
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
void evm_sender_address(uint64_t register_id) {
    evm_import_instance::instance()->sender_address(register_id);
    return;
}
void evm_input(uint64_t register_id) {
    evm_import_instance::instance()->input(register_id);
    return;
}

// # EVM API #
uint64_t evm_chain_id() {
    return evm_import_instance::instance()->evm_chain_id();
}
void evm_block_coinbase(uint64_t register_id) {
    return evm_import_instance::instance()->evm_block_coinbase(register_id);
}
uint64_t evm_block_height() {
    return evm_import_instance::instance()->evm_block_height();
}
uint64_t evm_block_timestamp() {
    return evm_import_instance::instance()->evm_block_timestamp();
}

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
void evm_error_return(uint32_t ec, uint64_t used_gas) {
    return evm_import_instance::instance()->error_return(ec, used_gas);
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

// # Extern Contract API #
bool evm_extern_contract_call(uint64_t args_len, uint64_t args_ptr) {
    return evm_import_instance::instance()->extern_contract_call(args_len, args_ptr);
}
uint64_t evm_get_result(uint64_t register_id) {
    return evm_import_instance::instance()->get_result(register_id);
}
uint64_t evm_get_error(uint64_t register_id) {
    return evm_import_instance::instance()->get_error(register_id);
}

void evm_engine_return(uint64_t ptr) {
    evm_import_instance::instance()->engine_return(ptr);
}

void evm_executor_return(uint64_t ptr) {
    evm_import_instance::instance()->executor_return(ptr);
}
}

}  // namespace evm
}  // namespace top

extern "C" {
void * evm_engine() {
    return top::evm::evm_import_instance::instance()->engine_ptr();
}

void * evm_executor() {
    return top::evm::evm_import_instance::instance()->executor_ptr();
}
}
