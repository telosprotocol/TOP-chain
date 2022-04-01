#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace top {
namespace evm {

using bytes = std::vector<uint8_t>;

#define T6_ACCOUNT_PREFIX "T6"
#define ETH_ACCOUNT_PREFIX "0x"

namespace utils {

std::vector<uint8_t> serialize_function_input(std::string const & contract_address, std::string const & contract_function, uint64_t value = 0);
void hex_string_bytes_char(std::string const & input, unsigned char * output);
std::vector<uint8_t> hex_string_to_bytes(std::string const & input);
std::string uint8_vector_to_hex_string(std::vector<uint8_t> const & v);
}  // namespace utils
}  // namespace evm
}  // namespace top