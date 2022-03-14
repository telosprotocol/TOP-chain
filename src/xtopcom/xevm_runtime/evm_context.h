#pragma once
#include "xevm_runtime/evm_util.h"

#include <string>
namespace top {
namespace evm {

class xtop_evm_context {
public:
    bytes m_random_seed;
    bytes m_input;
    bytes m_predecessor_account_id;

public:
    xtop_evm_context(bytes const & random_seed, bytes const & input, bytes const & predecessor_account_id)
      : m_random_seed{random_seed}, m_input{input}, m_predecessor_account_id{predecessor_account_id} {
    }

    void update_random_seed(std::string const & input_hex) {
        m_random_seed = utils::hex_string_to_bytes(input_hex);
    }

    void update_input(bytes const & input_vec_u8) {
        m_input = input_vec_u8;
    }

    void update_hex_string_input(std::string const & input_hex) {
        m_input = utils::hex_string_to_bytes(input_hex);
    }

    void update_string_predecessor_account_id(std::string const & input_str) {
        m_predecessor_account_id = utils::string_to_bytes(input_str);
    }
};
using xevm_context_t = xtop_evm_context;
}  // namespace evm
}  // namespace top