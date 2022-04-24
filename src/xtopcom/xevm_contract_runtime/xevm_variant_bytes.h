#pragma once

#include "assert.h"
#include "xbase/xns_macro.h"
#include "xbasic/xbyte_buffer.h"

NS_BEG3(top, contract_runtime, evm)

#define CON(x) (std::isdigit((x)) ? ((x) - '0') : (std::tolower((x)) - 'W'))
#define T6_ACCOUNT_PREFIX "T60004"
#define ETH_ACCOUNT_PREFIX "0x"

std::string evm_to_top_address(std::string const & input);

class xvariant_bytes {
private:
    xbytes_t m_data;

public:
    xvariant_bytes();
    xvariant_bytes(xbytes_t input);
    xvariant_bytes(std::string const & input, bool is_hex);

public:
    xbytes_t to_bytes();
    std::string to_string();
    std::string to_hex_string(std::string const & PREFIX = "");
};

NS_END3