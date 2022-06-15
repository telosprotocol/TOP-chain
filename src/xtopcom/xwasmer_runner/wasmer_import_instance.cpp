#include "xwasmer_runner/wasmer_import_instance.h"

#include "assert.h"
#include "stdint.h"
#include "xbasic/xbyte_buffer.h"

namespace top {
namespace wasm {

wasmer_import_instance * wasmer_import_instance::instance() {
    static wasmer_import_instance ins;
    return &ins;
}

std::shared_ptr<top::wasm::xwasmer_logic_face_t> wasmer_import_instance::current_vm_logic() {
    std::shared_ptr<top::wasm::xwasmer_logic_face_t> vm_logic{nullptr};
    auto current_thread_id_hash = std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    // xdbg("wasmer_import_instance::current_vm_logic %s", current_thread_id_hash.c_str());
    m_rwlock.lock_read();
    assert(m_vm_logic_dict.find(current_thread_id_hash) != m_vm_logic_dict.end());
    vm_logic = m_vm_logic_dict.at(current_thread_id_hash);
    m_rwlock.release_read();
    assert(vm_logic != nullptr);
    return vm_logic;
}

void wasmer_import_instance::add_wasmer_logic(std::shared_ptr<top::wasm::xwasmer_logic_face_t> vm_logic_ptr) {
    auto current_thread_id_hash = std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    xdbg("wasmer_import_instance::add_wasmer_logic %s", current_thread_id_hash.c_str());
    m_rwlock.lock_write();
    assert(m_vm_logic_dict.find(current_thread_id_hash) == m_vm_logic_dict.end());
    m_vm_logic_dict.insert({current_thread_id_hash, vm_logic_ptr});
    m_rwlock.release_write();
}

void wasmer_import_instance::remove_wasmer_logic() {
    auto current_thread_id_hash = std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    xdbg("wasmer_import_instance::remove_wasmer_logic %s", current_thread_id_hash.c_str());
    m_rwlock.lock_write();
    assert(m_vm_logic_dict.find(current_thread_id_hash) != m_vm_logic_dict.end());
    m_vm_logic_dict.erase(current_thread_id_hash);
    m_rwlock.release_write();
}

uint64_t wasmer_import_instance::wasmer_block_index() {
    return current_vm_logic()->block_index();
}

/// =======================================
/// RUST CALL C
/// =======================================

extern "C" {
uint64_t wasmer_block_index() {
    return wasmer_import_instance::instance()->wasmer_block_index();
}
}
}  // namespace wasm
}  // namespace top
