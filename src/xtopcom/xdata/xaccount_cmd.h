// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>
#include <map>

#include "xbase/xdata.h"
#include "xbase/xobject_ptr.h"

#include "xdata/xproperty.h"
#include "xdata/xpropertylog.h"


namespace top { namespace data {

using data::xproperty_instruction_t;
using data::xaccount_binlog_t;
using data::xproperty_op_code_t;
using data::xproperty_log_ptr_t;

// modify blockchain state according to property binlog
// caller need verify whether it is duplicated set/modify
class xaccount_cmd {
 public:
    xaccount_cmd(const std::map<std::string, xdataobj_ptr_t> & property_objs);
    std::map<std::string, std::string> get_property_hash();
    std::map<std::string, xdataobj_ptr_t> get_change_property();
    std::map<std::string, xdataobj_ptr_t> const & get_all_property() const noexcept;
    xproperty_log_ptr_t get_property_log();

    int32_t do_property_log(const xproperty_log_ptr_t & logs);
    int32_t restore_log_instruction(const std::string & prop_name, xproperty_instruction_t instruction);
    int32_t restore_log_instruction(xdataobj_ptr_t & obj, xproperty_instruction_t instruction);
    int32_t do_instruction(const std::string & prop_name, xproperty_instruction_t instruction, bool add_flag = true);

    xstring_ptr_t string_get(const std::string& prop_name);
    xstrdeque_ptr_t deque_get(const std::string& prop_name);
    xstrmap_ptr_t map_get(const std::string& prop_name);

    xdataobj_ptr_t get_property(const std::string& prop_name, int32_t & error_code);

    int32_t string_create(const std::string& prop_name, bool add_flag = true);
    int32_t string_set(const std::string& prop_name, const std::string& value, bool add_flag = true);
    int32_t string_get(const std::string& prop_name, std::string& value);
    int32_t string_empty(const std::string& prop_name, bool& empty);
    int32_t string_size(const std::string& prop_name, int32_t& size);

    int32_t list_create(const std::string& prop_name, bool add_flag = true);
    int32_t list_push_back(const std::string& prop_name, const std::string& value, bool add_flag = true);
    int32_t list_push_front(const std::string& prop_name, const std::string& value, bool add_flag = true);
    int32_t list_pop_back(const std::string& prop_name, std::string& value, bool add_flag = true);
    int32_t list_pop_front(const std::string& prop_name, std::string& value, bool add_flag = true);
    int32_t list_clear(const std::string& prop_name, bool add_flag = true);
    int32_t list_get_back(const std::string& prop_name, std::string & value);
    int32_t list_get_front(const std::string& prop_name, std::string & value);
    int32_t list_get(const std::string& prop_name, const uint32_t index, std::string & value);
    int32_t list_empty(const std::string& prop_name, bool& empty);
    int32_t list_size(const std::string& prop_name, int32_t& size);
    int32_t list_get_range(const std::string &prop_name, int32_t start, int32_t stop, std::vector<std::string> &values);
    int32_t list_get_all(const std::string &prop_name, std::vector<std::string> &values);
    int32_t list_copy_get(const std::string &prop_name, std::deque<std::string> & deque);

    int32_t map_create(const std::string& prop_name, bool add_flag = true);
    int32_t map_get(const std::string & prop_name, const std::string & field, std::string & value);
    int32_t map_set(const std::string & prop_name, const std::string & field, const std::string & value, bool add_flag = true);
    int32_t map_remove(const std::string & prop_name, const std::string & field, bool add_flag = true);
    int32_t map_clear(const std::string & prop_name, bool add_flag = true);
    int32_t map_empty(const std::string & prop_name, bool& empty);
    int32_t map_size(const std::string & prop_name, int32_t& size);
    int32_t map_copy_get(const std::string & key, std::map<std::string, std::string> & map);

 private:
    xdataobj_ptr_t get_property_with_type(const std::string& prop_name, int32_t type, int32_t & error_code);
    int32_t create_property(const std::string & prop_name, int32_t type);
    void make_property(const std::string & prop_name, int32_t type);
    void make_property(xdataobj_ptr_t & obj, int32_t type);
    int32_t delete_property(const std::string & prop_name, bool add_flag = true);
    int32_t delete_property(xdataobj_ptr_t & obj);
    int32_t string_set(xdataobj_ptr_t & obj, const std::string& value);
    int32_t list_push_back(xdataobj_ptr_t & obj, const std::string& value);
    int32_t list_push_front(xdataobj_ptr_t & obj, const std::string& value);
    int32_t list_pop_back(xdataobj_ptr_t & obj, std::string& value);
    int32_t list_pop_front(xdataobj_ptr_t & obj, std::string& value);
    int32_t list_clear(xdataobj_ptr_t & obj);
    int32_t map_set(xdataobj_ptr_t & obj, const std::string & field, const std::string & value);
    int32_t map_remove(xdataobj_ptr_t & obj, const std::string & field);
    int32_t map_clear(xdataobj_ptr_t & obj);


 private:
    xproperty_log_ptr_t         m_proplogs;
    std::map<std::string, xdataobj_ptr_t>   m_clone_objs;
};

using xaccount_cmd_ptr_t = std::shared_ptr<xaccount_cmd>;

}  // namespace store
}  // namespace top
