#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace top {
namespace evm {

using bytes = std::vector<uint8_t>;
typedef __int128 int128_t;
typedef unsigned __int128 uint128_t;

namespace utils {

std::vector<uint8_t> serialize_function_input(std::string const & contract_address, std::string const & contract_function, uint64_t value = 0);
std::vector<uint8_t> to_le_bytes(uint128_t value);
void hex_string_bytes_char(std::string const & input, unsigned char * output);
std::vector<uint8_t> hex_string_to_bytes(std::string const & input);
std::vector<uint8_t> string_to_bytes(std::string const & input);
std::string uint8_vector_to_hex_string(std::vector<uint8_t> const & v);
std::vector<uint8_t> get_sha256(std::vector<uint8_t> const & input);
std::vector<uint8_t> get_ripemd160(std::vector<uint8_t> const & input);
}  // namespace utils
}  // namespace evm
}  // namespace top