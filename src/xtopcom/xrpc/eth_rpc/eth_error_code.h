// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <map>
namespace eth {
enum enum_error_code {
    enum_invalid_argument_hex,
    enum_invalid_address,
};
struct ErrorMessage {
    ErrorMessage(int32_t code, const std::string& message):m_code(code), m_message(message) {}
    ErrorMessage():m_code(0) {}
    int32_t m_code;
    std::string m_message;
};
class EthErrorCode {
private:
    std::map<enum_error_code, ErrorMessage> m_error_info;
public:
    EthErrorCode() {
        m_error_info.emplace(enum_invalid_argument_hex, ErrorMessage{-32602, std::string("invalid argument: json: cannot unmarshal hex string of odd length of type common.Address")});
        m_error_info.emplace(enum_invalid_address, ErrorMessage{-32801, "invalid address"});
    }
    bool get_error(enum_error_code error_id, ErrorMessage& msg) {
        if (m_error_info.find(error_id) == m_error_info.end())
            return false;
        msg = m_error_info[error_id];
        return true;
    }
};
}