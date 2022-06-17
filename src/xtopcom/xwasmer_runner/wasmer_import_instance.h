#pragma once
#include "stdint.h"
#include "xbase/xlock.h"
#include "xwasmer_runner/wasmer_logic_face.h"

#include <map>
#include <thread>

namespace top {
namespace wasm {

class wasmer_import_instance {
public:
    static wasmer_import_instance * instance();

private:
    wasmer_import_instance() {
    }
    ~wasmer_import_instance() {
    }

    std::map<std::string, std::shared_ptr<top::wasm::xwasmer_logic_face_t>> m_vm_logic_dict;
    base::xrwlock_t m_rwlock;
    std::shared_ptr<top::wasm::xwasmer_logic_face_t> current_vm_logic();

public:
    // add {thread_id, vm_logic} into evm_instance.
    // !!!Noted: do not forget to call **`remove_evm_logic`** after used in same thread.
    void add_wasmer_logic(std::shared_ptr<top::wasm::xwasmer_logic_face_t> vm_logic_ptr);

    // remove {thread_id, vm_logic} from evm_instance.
    void remove_wasmer_logic();

public:
    uint64_t wasmer_block_index();
    const char * wasmer_get_args_ptr();
    uint32_t wasmer_get_args_size();
};

}  // namespace wasm
}  // namespace top
