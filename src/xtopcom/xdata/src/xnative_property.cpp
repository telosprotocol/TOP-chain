// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <vector>
#include <map>
#include <iostream>

#include "xdata/xdata_error.h"
#include "xdata/xnative_property.h"
#include "xdata/xdata_common.h"

#include "xbase/xdata.h"
#include "xbase/xobject_ptr.h"

#include "xbasic/xversion.h"

namespace top { namespace data {

int32_t xnative_property_t::do_write(base::xstream_t & stream) const {
    KEEP_SIZE();
    SERIALIZE_CONTAINER(m_objs) {
        SERIALIZE_FIELD_BT(item.first);
        xassert(item.second != nullptr);
        item.second->serialize_to(stream);
    }
    return CALC_LEN();
}

int32_t xnative_property_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    DESERIALIZE_CONTAINER(m_objs) {
        std::string name;
        DESERIALIZE_FIELD_BT(name);

        xdataunit_ptr_t property;
        REFLECT_DESERIALIZE_NONNULL_PTR(property, base::xdataunit_t);
        m_objs[name] = property;
    }

    return CALC_LEN();
}

bool xnative_property_t::is_property_exist(const std::string& prop_name) const {
    auto iter = m_objs.find(prop_name);
    if (iter == m_objs.end()) {
        return false;
    }
    return true;
}

xdataunit_ptr_t xnative_property_t::property_get(const std::string& prop_name, int32_t type) const {
    auto iter = m_objs.find(prop_name);
    if (iter == m_objs.end()) {
        return nullptr;
    }
    if (iter->second->get_obj_type() != type) {
        xerror("obj type not match. %d %d", iter->second->get_obj_type(), type);
        return nullptr;
    }
    return iter->second;
}

xstring_ptr_t xnative_property_t::string_get(const std::string& prop_name) const {
    return dynamic_xobject_ptr_cast<base::xstring_t>(property_get(prop_name, base::xstring_t::enum_obj_type));
}
xstrdeque_ptr_t xnative_property_t::deque_get(const std::string& prop_name) const {
    return dynamic_xobject_ptr_cast<base::xstrdeque_t>(property_get(prop_name, base::xstrdeque_t::enum_obj_type));
}
xstrmap_ptr_t xnative_property_t::map_get(const std::string& prop_name) const {
    return dynamic_xobject_ptr_cast<base::xstrmap_t>(property_get(prop_name, base::xstrmap_t::enum_obj_type));
}

int32_t xnative_property_t::property_set(const std::string& prop_name, const xdataunit_ptr_t & property) {
    auto iter = m_objs.find(prop_name);
    if (iter == m_objs.end()) {
        m_objs[prop_name] = property;
        m_modify_objs[prop_name] = property;
        return xsuccess;
    }
    if (iter->second != property) {
        xerror("dataunit is not same");
        return -1;
    }
    m_modify_objs[prop_name] = property;
    return xsuccess;
}

int32_t xnative_property_t::native_string_set(const std::string& prop_name, const std::string& value) {
    auto property = string_get(prop_name);
    if (property == nullptr) {
        property = make_object_ptr<base::xstring_t>();
    }
    property->set(value);
    return property_set(prop_name, property);
}

int32_t xnative_property_t::native_string_get(const std::string& prop_name, std::string& value) const {
    auto property = string_get(prop_name);
    if (property != nullptr) {
        property->get(value);
        assert(!value.empty());
        return xsuccess;
    }
    return xdata_error_native_property_not_exist;
}

int32_t xnative_property_t::native_map_set(const std::string& prop_name, const std::string& field, const std::string & value) {
    assert(!value.empty());
    auto property = map_get(prop_name);
    if (property == nullptr) {
        property = make_object_ptr<base::xstrmap_t>();
    }
    property->set(field, value);
    return property_set(prop_name, property);
}
int32_t xnative_property_t::native_map_clear(const std::string & prop_name) {
    auto property = map_get(prop_name);
    if (property != nullptr) {
        property->clear();
        return property_set(prop_name, property);
    }
    return xdata_error_native_property_not_exist;
}
int32_t xnative_property_t::native_map_get(const std::string& prop_name, const std::string& field, std::string & value) const {
    auto property = map_get(prop_name);
    if (property != nullptr) {
        if (true == property->get(field, value)) {
            assert(!value.empty());
            return xsuccess;
        }
    }
    return xdata_error_native_property_not_exist;
}
int32_t xnative_property_t::native_map_erase(const std::string& prop_name, const std::string& field) {
    auto property = map_get(prop_name);
    if (property != nullptr) {
        auto ret = property->remove(field);
        if (!ret) {
            xerror("prop_name:%s field:%s remove not find key", prop_name.c_str(), field.c_str());
        }
        return property_set(prop_name, property);
    }
    return xdata_error_native_property_not_exist;
}
int32_t xnative_property_t::native_map_size(const std::string& prop_name, int32_t& size) const {
    auto property = map_get(prop_name);
    if (property != nullptr) {
        size = property->size();
        return xsuccess;
    }
    return xdata_error_native_property_not_exist;
}

int32_t xnative_property_t::native_deque_push_back(const std::string& prop_name, const std::string& value) {
    assert(!value.empty());
    auto property = deque_get(prop_name);
    if (property == nullptr) {
        property = make_object_ptr<base::xstrdeque_t>();
    }
    property->push_back(value);
    return property_set(prop_name, property);
}

int32_t xnative_property_t::native_deque_push_front(const std::string& prop_name, const std::string& value) {
    assert(!value.empty());
    auto property = deque_get(prop_name);
    if (property == nullptr) {
        property = make_object_ptr<base::xstrdeque_t>();
    }
    property->push_front(value);
    return property_set(prop_name, property);
}

int32_t xnative_property_t::native_deque_pop_back(const std::string& prop_name, std::string& value) {
    auto property = deque_get(prop_name);
    if (property != nullptr) {
        if (!property->pop_back(value)) {
            return xdata_error_native_property_value_not_exist;
        }
        assert(!value.empty());
        return property_set(prop_name, property);
    }
    return xdata_error_native_property_not_exist;
}

int32_t xnative_property_t::native_deque_pop_front(const std::string& prop_name, std::string& value) {
    auto property = deque_get(prop_name);
    if (property != nullptr) {
        if (!property->pop_front(value)) {
            return xdata_error_native_property_value_not_exist;
        }
        assert(!value.empty());
        return property_set(prop_name, property);
    }
    return xdata_error_native_property_not_exist;
}

int32_t xnative_property_t::native_deque_erase(const std::string& prop_name, const std::string& value) {
    auto property = deque_get(prop_name);
    if (property != nullptr) {
        if (!property->erase(value)) {
            return xdata_error_native_property_value_not_exist;
        }
        return property_set(prop_name, property);
    }
    return xdata_error_native_property_not_exist;
}

int32_t xnative_property_t::native_deque_clear(const std::string& prop_name) {
    auto property = deque_get(prop_name);
    if (property != nullptr) {
        property->clear();
        return property_set(prop_name, property);
    }
    return xdata_error_native_property_not_exist;
}

bool xnative_property_t::native_deque_exist(const std::string& prop_name, const std::string& value) const {
    auto property = deque_get(prop_name);
    if (property != nullptr) {
        int size = property->size();
        std::string _value;
        for (int i = 0; i < size; ++i) {
            if (property->get(i, _value) && value == _value) {
                return true;
            }
        }
    }
    return false;
}

int32_t xnative_property_t::native_deque_size(const std::string& prop_name, int32_t& size) const {
    auto property = deque_get(prop_name);
    if (property != nullptr) {
        size = property->size();
        return xsuccess;
    }
    return xdata_error_native_property_not_exist;
}

int32_t xnative_property_t::native_deque_get(const std::string& prop_name, std::vector<std::string>& prop_value) const {
    auto property = deque_get(prop_name);
    if (property != nullptr) {
        int size = property->size();
        std::string _value;
        for (int i = 0; i < size; ++i) {
            if (property->get(i, _value)) {
                prop_value.emplace_back(_value);
            }
        }
        return xsuccess;
    }
    return xdata_error_native_property_not_exist;
}

}  // namespace data
}  // namespace top
