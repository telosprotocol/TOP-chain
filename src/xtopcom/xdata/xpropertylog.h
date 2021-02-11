// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>
#include <map>

#include "xdata/xdata_common.h"
#include "xdata/xhash_base.hpp"

#include "xbasic/xobject_ptr.h"
#include "xbasic/xdataobj_base.hpp"
#include "xbasic/xversion.h"
#include "xbasic/xserialize_face.h"

namespace top { namespace data {

enum xproperty_op_code_t
: uint8_t {
    xproperty_cmd_type_invalid,
    xproperty_cmd_type_delete,
    xproperty_cmd_type_int64_create,
    xproperty_cmd_type_int64_add,
    xproperty_cmd_type_int64_sub,
    xproperty_cmd_type_string_create,
    xproperty_cmd_type_string_set,
    xproperty_cmd_type_list_create,
    xproperty_cmd_type_list_push_back,
    xproperty_cmd_type_list_push_front,
    xproperty_cmd_type_list_pop_back,
    xproperty_cmd_type_list_pop_front,
    xproperty_cmd_type_list_clear,
    xproperty_cmd_type_map_create,
    xproperty_cmd_type_map_set,
    xproperty_cmd_type_map_remove,
    xproperty_cmd_type_map_clear,

    xproperty_cmd_type_max
};

// one property instruction
class xproperty_instruction_t : public xserializable_based_on<void> {
 public:
    xproperty_instruction_t() = default;
    explicit xproperty_instruction_t(xproperty_op_code_t op_code)
    : m_op_code(op_code) {
    }
    xproperty_instruction_t(xproperty_op_code_t op_code, const std::string & op_para1)
    : m_op_code(op_code), m_op_para1(op_para1) {
    }
    xproperty_instruction_t(xproperty_op_code_t op_code, const std::string & op_para1, const std::string & op_para2)
    : m_op_code(op_code), m_op_para1(op_para1), m_op_para2(op_para2) {
    }

    int32_t do_write(base::xstream_t & stream) const override {
        KEEP_SIZE();
        SERIALIZE_FIELD_BT(m_op_code);
        SERIALIZE_FIELD_BT(m_op_para1);
        SERIALIZE_FIELD_BT(m_op_para2);
        return CALC_LEN();
    }

    int32_t do_read(base::xstream_t & stream) override {
        KEEP_SIZE();
        DESERIALIZE_FIELD_BT(m_op_code);
        DESERIALIZE_FIELD_BT(m_op_para1);
        DESERIALIZE_FIELD_BT(m_op_para2);
        return CALC_LEN();
    }

 public:
    xproperty_op_code_t m_op_code;
    std::string m_op_para1{};
    std::string m_op_para2{};
};

// one property instruction set
class xproperty_binlog_t : public xserializable_based_on<void> {
 public:
    bool empty() { return m_logs.empty(); }

    int32_t    do_write(base::xstream_t & stream) const override {
        KEEP_SIZE();
        SERIALIZE_CONTAINER(m_logs) {
            item.serialize_to(stream);
        }
        return CALC_LEN();
    }
    int32_t    do_read(base::xstream_t & stream) override {
        KEEP_SIZE();
        DESERIALIZE_CONTAINER(m_logs) {
            xproperty_instruction_t item;
            item.serialize_from(stream);
            m_logs.emplace_back(item);
        }

        return CALC_LEN();
    }

