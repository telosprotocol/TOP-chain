// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <vector>
#include <cinttypes>
#include <map>

#include "xdata/xaccount_cmd.h"
#include "xdata/xdata_error.h"

#include "xbase/xdata.h"
#include "xbase/xobject_ptr.h"

#include "xdata/xproperty.h"
#include "xdata/xpropertylog.h"

namespace top { namespace data {

#define CLONE_PROPERTY(prop, name, type)\
do {\
    int32_t err = 0;\
    (prop) = dynamic_xobject_ptr_cast<decltype(prop)::element_type>(get_property_with_type((name), (type), err));\
    if (err != 0) {\
        xwarn("xaccount_cmd get user property fail. %s ret:0x%x", name.c_str(), err);\
        return err;\
    }\
}while(0)

#define CLONE_NATIVE_PROPERTY(native_obj)\
do {\
    int32_t err;\
    (native_obj) = get_native_property(err);\
    if ((native_obj) == nullptr) {\
        xwarn("xaccount_cmd get native property fail. ret:0x%x", err);\
        return err;\
    }\
}while(0)

xaccount_cmd::xaccount_cmd(const std::map<std::string, xdataobj_ptr_t> & property_objs) {
    m_clone_objs = property_objs;
    m_proplogs = make_object_ptr<xaccount_binlog_t>();
}

int32_t xaccount_cmd::do_property_log(const xproperty_log_ptr_t & log) {
    int32_t ret;
    auto logs = log->get_instruction();
    for (auto & proplog : logs) {
        xdbg("xaccount_cmd::do_property_log prop_name:%s", proplog.first.c_str());
        auto instructions = proplog.second.get_logs();
        for (auto & instruction : instructions) {
            xdbg("xaccount_cmd::do_property_log code;%d para1:%s para2:%s",
                instruction.m_op_code, instruction.m_op_para1.c_str(), instruction.m_op_para2.c_str());
            ret = restore_log_instruction(proplog.first.c_str(), instruction);
            if (ret) {
                xwarn("xaccount_cmd::do_property_log restore_log_instruction fail 0x%x", ret);
                return ret;
            }
        }
    }

    return xsuccess;
}

int32_t xaccount_cmd::restore_log_instruction(const std::string & prop_name, xproperty_instruction_t instruction) {
    bool add_flag = false;
    switch (instruction.m_op_code) {
        case xproperty_cmd_type_string_create:
            make_property(prop_name, base::xstring_t::enum_obj_type);
            return xsuccess;
        case xproperty_cmd_type_list_create:
            make_property(prop_name, base::xstrdeque_t::enum_obj_type);
            return xsuccess;
        case xproperty_cmd_type_map_create:
            make_property(prop_name, base::xstrmap_t::enum_obj_type);
            return xsuccess;
        case xproperty_cmd_type_delete:
            return delete_property(prop_name, add_flag);
        case xproperty_cmd_type_string_set:
            return string_set(prop_name, instruction.m_op_para1, add_flag);
        case xproperty_cmd_type_list_push_back:
            return list_push_back(prop_name, instruction.m_op_para1, add_flag);
        case xproperty_cmd_type_list_push_front:
            return list_push_front(prop_name, instruction.m_op_para1, add_flag);
        case xproperty_cmd_type_list_pop_back: {
            std::string pop_value;
            return list_pop_back(prop_name, pop_value, add_flag);
        }
        case xproperty_cmd_type_list_pop_front: {
            std::string pop_value;
            return list_pop_front(prop_name, pop_value, add_flag);
        }
        case xproperty_cmd_type_list_clear: {
            return list_clear(prop_name, add_flag);
        }
        case xproperty_cmd_type_map_set:
            return map_set(prop_name, instruction.m_op_para1, instruction.m_op_para2, add_flag);
        case xproperty_cmd_type_map_remove:
            return map_remove(prop_name, instruction.m_op_para1, add_flag);
        case xproperty_cmd_type_map_clear: {
            return map_clear(prop_name, add_flag);
        }
        default:
            xassert(0);
            return -1;  // TODO(jimmy)
    }
}

int32_t xaccount_cmd::restore_log_instruction(xdataobj_ptr_t & obj, xproperty_instruction_t instruction) {
    bool add_flag = false;
    switch (instruction.m_op_code) {
        case xproperty_cmd_type_string_create:
            make_property(obj, base::xstring_t::enum_obj_type);
            return xsuccess;
        case xproperty_cmd_type_list_create:
            make_property(obj, base::xstrdeque_t::enum_obj_type);
            return xsuccess;
        case xproperty_cmd_type_map_create:
            make_property(obj, base::xstrmap_t::enum_obj_type);
            return xsuccess;
        case xproperty_cmd_type_delete:
            return delete_property(obj);
        case xproperty_cmd_type_string_set:
            return string_set(obj, instruction.m_op_para1);
        case xproperty_cmd_type_list_push_back:
            return list_push_back(obj, instruction.m_op_para1);
        case xproperty_cmd_type_list_push_front:
            return list_push_front(obj, instruction.m_op_para1);
        case xproperty_cmd_type_list_pop_back: {
            std::string pop_value;
            return list_pop_back(obj, pop_value);
        }
        case xproperty_cmd_type_list_pop_front: {
            std::string pop_value;
            return list_pop_front(obj, pop_value);
        }
        case xproperty_cmd_type_list_clear: {
            return list_clear(obj);
        }
        case xproperty_cmd_type_map_set:
            return map_set(obj, instruction.m_op_para1, instruction.m_op_para2);
        case xproperty_cmd_type_map_remove:
            return map_remove(obj, instruction.m_op_para1);
        case xproperty_cmd_type_map_clear: {
            return map_clear(obj);
        }
        default:
            xassert(0);
            return -1;
    }
}

int32_t xaccount_cmd::do_instruction(const std::string & prop_name, xproperty_instruction_t instruction, bool add_flag) {
    switch (instruction.m_op_code) {
        case xproperty_cmd_type_string_create:
            return string_create(prop_name, add_flag);
        case xproperty_cmd_type_list_create:
            return list_create(prop_name, add_flag);
        case xproperty_cmd_type_map_create:
            return map_create(prop_name, add_flag);
        case xproperty_cmd_type_delete:
            return delete_property(prop_name, add_flag);
        case xproperty_cmd_type_string_set:
            return string_set(prop_name, instruction.m_op_para1, add_flag);
        case xproperty_cmd_type_list_push_back:
            return list_push_back(prop_name, instruction.m_op_para1, add_flag);
        case xproperty_cmd_type_list_push_front:
            return list_push_front(prop_name, instruction.m_op_para1, add_flag);
        case xproperty_cmd_type_list_pop_back: {
            std::string pop_value;
            return list_pop_back(prop_name, pop_value, add_flag);
        }
        case xproperty_cmd_type_list_pop_front: {
            std::string pop_value;
            return list_pop_front(prop_name, pop_value, add_flag);
        }
        case xproperty_cmd_type_list_clear: {
            return list_clear(prop_name, add_flag);
        }
        case xproperty_cmd_type_map_set:
            return map_set(prop_name, instruction.m_op_para1, instruction.m_op_para2, add_flag);
        case xproperty_cmd_type_map_remove:
            return map_remove(prop_name, instruction.m_op_para1, add_flag);
        case xproperty_cmd_type_map_clear: {
            return map_clear(prop_name, add_flag);
        }
        default:
            xassert(0);
            return -1;
    }
}

xdataobj_ptr_t xaccount_cmd::get_property(const std::string& prop_name, int32_t & error_code) {
    auto iter = m_clone_objs.find(prop_name);
    if (iter != m_clone_objs.end()) {
        if (iter->second != nullptr) {
            error_code = xsuccess;
            return iter->second;
        }
        error_code = xaccount_cmd_property_has_already_delete;
        return nullptr;
    }
    error_code = xaccount_cmd_property_not_create;
    return nullptr;
}

xdataobj_ptr_t xaccount_cmd::get_property_with_type(const std::string& prop_name, int32_t type, int32_t & error_code) {
    xdataobj_ptr_t obj;
    obj = get_property(prop_name, error_code);
    if (obj == nullptr) {
        xdbg("get_property_with_type empty:%s, %d, %d", prop_name.c_str(), type, error_code);
        return nullptr;
    }

    if (obj->get_obj_type() != type) {
        error_code = xaccount_cmd_property_operate_type_unmatch;
        return nullptr;
    }

    return obj;
}

xstring_ptr_t xaccount_cmd::string_get(const std::string& prop_name) {
    int32_t error_code;
    return dynamic_xobject_ptr_cast<base::xstring_t>(get_property_with_type(prop_name, base::xstring_t::enum_obj_type, error_code));
}
xstrdeque_ptr_t xaccount_cmd::deque_get(const std::string& prop_name) {
    int32_t error_code;
    return dynamic_xobject_ptr_cast<base::xstrdeque_t>(get_property_with_type(prop_name, base::xstrdeque_t::enum_obj_type, error_code));
}
xstrmap_ptr_t xaccount_cmd::map_get(const std::string& prop_name) {
    int32_t error_code;
    return dynamic_xobject_ptr_cast<base::xstrmap_t>(get_property_with_type(prop_name, base::xstrmap_t::enum_obj_type, error_code));
}

void xaccount_cmd::make_property(const std::string & prop_name, int32_t type) {
    if (type == base::xstring_t::enum_obj_type) {
        m_clone_objs[prop_name] = make_object_ptr<base::xstring_t>();
    } else if (type == base::xstrdeque_t::enum_obj_type) {
        m_clone_objs[prop_name] = make_object_ptr<base::xstrdeque_t>();
    } else if (type == base::xstrmap_t::enum_obj_type) {
        m_clone_objs[prop_name] = make_object_ptr<base::xstrmap_t>();
    } else {
        assert(0);
    }
}

void xaccount_cmd::make_property(xdataobj_ptr_t & obj, int32_t type) {
    if (type == base::xstring_t::enum_obj_type) {
        obj = make_object_ptr<base::xstring_t>();
    } else if (type == base::xstrdeque_t::enum_obj_type) {
        obj = make_object_ptr<base::xstrdeque_t>();
    } else if (type == base::xstrmap_t::enum_obj_type) {
        obj = make_object_ptr<base::xstrmap_t>();
    } else {
        assert(0);
    }
}


int32_t xaccount_cmd::create_property(const std::string & prop_name, int32_t type) {
    int32_t error_code;
    xdataobj_ptr_t obj = get_property(prop_name, error_code);
    if (obj != nullptr || error_code != xaccount_cmd_property_not_create) {
        return xaccount_cmd_property_has_already_create;
    }

    make_property(prop_name, type);
    return xsuccess;
}

int32_t xaccount_cmd::delete_property(const std::string & prop_name, bool add_flag) {
    int32_t error_code;
    xdataobj_ptr_t obj = get_property(prop_name, error_code);
    if (error_code == xaccount_cmd_property_has_already_delete || error_code == xaccount_cmd_property_not_create) {
        return error_code;
    }
    m_clone_objs[prop_name] = nullptr;
    if (add_flag) {
        m_proplogs->add_instruction(prop_name, xproperty_cmd_type_delete);
    }
    return xsuccess;
}

int32_t xaccount_cmd::delete_property(xdataobj_ptr_t & obj) {
    obj = nullptr;
    return xsuccess;
}

std::map<std::string, std::string> xaccount_cmd::get_property_hash() {
    std::map<std::string, xdataobj_ptr_t> obj_map = get_change_property();
    std::map<std::string, std::string> prop_hashs;
    for (auto & obj : obj_map) {
        prop_hashs[obj.first] = xhash_base_t::calc_dataunit_hash(obj.second.get());
    }

    return prop_hashs;
}

std::map<std::string, xdataobj_ptr_t> xaccount_cmd::get_change_property() {
    std::map<std::string, xdataobj_ptr_t> obj_map;
    for (auto & v : m_clone_objs) {
        if (m_proplogs->is_property_changed(v.first)) {
            obj_map.emplace(v);
        }
    }
    return obj_map;
}

std::map<std::string, xdataobj_ptr_t> xaccount_cmd::get_all_property() {
    return m_clone_objs;
}

xproperty_log_ptr_t xaccount_cmd::get_property_log() {
    if (m_proplogs->get_property_size() == 0) {
        return nullptr;
    }
    return m_proplogs;
}

int32_t xaccount_cmd::string_create(const std::string& prop_name, bool add_flag) {
    auto ret = create_property(prop_name, base::xstring_t::enum_obj_type);
    if (add_flag && ret == xsuccess) {
        m_proplogs->add_instruction(prop_name, xproperty_cmd_type_string_create);
    }
    return ret;
}

int32_t xaccount_cmd::string_set(const std::string& prop_name, const std::string& value, bool add_flag) {
    // xstring_ptr_t prop;
    xdataobj_ptr_t prop;
    CLONE_PROPERTY(prop, prop_name, base::xstring_t::enum_obj_type);
    int32_t ret = string_set(prop, value);
    if (ret == xsuccess && add_flag) {
        m_proplogs->add_instruction(prop_name, xproperty_cmd_type_string_set, value);
    } else {
        xdbg("property set repeat invalid. prop_name:%s", prop_name.c_str());
    }
    return xsuccess;
}

int32_t xaccount_cmd::string_set(xdataobj_ptr_t & obj, const std::string& value) {
    xstring_ptr_t prop = dynamic_xobject_ptr_cast<base::xstring_t>(obj);
    std::string value_in_prop;
    prop->get(value_in_prop);
    if (value != value_in_prop) {
        prop->set(value);
        return xsuccess;
    }
    return xaccount_cmd_property_set_value_same;
}

int32_t xaccount_cmd::string_get(const std::string& prop_name, std::string& value) {
    xstring_ptr_t prop;
    CLONE_PROPERTY(prop, prop_name, base::xstring_t::enum_obj_type);
    value = prop->get();
    return xsuccess;
}
int32_t xaccount_cmd::string_empty(const std::string& prop_name, bool& empty) {
    xstring_ptr_t prop;
    CLONE_PROPERTY(prop, prop_name, base::xstring_t::enum_obj_type);
    empty = prop->empty();
    return xsuccess;
}
int32_t xaccount_cmd::string_size(const std::string& prop_name, int32_t& size) {
    xstring_ptr_t prop;
    CLONE_PROPERTY(prop, prop_name, base::xstring_t::enum_obj_type);
    size = prop->size();
    return xsuccess;
}

int32_t xaccount_cmd::list_create(const std::string& prop_name, bool add_flag) {
    auto ret = create_property(prop_name, base::xstrdeque_t::enum_obj_type);
    if (add_flag && ret == xsuccess) {
        m_proplogs->add_instruction(prop_name, xproperty_cmd_type_list_create);
    }
    return ret;
}

int32_t xaccount_cmd::list_push_back(const std::string& prop_name, const std::string& value, bool add_flag) {
    // xstrdeque_ptr_t prop;
    xdataobj_ptr_t prop;
    CLONE_PROPERTY(prop, prop_name, base::xstrdeque_t::enum_obj_type);
    bool ret = list_push_back(prop, value);
    if (ret == xsuccess && add_flag) {
        m_proplogs->add_instruction(prop_name, xproperty_cmd_type_list_push_back, value);
    }
    return ret;
}

int32_t xaccount_cmd::list_push_front(const std::string& prop_name, const std::string& value, bool add_flag) {
    // xstrdeque_ptr_t prop;
    xdataobj_ptr_t prop;
    CLONE_PROPERTY(prop, prop_name, base::xstrdeque_t::enum_obj_type);
    int32_t ret = list_push_front(prop, value);
    if (ret == xsuccess && add_flag) {
        m_proplogs->add_instruction(prop_name, xproperty_cmd_type_list_push_front, value);
    }
    return ret;
}
int32_t xaccount_cmd::list_pop_back(const std::string& prop_name, std::string& value, bool add_flag) {
    // xstrdeque_ptr_t prop;
    xdataobj_ptr_t prop;
    CLONE_PROPERTY(prop, prop_name, base::xstrdeque_t::enum_obj_type);
    int32_t ret = list_pop_back(prop, value);
    if (ret == xsuccess && add_flag) {
        m_proplogs->add_instruction(prop_name, xproperty_cmd_type_list_pop_back);
    }
    return ret;
}
int32_t xaccount_cmd::list_pop_front(const std::string& prop_name, std::string& value, bool add_flag) {
    // xstrdeque_ptr_t prop;
    xdataobj_ptr_t prop;
    CLONE_PROPERTY(prop, prop_name, base::xstrdeque_t::enum_obj_type);
    int32_t ret = list_pop_front(prop, value);
    if (ret == xsuccess && add_flag) {
        m_proplogs->add_instruction(prop_name, xproperty_cmd_type_list_pop_front, value);
    }
    return ret;
}
int32_t xaccount_cmd::list_clear(const std::string& prop_name, bool add_flag) {
    // xstrdeque_ptr_t prop;
    xdataobj_ptr_t prop;
    CLONE_PROPERTY(prop, prop_name, base::xstrdeque_t::enum_obj_type);
    int32_t ret = list_clear(prop);
    if (ret == xsuccess && add_flag) {
        m_proplogs->add_instruction(prop_name, xproperty_cmd_type_list_clear);
    }
    return ret;
}

int32_t xaccount_cmd::list_push_back(xdataobj_ptr_t & obj, const std::string& value) {
    xstrdeque_ptr_t prop = dynamic_xobject_ptr_cast<base::xstrdeque_t>(obj);
    bool ret = prop->push_back(value);
    return (ret == true) ? xsuccess : xaccount_cmd_property_operate_fail;
}

int32_t xaccount_cmd::list_push_front(xdataobj_ptr_t & obj, const std::string& value) {
    xstrdeque_ptr_t prop = dynamic_xobject_ptr_cast<base::xstrdeque_t>(obj);
    bool ret = prop->push_front(value);
    return (ret == true) ? xsuccess : xaccount_cmd_property_operate_fail;
}
int32_t xaccount_cmd::list_pop_back(xdataobj_ptr_t & obj, std::string& value) {
    xstrdeque_ptr_t prop = dynamic_xobject_ptr_cast<base::xstrdeque_t>(obj);
    bool ret = prop->pop_back(value);
    return (ret == true) ? xsuccess : xaccount_cmd_property_operate_fail;
}
int32_t xaccount_cmd::list_pop_front(xdataobj_ptr_t & obj, std::string& value) {
    xstrdeque_ptr_t prop = dynamic_xobject_ptr_cast<base::xstrdeque_t>(obj);
    bool ret = prop->pop_front(value);
    return (ret == true) ? xsuccess : xaccount_cmd_property_operate_fail;
}
int32_t xaccount_cmd::list_clear(xdataobj_ptr_t & obj) {
    xstrdeque_ptr_t prop = dynamic_xobject_ptr_cast<base::xstrdeque_t>(obj);
    bool ret = prop->clear();
    return (ret == true) ? xsuccess : xaccount_cmd_property_operate_fail;
}

int32_t xaccount_cmd::list_get_back(const std::string& prop_name, std::string & value) {
    xstrdeque_ptr_t prop;
    CLONE_PROPERTY(prop, prop_name, base::xstrdeque_t::enum_obj_type);
    bool ret = prop->get_back(value);
    return (ret == true) ? xsuccess : xaccount_cmd_property_operate_fail;
}
int32_t xaccount_cmd::list_get_front(const std::string& prop_name, std::string & value) {
    xstrdeque_ptr_t prop;
    CLONE_PROPERTY(prop, prop_name, base::xstrdeque_t::enum_obj_type);
    bool ret = prop->get_front(value);
    return (ret == true) ? xsuccess : xaccount_cmd_property_operate_fail;
}
int32_t xaccount_cmd::list_get(const std::string& prop_name, const uint32_t index, std::string & value) {
    xstrdeque_ptr_t prop;
    CLONE_PROPERTY(prop, prop_name, base::xstrdeque_t::enum_obj_type);
    bool ret = prop->get(index, value);
    return (ret == true) ? xsuccess : xaccount_cmd_property_operate_fail;
}
int32_t xaccount_cmd::list_empty(const std::string& prop_name, bool& empty) {
    xstrdeque_ptr_t prop;
    CLONE_PROPERTY(prop, prop_name, base::xstrdeque_t::enum_obj_type);
    empty = prop->empty();
    return xsuccess;
}
int32_t xaccount_cmd::list_size(const std::string& prop_name, int32_t& size) {
    xstrdeque_ptr_t prop;
    CLONE_PROPERTY(prop, prop_name, base::xstrdeque_t::enum_obj_type);
    size = prop->size();
    return xsuccess;
}
int32_t xaccount_cmd::list_get_range(const std::string &prop_name, int32_t start, int32_t stop, std::vector<std::string> &values) {
    xstrdeque_ptr_t prop;
    CLONE_PROPERTY(prop, prop_name, base::xstrdeque_t::enum_obj_type);
    int32_t size = prop->size();
    if (stop > size) {
        stop = size;
    }
    for (int32_t i=start; i < stop; i++) {
        std::string value;
        auto ret = prop->get(i, value);
        assert(ret);
        values.push_back(value);
    }

    return xsuccess;
}
int32_t xaccount_cmd::list_get_all(const std::string &prop_name, std::vector<std::string> &values) {
    xstrdeque_ptr_t prop;
    CLONE_PROPERTY(prop, prop_name, base::xstrdeque_t::enum_obj_type);
    int32_t size = prop->size();
    for (int32_t i=0; i < size; i++) {
        std::string value;
        auto ret = prop->get(i, value);
        assert(ret);
        values.push_back(value);
    }
    return xsuccess;
}

int32_t xaccount_cmd::list_copy_get(const std::string &prop_name, std::deque<std::string> & deque) {
    xstrdeque_ptr_t prop;
    CLONE_PROPERTY(prop, prop_name, base::xstrdeque_t::enum_obj_type);
    deque = prop->get_deque();
    return xsuccess;
}

int32_t xaccount_cmd::map_create(const std::string& prop_name, bool add_flag) {
    auto ret = create_property(prop_name, base::xstrmap_t::enum_obj_type);
    if (add_flag && ret == xsuccess) {
        m_proplogs->add_instruction(prop_name, xproperty_cmd_type_map_create);
    }
    return ret;
}

int32_t xaccount_cmd::map_get(const std::string & prop_name, const std::string & field, std::string & value) {
    xstrmap_ptr_t prop;
    CLONE_PROPERTY(prop, prop_name, base::xstrmap_t::enum_obj_type);
    bool ret = prop->get(field, value);
    return (ret == true) ? xsuccess : xaccount_cmd_property_map_field_not_create;
}
int32_t xaccount_cmd::map_set(const std::string & prop_name, const std::string & field, const std::string & value, bool add_flag) {
    xstrmap_ptr_t prop;
    CLONE_PROPERTY(prop, prop_name, base::xstrmap_t::enum_obj_type);
    prop->set(field, value);
    if (add_flag) {
        m_proplogs->add_instruction(prop_name, xproperty_cmd_type_map_set, field, value);
    }
    return xsuccess;
}
int32_t xaccount_cmd::map_remove(const std::string & prop_name, const std::string & field, bool add_flag) {
    xstrmap_ptr_t prop;
    CLONE_PROPERTY(prop, prop_name, base::xstrmap_t::enum_obj_type);
    bool ret = prop->remove(field);
    if (add_flag && ret) {
        m_proplogs->add_instruction(prop_name, xproperty_cmd_type_map_remove, field);
    }
    return (ret == true) ? xsuccess : xaccount_cmd_property_operate_fail;
}
int32_t xaccount_cmd::map_clear(const std::string & prop_name, bool add_flag) {
    xstrmap_ptr_t prop;
    CLONE_PROPERTY(prop, prop_name, base::xstrmap_t::enum_obj_type);
    bool ret = prop->clear();
    if (add_flag && ret) {
        m_proplogs->add_instruction(prop_name, xproperty_cmd_type_map_clear);
    }
    return xsuccess;
}

int32_t xaccount_cmd::map_set(xdataobj_ptr_t & obj, const std::string & field, const std::string & value) {
    xstrmap_ptr_t prop = dynamic_xobject_ptr_cast<base::xstrmap_t>(obj);
    prop->set(field, value);
    return xsuccess;
}
int32_t xaccount_cmd::map_remove(xdataobj_ptr_t & obj, const std::string & field) {
    xstrmap_ptr_t prop = dynamic_xobject_ptr_cast<base::xstrmap_t>(obj);
    bool ret = prop->remove(field);
    return (ret == true) ? xsuccess : xaccount_cmd_property_operate_fail;
}
int32_t xaccount_cmd::map_clear(xdataobj_ptr_t & obj) {
    xstrmap_ptr_t prop = dynamic_xobject_ptr_cast<base::xstrmap_t>(obj);
    bool ret = prop->clear();
    return xsuccess;
}

int32_t xaccount_cmd::map_empty(const std::string & prop_name, bool& empty) {
    xstrmap_ptr_t prop;
    CLONE_PROPERTY(prop, prop_name, base::xstrmap_t::enum_obj_type);
    empty = prop->empty();
    return xsuccess;
}
int32_t xaccount_cmd::map_size(const std::string & prop_name, int32_t& size) {
    xstrmap_ptr_t prop;
    CLONE_PROPERTY(prop, prop_name, base::xstrmap_t::enum_obj_type);
    size = prop->size();
    return xsuccess;
}
int32_t xaccount_cmd::map_copy_get(const std::string & prop_name, std::map<std::string, std::string> & map) {
    xstrmap_ptr_t prop;
    CLONE_PROPERTY(prop, prop_name, base::xstrmap_t::enum_obj_type);
    map = prop->get_map();
    return xsuccess;
}

}  // namespace store
}  // namespace top
