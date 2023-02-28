// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtvm_runtime/xtvm_logic.h"

#include "xbasic/xhex.h"

#include <vector>

NS_BEG2(top, tvm)

NS_BEG1(register_tools)

void read_memory(uint64_t offset, xbytes_t & buffer) {
    char * begin_address = (char *)offset;
    for (std::size_t i = 0; i < buffer.size(); ++i) {
        buffer[i] = *begin_address;
        begin_address++;
    }
}

void write_memory(uint64_t offset, xbytes_t const & buffer) {
    char * begin_address = (char *)offset;
    for (std::size_t i = 0; i < buffer.size(); ++i) {
        *begin_address = buffer[i];
        begin_address++;
    }
}

NS_END1

xtop_vm_logic::xtop_vm_logic(observer_ptr<xtvm_storage_t> storage_ptr, observer_ptr<xtvm_context_t> context) : m_storage{storage_ptr}, m_context{context} {
    xdbg("tvm logic instance %p", static_cast<void *>(this));
}

xbytes_t xtop_vm_logic::get_return_value() const {
    return m_return_data_value;
}

void xtop_vm_logic::read_register(uint64_t register_id, uint64_t ptr) {
    auto data = m_registers.at(register_id);
    register_tools::write_memory(ptr, data);
}
uint64_t xtop_vm_logic::register_len(uint64_t register_id) {
    return m_registers.at(register_id).size();
}
void xtop_vm_logic::input(uint64_t register_id) {
    m_registers[register_id] = m_context->input_data();
}
void xtop_vm_logic::result(uint64_t value_len, uint64_t value_ptr) {
    m_return_data_value.resize(value_len, 0);
    register_tools::read_memory(value_ptr, m_return_data_value);
}
uint64_t xtop_vm_logic::storage_write(uint64_t key_len, uint64_t key_ptr, uint64_t value_len, uint64_t value_ptr, uint64_t register_id) {
    auto key = xbytes_t(key_len);
    register_tools::read_memory(key_ptr, key);
    auto value = xbytes_t(value_len);
    register_tools::read_memory(value_ptr, value);
    auto old_value = m_storage->storage_get(key);
    m_storage->storage_set(key, value);
    if (!old_value.empty()) {
        m_registers[register_id] = old_value;
        return 1;
    }
    return 0;
}
uint64_t xtop_vm_logic::storage_read(uint64_t key_len, uint64_t key_ptr, uint64_t register_id) {
    auto key = xbytes_t(key_len);
    register_tools::read_memory(key_ptr, key);
    auto value = m_storage->storage_get(key);
    if (!value.empty()) {
        m_registers[register_id] = value;
        return 1;
    }
    return 0;
}
uint64_t xtop_vm_logic::storage_remove(uint64_t key_len, uint64_t key_ptr, uint64_t register_id) {
    auto key = xbytes_t(key_len);
    register_tools::read_memory(key_ptr, key);
    auto old_value = m_storage->storage_get(key);
    m_storage->storage_remove(key);
    if (!old_value.empty()) {
        m_registers[register_id] = old_value;
        return 1;
    }
    return 0;
}
uint64_t xtop_vm_logic::gas_price() {
    return m_context->gas_price();
}
void xtop_vm_logic::origin_address(uint64_t register_id) {
    auto sender = m_context->sender();
    assert(sender.to_string().substr(0, 6) == std::string("T80000"));
    std::string sender_str = sender.to_string().substr(6);
    assert(sender_str.size() == 40);
    std::error_code ec;
    auto sender_bytes = top::from_hex(sender_str, ec);
    assert(!ec);
    assert(sender_bytes.size() == 20);
    m_registers[register_id] = sender_bytes;
}
uint64_t xtop_vm_logic::block_height() {
    return m_context->block_height();
}
void xtop_vm_logic::block_coinbase(uint64_t register_id) {
    auto coinbase = m_context->coinbase();
    assert(coinbase.to_string().substr(0, 6) == std::string("T80000"));
    std::string coinbase_str = coinbase.to_string().substr(6);
    assert(coinbase_str.size() == 40);
    std::error_code ec;
    auto coinbase_bytes = top::from_hex(coinbase_str, ec);
    assert(!ec);
    assert(coinbase_bytes.size() == 20);
    m_registers[register_id] = coinbase_bytes;
}
uint64_t xtop_vm_logic::block_timestamp() {
    return m_context->block_timestamp();
}
uint64_t xtop_vm_logic::chain_id() {
    return m_context->chain_id();
}
void xtop_vm_logic::log_utf8(uint64_t len, uint64_t ptr) {
    auto logs_bytes = xbytes_t(len);
    register_tools::read_memory(ptr, logs_bytes);
    std::string logs_str = top::to_string(logs_bytes);
    xinfo("[log_utf8] TVM_LOG: %s", logs_str.c_str());
}

NS_END2