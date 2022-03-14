
#include "xevm_runtime/evm_logic.h"

#include "xevm_runtime/evm_memory_tools.h"
#include "xevm_runtime/evm_util.h"

#include <climits>
namespace top {
namespace evm {

xtop_evm_logic::xtop_evm_logic(std::shared_ptr<xevm_storage_face_t> storage_ptr, xevm_context_t const & context) : m_storage_ptr{storage_ptr}, m_context{context} {
    m_registers.clear();
    m_return_data_value.clear();
}

//  =========================== for runtime ===============================
std::shared_ptr<xevm_storage_face_t> xtop_evm_logic::ext_ref() {
    return m_storage_ptr;
}

xevm_context_t & xtop_evm_logic::context_ref() {
    return m_context;
}

std::vector<uint8_t> xtop_evm_logic::return_value() {
    return m_return_data_value;
}

//  =========================== interface to evm_import ===============================
uint64_t xtop_evm_logic::register_len(uint64_t register_id) {
    // printf("[debug][register_len] size: %zu request: %lu \n", m_registers.size(), register_id);
    return m_registers.at(register_id).size();
}

void xtop_evm_logic::read_register(uint64_t register_id, uint64_t ptr) {
    std::vector<uint8_t> data = internal_read_register(register_id);
    // printf("[debug][read_register] request: %lu \n ", register_id);
    // for (auto const & _c : data) {
    //     printf("%x", _c);
    // }
    // printf("\n");
    // printf("debug %lu \n",ptr);
    memory_set_slice(ptr, data);
}

// void xtop_evm_logic::current_account_id(uint64_t register_id) {
//     // printf("[debug][current_account_id] request: %lu \n", register_id);
//     internal_write_register(register_id, m_context.account_id);
// }

void xtop_evm_logic::predecessor_account_id(uint64_t register_id) {
    // printf("[debug][predecessor_account_id] request: %lu \n", register_id);
    internal_write_register(register_id, m_context.m_predecessor_account_id);
}

// void xtop_evm_logic::signer_account_id(uint64_t register_id) {
//     // printf("[debug][signer_account_id] request: %lu\n", register_id);
//     internal_write_register(register_id, m_context.signer_account_id);
// }

void xtop_evm_logic::input(uint64_t register_id) {
    // printf("[debug][input] request: %lu\n", register_id);
    internal_write_register(register_id, m_context.m_input);
    return;
}

// storage:
uint64_t xtop_evm_logic::storage_read(uint64_t key_len, uint64_t key_ptr, uint64_t register_id) {
    // printf("[debug][storage_read] request: %lu\n", register_id);
    std::vector<uint8_t> key = get_vec_from_memory_or_register(key_ptr, key_len);
    std::vector<uint8_t> read = m_storage_ptr->storage_get(key);
    if (!read.empty()) {
        internal_write_register(register_id, read);
        return 1;
    } else {
        return 0;
    }
}

uint64_t xtop_evm_logic::storage_write(uint64_t key_len, uint64_t key_ptr, uint64_t value_len, uint64_t value_ptr, uint64_t register_id) {
    // printf("[debug][storage_write] request: %lu\n", register_id);
    std::vector<uint8_t> key = get_vec_from_memory_or_register(key_ptr, key_len);
    std::vector<uint8_t> value = get_vec_from_memory_or_register(value_ptr, value_len);

    std::vector<uint8_t> read_old_value = m_storage_ptr->storage_get(key);

    m_storage_ptr->storage_set(key, value);

    if (!read_old_value.empty()) {
        internal_write_register(register_id, read_old_value);
        return 1;
    } else {
        return 0;
    }
}

uint64_t xtop_evm_logic::storage_remove(uint64_t key_len, uint64_t key_ptr, uint64_t register_id) {
    // printf("[debug][storage_remove] request: %lu\n", register_id);
    std::vector<uint8_t> key = get_vec_from_memory_or_register(key_ptr, key_len);
    std::vector<uint8_t> read = m_storage_ptr->storage_get(key);

    if (!read.empty()) {
        internal_write_register(register_id, read);
        m_storage_ptr->storage_remove(key);
        return 1;
    } else {
        return 0;
    }

    return 0;
}

void xtop_evm_logic::value_return(uint64_t key_len, uint64_t key_ptr) {
    m_return_data_value = get_vec_from_memory_or_register(key_ptr, key_len);
    printf("[debug][value_return] in hex: ");
    for (auto const & _c : m_return_data_value) {
        printf("%x", _c);
    }
    printf("\n");
}

// void xtop_evm_logic::account_balance(uint64_t balance_ptr) {
//     printf("[debug][account_balance]\n");

//     memory_tools::write_memory(balance_ptr, to_le_bytes(current_account_balance));
// }

void xtop_evm_logic::sha256(uint64_t value_len, uint64_t value_ptr, uint64_t register_id) {
    std::vector<uint8_t> value = get_vec_from_memory_or_register(value_ptr, value_len);

    auto value_hash = utils::get_sha256(value);

    internal_write_register(register_id, value_hash);
}

void xtop_evm_logic::keccak256(uint64_t value_len, uint64_t value_ptr, uint64_t register_id) {
    sha256(value_len, value_ptr, register_id);
}

void xtop_evm_logic::ripemd160(uint64_t value_len, uint64_t value_ptr, uint64_t register_id) {
    std::vector<uint8_t> value = get_vec_from_memory_or_register(value_ptr, value_len);

    auto value_hash = utils::get_ripemd160(value);

    internal_write_register(register_id, value_hash);
}

// MATH API
void xtop_evm_logic::random_seed(uint64_t register_id) {
    internal_write_register(register_id, m_context.m_random_seed);
}

// LOG
void xtop_evm_logic::log_utf8(uint64_t len, uint64_t ptr) {
    std::string message = get_utf8_string(len, ptr);
    printf("[log_utf8] VM_LOG: %s \n", message.c_str());
}

//  =========================== inner  api ===============================
std::string xtop_evm_logic::get_utf8_string(uint64_t len, uint64_t ptr) {
    std::vector<uint8_t> buf;
    if (len != UINT64_MAX) {
        buf = memory_get_vec(ptr, len);
    } else {
        // todo
    }

    std::string res;
    for (auto const & c : buf) {
        res.push_back(c);
    }
    return res;
}

void xtop_evm_logic::internal_write_register(uint64_t register_id, std::vector<uint8_t> const & context_input) {
    // printf("[internal_write_register]before write register size: %zu\n", m_registers.size());
    m_registers[register_id] = context_input;
    // printf("[internal_write_register]after write register size: %zu\n", m_registers.size());
    // for (auto const & _p : m_registers) {
    // printf("[debug][internal_write_register] after debug: %zu : ", _p.first);
    // for (auto const & _c : _p.second) {
    //     printf("%x", _c);
    // }
    // printf("\n");
    // }
}

std::vector<uint8_t> xtop_evm_logic::get_vec_from_memory_or_register(uint64_t offset, uint64_t len) {
    if (len != UINT64_MAX) {
        return memory_get_vec(offset, len);
    } else {
        return internal_read_register(offset);
    }
}

void xtop_evm_logic::memory_set_slice(uint64_t offset, std::vector<uint8_t> buf) {
    memory_tools::write_memory(offset, buf);
}

std::vector<uint8_t> xtop_evm_logic::memory_get_vec(uint64_t offset, uint64_t len) {
    std::vector<uint8_t> buf(len, 0);
    memory_tools::read_memory(offset, buf);
    return buf;
}

std::vector<uint8_t> xtop_evm_logic::internal_read_register(uint64_t register_id) {
    return m_registers.at(register_id);
}
}  // namespace evm
}  // namespace top
