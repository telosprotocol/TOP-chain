// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/xevm_logic.h"

#include "xbasic/endianness.h"
#include "xbasic/xhex.h"
#include "xcommon/xtoken_metadata.h"
#include "xcontract_runtime/xerror/xerror.h"
#include "xcommon/common_data.h"
#include "xevm_contract_runtime/xevm_memory_tools.h"
#if defined(XCXX20)
#include "xevm_runner/proto/ubuntu/proto_basic.pb.h"
#include "xevm_runner/proto/ubuntu/proto_parameters.pb.h"
#else
#include "xevm_runner/proto/centos/proto_basic.pb.h"
#include "xevm_runner/proto/centos/proto_parameters.pb.h"
#endif

#include <climits>

NS_BEG3(top, contract_runtime, evm)

xtop_evm_logic::xtop_evm_logic(std::unique_ptr<xevm_storage_face_t> storage_ptr,
                               observer_ptr<statectx::xstatectx_face_t> state_ctx,
                               observer_ptr<evm_runtime::xevm_context_t> const & context,
                               observer_ptr<xevm_contract_manager_t> const & contract_manager)
  : m_storage_ptr{std::move(storage_ptr)}, m_state_ctx{state_ctx}, m_context{context}, m_contract_manager{contract_manager} {
    xdbg("emv logic instance %p, contract manager instance %p", static_cast<void *>(this), static_cast<void *>(m_contract_manager.get()));
    assert(m_contract_manager != nullptr);
}

//  =========================== for runtime ===============================
xbytes_t xtop_evm_logic::get_return_value() const {
    return m_return_data_value;
}

std::pair<uint32_t, uint64_t> xtop_evm_logic::get_return_error() const {
    return m_return_error_value;
}

//  =========================== interface to evm_import ===============================
uint64_t xtop_evm_logic::register_len(uint64_t register_id) {
    return m_registers.at(register_id).size();
}

void xtop_evm_logic::read_register(uint64_t register_id, uint64_t ptr) {
    xbytes_t data = internal_read_register(register_id);
    memory_set_slice(ptr, data);
}

void xtop_evm_logic::sender_address(uint64_t register_id) {
    auto sender = m_context->sender().to_string();
    std::error_code ec;
    auto address_bytes = top::from_hex(sender.substr(6), ec);  // remove T60004
    xassert(!ec);
    internal_write_register(register_id, address_bytes);
}

void xtop_evm_logic::input(uint64_t register_id) {
    internal_write_register(register_id, m_context->input_data());
    return;
}

// EVM API:
uint64_t xtop_evm_logic::chain_id() {
    return m_context->chain_id();
}
void xtop_evm_logic::block_coinbase(uint64_t register_id) {
    auto coinbase = m_context->block_coinbase();
    std::error_code ec;
    auto coinbase_bytes = top::from_hex(coinbase.substr(6), ec);  // remove T60004
    xassert(!ec);
    internal_write_register(register_id, coinbase_bytes);
}
uint64_t xtop_evm_logic::block_height() {
    return m_context->block_height();
}
uint64_t xtop_evm_logic::block_timestamp() {
    return m_context->block_timestamp();
}

// storage:
uint64_t xtop_evm_logic::storage_read(uint64_t key_len, uint64_t key_ptr, uint64_t register_id) {
    xbytes_t key = get_vec_from_memory_or_register(key_ptr, key_len);
    xbytes_t read = m_storage_ptr->storage_get(key);
    if (!read.empty()) {
        internal_write_register(register_id, read);
        return 1;
    } else {
        return 0;
    }
}

uint64_t xtop_evm_logic::storage_write(uint64_t key_len, uint64_t key_ptr, uint64_t value_len, uint64_t value_ptr, uint64_t register_id) {
    xbytes_t key = get_vec_from_memory_or_register(key_ptr, key_len);
    xbytes_t value = get_vec_from_memory_or_register(value_ptr, value_len);

    xbytes_t read_old_value = m_storage_ptr->storage_get(key);

    m_storage_ptr->storage_set(key, value);

    if (!read_old_value.empty()) {
        internal_write_register(register_id, read_old_value);
        return 1;
    } else {
        return 0;
    }
}

uint64_t xtop_evm_logic::storage_remove(uint64_t key_len, uint64_t key_ptr, uint64_t register_id) {
    xbytes_t key = get_vec_from_memory_or_register(key_ptr, key_len);
    xbytes_t read = m_storage_ptr->storage_get(key);

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
}

