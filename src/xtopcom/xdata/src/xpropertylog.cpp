// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <vector>

#include "xbase/xutl.h"
#include "xdata/xpropertylog.h"

namespace top { namespace data {

REG_CLS(xaccount_binlog_t);

int32_t xaccount_binlog_t::add_instruction(const std::string & name, xproperty_instruction_t instruction) {
    auto iter = m_property_logs.find(name);
    if (iter != m_property_logs.end()) {
        iter->second.add_binlog(instruction);
        if (iter->second.empty()) {
            m_property_logs.erase(iter);
        }
    } else {
        xproperty_binlog_t prop_log;
        prop_log.add_binlog(instruction);
        m_property_logs[name] = prop_log;
    }
    //xdbg("[xaccount_binlog_t::add_instruction] name: %s, instruction: opcode: %d, para1: %s, para2: %s",
    //    name.c_str(), instruction.m_op_code, instruction.m_op_para1.c_str(), to_hex_str(instruction.m_op_para2).c_str());
    add_modified_count();
    return 0;
}

int32_t xaccount_binlog_t::add_instruction(const std::string & name, xproperty_op_code_t op_code) {
    xproperty_instruction_t instruction(op_code);
    return add_instruction(name, instruction);
}

int32_t xaccount_binlog_t::add_instruction(const std::string & name, xproperty_op_code_t op_code, const std::string & op_para1) {
    xproperty_instruction_t instruction(op_code, op_para1);
    return add_instruction(name, instruction);
}

int32_t xaccount_binlog_t::add_instruction(const std::string & name, xproperty_op_code_t op_code, const std::string & op_para1, const std::string & op_para2) {
    xproperty_instruction_t instruction(op_code, op_para1, op_para2);
    return add_instruction(name, instruction);
}

std::map<std::string, xproperty_binlog_t> const & xaccount_binlog_t::get_instruction() const noexcept {
    return m_property_logs;
}

bool xaccount_binlog_t::is_property_changed(const std::string & name) {
    if (m_property_logs.find(name) != m_property_logs.end()) {
        return true;
    }
    return false;
}

void xaccount_binlog_t::print_property_log() {
    xdbg("m_unit_height %d", m_unit_height);
    for (auto & proplog : m_property_logs) {
        xdbg("prop_name:%s", proplog.first.c_str());
        auto instructions = proplog.second.get_logs();
        for (auto & instruction : instructions) {
            std::string str1{};
            std::string str2{};
            if (!instruction.m_op_para1.empty()) {
                str1 = base::xstring_utl::base64_encode((const uint8_t*)instruction.m_op_para1.data(), instruction.m_op_para1.size());
            }
            if (!instruction.m_op_para2.empty()) {
                str2 = base::xstring_utl::base64_encode((const uint8_t*)instruction.m_op_para2.data(), instruction.m_op_para2.size());
            }
            xdbg("do_property code:%d para1:%s para2:%s",
                instruction.m_op_code, str1.c_str(), str2.c_str());
        }
    }
}

}  // namespace data
}  // namespace top
