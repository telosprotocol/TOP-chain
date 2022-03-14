#include "xevm_runtime/evm_util.h"

#include "assert.h"
#include "ripemd160.h"
#include "sh256.h"
#include "xevm_runtime/evm_engine_interface.h"

namespace top {
namespace evm {
namespace utils {

#define __ALIGN 32

std::vector<uint8_t> serialize_function_input(std::string const & contract_address, std::string const & contract_params, uint64_t value) {
    unsigned char * params = (unsigned char *)malloc((contract_params.size() - 2) / 2);
    hex_string_bytes_char(contract_params, params);

    std::size_t sum_size = 2 * (contract_address.size() + contract_params.size());
    uint64_t max_output_size = sum_size + ((-sum_size) & (__ALIGN - 1)); // might be risky
    unsigned char * output = (uint8_t *)malloc(max_output_size);
    uint64_t output_len = 0;

    serial_param_function_callargs(contract_address.c_str(), contract_address.size(), value, params, (contract_params.size() - 2) / 2, max_output_size, output, &output_len);

    std::vector<uint8_t> res;
    res.resize(output_len);
    // printf("outputlen: %lu\n", output_len);
    // printf("[");
    for (uint64_t i = 0; i < output_len; i++) {
        // printf("%d, ", output[i]);
        res[i] = output[i];
    }
    // printf("]\n");
    free(output);
    return res;
}

std::vector<uint8_t> to_le_bytes(uint128_t value) {
    std::vector<uint8_t> ret(16);
    for (auto i = 0; i < 16; i++) {
        ret[0] = (value >> i * 8) & 0xff;
    }
    return ret;
}

#define CON(x) (std::isdigit((x)) ? ((x) - '0') : (std::tolower((x)) - 'W'))

void hex_string_bytes_char(std::string const & input, unsigned char * output) {
    assert(input.substr(0, 2) == "0x");
    assert(input.size() % 2 == 0);
    for (std::size_t i = 1; i < input.size() / 2; ++i) {
        *(output + i - 1) = (CON(input[2 * i]) << 4) + CON(input[2 * i + 1]);
    }
    return;
}

std::vector<uint8_t> hex_string_to_bytes(std::string const & input) {
    assert(input.substr(0, 2) == "0x" || input.substr(0, 2) == "T6");
    assert(input.size() % 2 == 0);
    std::vector<uint8_t> res;

    printf("[debug][hex_string_to_bytes] input: %s \n", input.c_str());
    res.resize(input.size() / 2 - 1);

    for (std::size_t i = 1; i < input.size() / 2; ++i) {
        res[i - 1] = (CON(input[2 * i]) << 4) + CON(input[2 * i + 1]);
    }
    // printf("[debug][hex_string_to_bytes] ");
    // for (auto c : res) {
    //     printf("\\0x%x ", c);
    // }
    // printf("\n");
    return res;
}

std::vector<uint8_t> string_to_bytes(std::string const & input) {
    std::vector<uint8_t> res;
    res.resize(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        res[i] = input[i];
    }
    return res;
}

std::string uint8_vector_to_hex_string(std::vector<uint8_t> const & v) {
    std::string result;
    result.reserve(v.size() * 2);  // two digits per character

    static constexpr char hex[] = "0123456789abcdef";

    for (uint8_t c : v) {
        result.push_back(hex[c / 16]);
        result.push_back(hex[c % 16]);
    }

    return result;
}

std::vector<uint8_t> get_sha256(std::vector<uint8_t> const & input) {
    BYTE _input[input.size()];
    for (std::size_t index = 0; index < input.size(); ++index) {
        _input[index] = input[index];
    }

    BYTE _res[SHA256_BLOCK_SIZE];

    SHA256_CTX ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, _input, input.size());
    sha256_final(&ctx, _res);

    std::vector<uint8_t> res;
    res.resize(SHA256_BLOCK_SIZE);
    for (std::size_t index = 0; index < SHA256_BLOCK_SIZE; ++index) {
        res[index] = _res[index];
    }
    return res;
}

std::vector<uint8_t> get_ripemd160(std::vector<uint8_t> const & input) {
    size_t msglen = input.size();
    size_t hashlen = 20;

    const uint8_t * msg = &input[0];
    std::vector<uint8_t> hash(hashlen);
    uint8_t * phash = &hash[0];

    ripemd160_cal(msg, msglen, phash);
    return hash;
}

}  // namespace utils
}  // namespace evm
}  // namespace top