    void add_binlog(xproperty_instruction_t instruction) {
        if (instruction.m_op_code == xproperty_cmd_type_list_pop_back) {
            auto ret = delete_binlog_deque_push(xproperty_cmd_type_list_push_back);
            if (ret) {
                return;
            }
        } else if (instruction.m_op_code == xproperty_cmd_type_list_pop_front) {
            auto ret = delete_binlog_deque_push(xproperty_cmd_type_list_push_front);
            if (ret) {
                return;
            }
        } else if (instruction.m_op_code == xproperty_cmd_type_map_remove) {
            auto ret = delete_binlog_map_set(xproperty_cmd_type_map_set, instruction.m_op_para1);
            if (ret) {
                return;
            }
        } else if (instruction.m_op_code == xproperty_cmd_type_string_set) {
            delete_binlog_string_set(xproperty_cmd_type_string_set);
        } else if (instruction.m_op_code == xproperty_cmd_type_list_clear) {
            delete_binlog_list_clear();
        } else if (instruction.m_op_code == xproperty_cmd_type_map_clear) {
            delete_binlog_map_clear();
        }

        m_logs.push_back(instruction);
    }
    void delete_binlog_string_set(xproperty_op_code_t op_code) {
        if (m_logs.empty()) {
            return;
        }
        if (m_logs.back().m_op_code == op_code) {
            m_logs.pop_back();
            return;
        }
    }
    void delete_binlog_list_clear() {
        while (!m_logs.empty()) {
            if (m_logs.back().m_op_code == xproperty_cmd_type_list_create) {
                return;
            }
            m_logs.pop_back();
        }
    }
    void delete_binlog_map_clear() {
        while (!m_logs.empty()) {
            if (m_logs.back().m_op_code == xproperty_cmd_type_map_create) {
                return;
            }
            m_logs.pop_back();
        }
    }
    bool delete_binlog_deque_push(xproperty_op_code_t op_code) {
        if (m_logs.empty()) {
            return false;
        }
        if (m_logs.back().m_op_code == op_code) {
            m_logs.pop_back();
            return true;
        }
        return false;
    }
    bool delete_binlog_map_set(xproperty_op_code_t op_code, const std::string & key) {
        if (m_logs.empty()) {
            return false;
        }

        for (auto iter = m_logs.begin(); iter != m_logs.end(); iter++) {
            if (iter->m_op_code == op_code && iter->m_op_para1 == key) {
                m_logs.erase(iter);
                return true;
            }
        }
        return false;
    }

    std::vector<xproperty_instruction_t> get_logs() {
        return m_logs;
    }

 public:
    std::vector<xproperty_instruction_t> m_logs;
};

class xaccount_binlog_t final : public xbase_dataobj_t<xaccount_binlog_t, xdata_type_account_binlog> {
 protected:
    ~xaccount_binlog_t() override {}

 public:
    xaccount_binlog_t() {
        add_modified_count();
    }
    explicit xaccount_binlog_t(uint64_t unit_height) : m_unit_height(unit_height) {
        add_modified_count();
    }

 public:
    int32_t get_property_size() { return (int32_t)m_property_logs.size(); }
    int32_t get_instruction_size()const {
        int32_t size = 0;
        for (auto & v : m_property_logs) {
            size += (int32_t)v.second.m_logs.size();
        }
        return size;
    }
    int32_t add_instruction(const std::string & name, xproperty_instruction_t instruction);
    int32_t add_instruction(const std::string & name, xproperty_op_code_t op_code);
    int32_t add_instruction(const std::string & name, xproperty_op_code_t op_code, const std::string & op_para1);
    int32_t add_instruction(const std::string & name, xproperty_op_code_t op_code, const std::string & op_para1, const std::string & op_para2);
    std::map<std::string, xproperty_binlog_t> const & get_instruction() const noexcept;
    bool is_property_changed(const std::string & name);
    void print_property_log();
    uint64_t get_unit_height()const {return m_unit_height;}

 public:
    int32_t    do_write(base::xstream_t & stream) override {
        KEEP_SIZE();
        SERIALIZE_FIELD_BT(m_unit_height);
        SERIALIZE_CONTAINER(m_property_logs) {
            SERIALIZE_FIELD_BT(item.first);
            SERIALIZE_FIELD_DU(item.second);
        }
        return CALC_LEN();
    }
    int32_t    do_read(base::xstream_t & stream) override {
        KEEP_SIZE();
        DESERIALIZE_FIELD_BT(m_unit_height);
        DESERIALIZE_CONTAINER(m_property_logs) {
            std::string key;
            xproperty_binlog_t value;
            DESERIALIZE_FIELD_BT(key);
            DESERIALIZE_FIELD_DU(value);
            m_property_logs.emplace(std::make_pair(std::move(key), std::move(value)));
        }
        return CALC_LEN();
    }

 private:
    uint64_t    m_unit_height{0};
    std::map<std::string, xproperty_binlog_t> m_property_logs;
};

using xproperty_log_ptr_t = xobject_ptr_t<xaccount_binlog_t>;
using xaccount_binlog_ptr_t = xobject_ptr_t<xaccount_binlog_t>;

}  // namespace data
}  // namespace top
