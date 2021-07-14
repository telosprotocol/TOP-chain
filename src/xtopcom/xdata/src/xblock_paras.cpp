// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbase/xutl.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xbasic/xversion.h"
#include "xdata/xblock_paras.h"
#include "xdata/xdata_common.h"

NS_BEG2(top, data)

xblockpara_base_t::xblockpara_base_t() {

}

xblockpara_base_t::xblockpara_base_t(const std::map<std::string, std::string> & values) {
    m_values = values;
}

std::string xblockpara_base_t::dump() const {
    std::stringstream ss;
    ss << "{";
    for (auto & v : m_values) {
        ss << ",k=" << v.first;
        ss << ",v=" << v.second.size();
    }
    ss << "}";
    return ss.str();
}

int32_t xblockpara_base_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    xassert(m_values.size() < 5000);
    const uint16_t count = (uint16_t)m_values.size();
    stream << count;
    for (auto & v : m_values) {
        stream.write_tiny_string(v.first);
        stream << v.second;
    }
    return CALC_LEN();
}
int32_t xblockpara_base_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    uint16_t count = 0;
    stream >> count;
    for (uint16_t i = 0; i < count; i++) {
        std::string key;
        stream.read_tiny_string(key);
        std::string value;
        stream >> value;
        m_values[key] = value;
    }
    return CALC_LEN();
}

// delete value for saving space
void xblockpara_base_t::delete_value(const std::string & key) {
    m_values.erase(key);
}

void xblockpara_base_t::set_value(const std::string & key, const std::string & value) {
    if (key.size() > 2 || value.size() > 4096 || value.empty()) {
        xerror("xblockpara_base_t::set_value para should limit key and value size. key=%s,value_size=%d",
            key.c_str(), value.size());
    }
    if (!value.empty()) {
        m_values[key] = value;
    }
}
std::string xblockpara_base_t::get_value(const std::string & key) const {
    auto iter = m_values.find(key);
    if (iter != m_values.end()) {
        return iter->second;
    }
    return {};
}
void xblockpara_base_t::set_value(const std::string & key, bool value) {
    if (value) {  // false not set
        set_value(key, std::to_string(value));
    } else {
        delete_value(key);
    }
}
bool xblockpara_base_t::get_value_bool(const std::string & key) const {
    std::string value = get_value(key);
    if (!value.empty()) {
        return static_cast<bool>(base::xstring_utl::touint32(value));
    }
    return false;  // default return false
}
void xblockpara_base_t::set_value(const std::string & key, uint32_t value) {
    if (value != 0) {  // zero not set
        set_value(key, std::to_string(value));
    } else {
        delete_value(key);
    }
}
uint32_t xblockpara_base_t::get_value_uint32(const std::string & key) const {
    std::string value = get_value(key);
    if (!value.empty()) {
        return base::xstring_utl::touint32(value);
    }
    return 0;  // default return zero
}
void xblockpara_base_t::set_value(const std::string & key, uint64_t value) {
    if (value != 0) {
        set_value(key, std::to_string(value));
    } else {
        delete_value(key);
    }
}
uint64_t xblockpara_base_t::get_value_uint64(const std::string & key) const {
    std::string value = get_value(key);
    if (!value.empty()) {
        return base::xstring_utl::touint64(value);
    }
    return 0;
}
void xblockpara_base_t::set_value(const std::string & key, uint16_t value) {
    if (value != 0) {
        set_value(key, std::to_string(value));
    } else {
        delete_value(key);
    }
}
uint16_t xblockpara_base_t::get_value_uint16(const std::string & key) const {
    std::string value = get_value(key);
    if (!value.empty()) {
        return static_cast<uint16_t>(base::xstring_utl::touint32(value));
    }
    return 0;
}

NS_END2
