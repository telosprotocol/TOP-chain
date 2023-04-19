// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xaction.h"

NS_BEG3(top, data, details)

void xtop_action::account_address(common::xaccount_address_t account_address) {
    m_account_addr = std::move(account_address);
}

common::xaccount_address_t const & xtop_action::account_address() const noexcept {
    return m_account_addr;
}

// Old APIs

int32_t xtop_action::serialize_write(base::xstream_t & stream, bool const is_write_without_len) const {
    const int32_t begin_pos = stream.size();
    stream << m_action_hash;
    stream << m_action_type;
    if (is_write_without_len) {
        constexpr uint16_t action_size = 0;
        stream << action_size;
    } else {
        stream << m_action_size;
    }
    stream << m_account_addr;
    stream << m_action_name;
    stream << m_action_param;
    stream << m_action_ext;
    stream << m_action_authorization;
    const int32_t end_pos = stream.size();
    return (end_pos - begin_pos);
}

int32_t xtop_action::serialize_read(base::xstream_t & stream) {
    const int32_t begin_pos = stream.size();
    stream >> m_action_hash;
    stream >> m_action_type;
    stream >> m_action_size;
    stream >> m_account_addr;
    stream >> m_action_name;
    stream >> m_action_param;
    stream >> m_action_ext;
    stream >> m_action_authorization;
    const int32_t end_pos = stream.size();
    return (begin_pos - end_pos);
}

std::string xtop_action::to_string() const {
    std::string str;
    str += std::to_string(m_action_hash);
    str += std::to_string(m_action_type);
    str += std::to_string(m_action_size);
    str += m_account_addr.to_string();
    str += m_action_name;
    str += to_hex_str(m_action_param);
    str += m_action_authorization;
    str += m_action_ext;
    return str;
}

void xtop_action::set_action_hash(uint32_t const hash) {
    m_action_hash = hash;
};
uint32_t xtop_action::get_action_hash() const {
    return m_action_hash;
};
void xtop_action::set_action_type(enum_xaction_type const type) {
    m_action_type = type;
};
enum_xaction_type xtop_action::get_action_type() const {
    return m_action_type;
};
void xtop_action::set_action_size(uint16_t const size) {
    m_action_size = size;
};
uint16_t xtop_action::get_action_size() const {
    return m_action_size;
};
// void set_account_addr(const std::string & addr) {m_account_addr = addr;};
// const std::string & get_account_addr() const {return m_account_addr;};
void xtop_action::set_action_name(const std::string & action_name) {
    m_action_name = action_name;
};
const std::string & xtop_action::get_action_name() const {
    return m_action_name;
};
void xtop_action::set_action_param(const std::string & action_param) {
    m_action_param = action_param;
};
const std::string & xtop_action::get_action_param() const {
    return m_action_param;
};
void xtop_action::clear_action_param() {
    m_action_param.clear();
};
void xtop_action::set_action_ext(const std::string & action_ext) {
    m_action_ext = action_ext;
};
const std::string & xtop_action::get_action_ext() const {
    return m_action_ext;
};
void xtop_action::set_action_authorization(const std::string & action_authorization) {
    m_action_authorization = action_authorization;
};
const std::string & xtop_action::get_action_authorization() const {
    return m_action_authorization;
};
size_t xtop_action::get_ex_alloc_size() const {
    return get_size(m_account_addr.to_string()) + get_size(m_action_name) + get_size(m_action_param) + get_size(m_action_ext) + get_size(m_action_authorization);
}

NS_END3
