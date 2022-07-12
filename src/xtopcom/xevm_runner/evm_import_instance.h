#pragma once

#include "xbase/xlock.h"
#include "xevm_runner/evm_logic_face.h"

#include <cstdint>
#include <thread>

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

    std::map<std::string, std::shared_ptr<top::evm::xevm_logic_face_t>> m_vm_logic_dict;
    base::xrwlock_t mutable m_rwlock;
    std::shared_ptr<top::evm::xevm_logic_face_t> current_vm_logic() const;
    std::shared_ptr<top::evm::xevm_logic_face_t> current_vm_logic(std::error_code & ec) const;

public:
    // add {thread_id, vm_logic} into evm_instance.
    // !!!Noted: do not forget to call **`remove_evm_logic`** after used in same thread.
    void add_evm_logic(std::shared_ptr<top::evm::xevm_logic_face_t> vm_logic_ptr);

    // remove {thread_id, vm_logic} from evm_instance.
    void remove_evm_logic();

public:
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

    void * engine_ptr() const;
    void * executor_ptr() const;
    void engine_return(uint64_t engine_ptr);
    void executor_return(uint64_t executor_ptr);
};

}  // namespace evm
}  // namespace top
