#include "xevm_contract_runtime/xevm_variant_bytes.h"

#include "assert.h"

NS_BEG3(top, contract_runtime, evm)

#define CON(x) (std::isdigit((x)) ? ((x) - '0') : (std::tolower((x)) - 'W'))

std::string evm_to_top_address(std::string const & input) {
    if (input.substr(0, 2) == ETH_ACCOUNT_PREFIX) {
        return T6_ACCOUNT_PREFIX + input.substr(2);
    }
    if (input.substr(0, 6) == T6_ACCOUNT_PREFIX) {
        return input;
    }
    return T6_ACCOUNT_PREFIX + input;
}

xvariant_bytes::xvariant_bytes() {
}

xvariant_bytes::xvariant_bytes(xbytes_t input) : m_data{input} {
}

xvariant_bytes::xvariant_bytes(std::string const & input, bool is_hex) {
    if (is_hex) {
        // hex string to xbytes_t:
        assert(input.size() % 2 == 0);
        if (input.size() % 2 != 0)
            return;
        std::string input_data;
        if (input.size() > 2 && input.substr(0, 2) == "0x") {
            input_data = input.substr(2);
        } else {
            input_data = input;
        }
        m_data.resize(input_data.size() / 2);
        for (std::size_t i = 0; i < input_data.size() / 2; ++i) {
            m_data[i] = (CON(input_data[2 * i]) << 4) + CON(input_data[2 * i + 1]);
        }
    } else {
        // string to xbytes_t:
        m_data.resize(input.size());
        for (std::size_t i = 0; i < input.size(); ++i) {
            m_data[i] = input[i];
        }
    }
}

xbytes_t xvariant_bytes::to_bytes() {
    return m_data;
}

std::string xvariant_bytes::to_string() {
    // bytes_to_string
    std::string result;
    result.resize(m_data.size());
    for (std::size_t i = 0; i < m_data.size(); ++i) {
        result[i] = m_data[i];
    }
    return result;
}

std::string xvariant_bytes::to_hex_string(std::string const & PREFIX) {
    if (m_data.empty()) {
        return "";
    }
    // bytes_to_hex_string
    std::string result;
    result.reserve(m_data.size() * 2);  // two digits per character

    static constexpr char hex[] = "0123456789abcdef";

    for (uint8_t c : m_data) {
        result.push_back(hex[c / 16]);
        result.push_back(hex[c % 16]);
    }

    return PREFIX + result;
}
NS_END3