void xtop_evm_logic::error_return(uint32_t ec, uint64_t used_gas) {
    m_return_error_value = std::make_pair(ec, used_gas);
}

void xtop_evm_logic::sha256(uint64_t value_len, uint64_t value_ptr, uint64_t register_id) {
    auto value = get_vec_from_memory_or_register(value_ptr, value_len);

    xbytes_t value_hash;
    utl::xsha2_256_t hasher;
    hasher.update(value.data(), value.size());
    hasher.get_hash(value_hash);

    internal_write_register(register_id, value_hash);
}

void xtop_evm_logic::keccak256(uint64_t value_len, uint64_t value_ptr, uint64_t register_id) {
    auto value = get_vec_from_memory_or_register(value_ptr, value_len);

    xbytes_t value_hash;
    utl::xkeccak256_t hasher;
    hasher.update(value.data(), value.size());
    hasher.get_hash(value_hash);

    internal_write_register(register_id, value_hash);
}

void xtop_evm_logic::ripemd160(uint64_t value_len, uint64_t value_ptr, uint64_t register_id) {
    auto value = get_vec_from_memory_or_register(value_ptr, value_len);

    xbytes_t value_hash;
    utl::xripemd160_t hasher;
    hasher.update(value.data(), value.size());
    hasher.get_hash(value_hash);

    internal_write_register(register_id, value_hash);
}

// MATH API
void xtop_evm_logic::random_seed(uint64_t register_id) {
    internal_write_register(register_id, top::to_bytes(m_context->random_seed()));
}

// LOG
void xtop_evm_logic::log_utf8(uint64_t len, uint64_t ptr) {
    std::string message = get_utf8_string(len, ptr);
    xinfo("[log_utf8] EVM_LOG: %s", message.c_str());
}

// extern contract:
bool xtop_evm_logic::extern_contract_call(uint64_t args_len, uint64_t args_ptr) {
    m_result_ok.clear();
    m_result_err.clear();
    m_call_contract_args = get_vec_from_memory_or_register(args_ptr, args_len);
    xbytes_t contract_output;
    xdbg("emv logic instance %p, contract manager instance %p", static_cast<void *>(this), static_cast<void *>(m_contract_manager.get()));
    assert(m_contract_manager != nullptr);
    if (m_contract_manager->execute_sys_contract(m_call_contract_args, m_state_ctx, contract_output)) {
        m_result_ok = contract_output;
        return true;
    } else {
        m_result_err = contract_output;
        return false;
    }
}
uint64_t xtop_evm_logic::get_result(uint64_t register_id) {
    if (!m_result_ok.empty()) {
        internal_write_register(register_id, m_result_ok);
        return 1;
    } else {
        return 0;
    }
}
uint64_t xtop_evm_logic::get_error(uint64_t register_id) {
    if (!m_result_err.empty()) {
        internal_write_register(register_id, m_result_err);
        return 1;
    } else {
        return 0;
    }
}

//  =========================== inner  api ===============================
std::string xtop_evm_logic::get_utf8_string(uint64_t len, uint64_t ptr) {
    xbytes_t buf;
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

void xtop_evm_logic::internal_write_register(uint64_t register_id, xbytes_t const & context_input) {
    m_registers[register_id] = context_input;
}

xbytes_t xtop_evm_logic::get_vec_from_memory_or_register(uint64_t offset, uint64_t len) {
    if (len != UINT64_MAX) {
        return memory_get_vec(offset, len);
    } else {
        return internal_read_register(offset);
    }
}

void xtop_evm_logic::memory_set_slice(uint64_t offset, xbytes_t buf) {
    memory_tools::write_memory(offset, buf);
}

xbytes_t xtop_evm_logic::memory_get_vec(uint64_t offset, uint64_t len) {
    xbytes_t buf(len, 0);
    memory_tools::read_memory(offset, buf);
    return buf;
}

xbytes_t xtop_evm_logic::internal_read_register(uint64_t register_id) {
    return m_registers.at(register_id);
}

void xtop_evm_logic::engine_return(uint64_t engine_ptr) {
    m_engine_ptr = reinterpret_cast<void *>(engine_ptr);
}

void xtop_evm_logic::executor_return(uint64_t executor_ptr) {
    m_executor_ptr = reinterpret_cast<void *>(executor_ptr);
}

void * xtop_evm_logic::engine_ptr() const {
    return m_engine_ptr;
}

void * xtop_evm_logic::executor_ptr() const {
    return m_executor_ptr;
}

NS_END3
