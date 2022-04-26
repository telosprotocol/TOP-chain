
#include "xevm_contract_runtime/xevm_logic.h"

#include "xbasic/endianness.h"
#include "xcontract_runtime/xerror/xerror.h"
#include "xevm_common/common_data.h"
#include "xevm_contract_runtime/xevm_memory_tools.h"
#include "xevm_contract_runtime/xevm_variant_bytes.h"
#include "xevm_runner/proto/proto_basic.pb.h"
#include "xevm_runner/proto/proto_parameters.pb.h"

#include <climits>

NS_BEG3(top, contract_runtime, evm)

xtop_evm_logic::xtop_evm_logic(std::shared_ptr<xevm_storage_face_t> storage_ptr,
                               observer_ptr<statectx::xstatectx_face_t> state_ctx,
                               observer_ptr<evm_runtime::xevm_context_t> const & context,
                               observer_ptr<xevm_contract_manager_t> const & contract_manager)
  : m_storage_ptr{storage_ptr}, m_state_ctx{state_ctx}, m_context{context}, m_contract_manager{contract_manager} {
    m_registers.clear();
    m_return_data_value.clear();
}

//  =========================== for runtime ===============================
xbytes_t xtop_evm_logic::get_return_value() {
    return m_return_data_value;
}

std::pair<uint32_t, uint64_t> xtop_evm_logic::get_return_error() {
    return m_return_error_value;
}

//  =========================== interface to evm_import ===============================
uint64_t xtop_evm_logic::register_len(uint64_t register_id) {
    // printf("[debug][register_len] size: %zu request: %lu \n", m_registers.size(), register_id);
    return m_registers.at(register_id).size();
}

void xtop_evm_logic::read_register(uint64_t register_id, uint64_t ptr) {
    xbytes_t data = internal_read_register(register_id);
    // printf("[debug][read_register] request: %lu \n ", register_id);
    // for (auto const & _c : data) {
    //     printf("%x", _c);
    // }
    // printf("\n");
    // printf("debug %lu \n",ptr);
    memory_set_slice(ptr, data);
}

void xtop_evm_logic::sender_address(uint64_t register_id) {
    // printf("[debug][sender_address] request: %lu \n", register_id);
    // internal_write_register(register_id, m_context->m_sender_address);
    auto sender = m_context->sender().value();
    xassert(sender.substr(0, 6) == T6_ACCOUNT_PREFIX);
    xvariant_bytes hex_address{sender.substr(6), true};
    internal_write_register(register_id, hex_address.to_bytes());
}

void xtop_evm_logic::input(uint64_t register_id) {
    // printf("[debug][input] request: %lu\n", register_id);
    // internal_write_register(register_id, m_context->input());

    internal_write_register(register_id, m_context->input_data());
    return;
}

// storage:
uint64_t xtop_evm_logic::storage_read(uint64_t key_len, uint64_t key_ptr, uint64_t register_id) {
    // printf("[debug][storage_read] request: %lu\n", register_id);
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
    // printf("[debug][storage_write] request: %lu\n", register_id);
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
    // printf("[debug][storage_remove] request: %lu\n", register_id);
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
    // printf("[debug][value_return] in hex: ");
    // for (auto const & _c : m_return_data_value) {
    //     printf("%x", _c);
    // }
    // printf("\n");
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
    // internal_write_register(register_id, m_context->random_seed());
    internal_write_register(register_id, top::to_bytes(m_context->random_seed()));
}

// LOG
void xtop_evm_logic::log_utf8(uint64_t len, uint64_t ptr) {
    std::string message = get_utf8_string(len, ptr);
    // todo add xinfo_log.
    // printf("[log_utf8] EVM_LOG: %s \n", message.c_str());
    xdbg("[log_utf8] EVM_LOG: %s", message.c_str());
}

// extern contract:
bool xtop_evm_logic::extern_contract_call(uint64_t args_len, uint64_t args_ptr) {
    m_result_ok.clear();
    m_result_err.clear();
    m_call_contract_args = get_vec_from_memory_or_register(args_ptr, args_len);
    xbytes_t contract_output;
    assert(m_contract_manager != nullptr);
    if (m_contract_manager->execute_sys_contract(m_call_contract_args, contract_output)) {
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

void xtop_evm_logic::call_erc20(xbytes_t const & input, std::string const & contract_address, uint64_t const target_gas, bool is_static, uint64_t const register_id) {
    // ERC20 method ids:
    //--------------------------------------------------
    // decimals()                            => 313ce567
    // totalSupply()                         => 18160ddd
    // balanceOf(address)                    => 70a08231
    // transfer(address,uint256)             => a9059cbb
    // transferFrom(address,address,uint256) => 23b872dd
    // approve(address,uint256)              => 095ea7b3
    // allowance(address,address)            => dd62ed3e
    // approveTOP(bytes32,uint64)            => 24655e23
    //--------------------------------------------------
#if defined(__LITTLE_ENDIAN__)
    constexpr uint32_t method_id_decimals{0x67e53c31};
    constexpr uint32_t method_id_total_supply{0xdd0d1618};
    constexpr uint32_t method_id_balance_of{0x3182a070};
    constexpr uint32_t method_id_transfer{0xbb9c05a9};
    constexpr uint32_t method_id_transfer_from{0xdd72b823};
    constexpr uint32_t method_id_approve{0xb3a75e09};
    constexpr uint32_t method_id_allowance{0x3eed62dd};
#elif defined(__BIG_ENDIAN__)
    constexpr uint32_t method_id_decimals{0x313ce567};
    constexpr uint32_t method_id_total_supply{0x18160ddd};
    constexpr uint32_t method_id_balance_of{0x70a08231};
    constexpr uint32_t method_id_transfer{0xa9059cbb};
    constexpr uint32_t method_id_transfer_from{0x23b872dd};
    constexpr uint32_t method_id_approve{0x095ea7b3};
    constexpr uint32_t method_id_allowance{0xdd62ed3e};
#else
#    error "I don't know what architecture this is!"
#endif
    // erc20_uuid (1 byte) | erc20_method_id (4 bytes) | parameters (depends)
    auto it = std::begin(input);

    xbyte_t const erc20_uuid{*it};
    std::advance(it, 1);

    xbytes_t const method_id_bytes{it, std::next(it, 4)};
    std::advance(it, 4);

    uint32_t method_id;
    std::memcpy(&method_id, method_id_bytes.data(), 4);

    switch (method_id) {
    case method_id_decimals: {
        xbytes_t decimals;
        decimals.resize(1);
        decimals[0] = static_cast<uint8_t>(18);

        internal_write_register(register_id, decimals);
        break;
    }

    case method_id_total_supply: {
        evm_common::u256 supply;
        internal_write_register(register_id, top::to_bytes(evm_common::toBigEndianString(supply)));
        break;
    }

    case method_id_balance_of: {
        assert(m_state_ctx);
        auto state = m_state_ctx->load_unit_state(m_context->sender().vaccount());
        evm_common::u256 value{0};
        switch (erc20_uuid) {
        case 0: {
            value = state->balance();
            break;
        }

        case 1: {
            value = state->tep_token_balance("USDT");
            break;
        }

        default:
            assert(false);
            break;
        }

        internal_write_register(register_id, top::to_bytes(evm_common::toBigEndianString(value)));
        break;
    }

    case method_id_transfer:
        break;

    case method_id_transfer_from:
        break;

    case method_id_approve:
        break;

    case method_id_allowance:
        break;

    default:
        assert(false);
        break;
    }
}

NS_END3
