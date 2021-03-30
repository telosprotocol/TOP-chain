// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <vector>
#include <string>
#include <map>
#include <deque>

#include "xbasic/xobject_ptr.h"
#include "xbase/xmem.h"
#include "xbase/xdata.h"
#include "xdata/xdata_common.h"

#include "xbasic/xserializable_based_on.h"

namespace top { namespace data {

class xnative_property_t : public xserializable_based_on<void> {
 public:
    xnative_property_t() { }

    void copy_modify_objs_only(const xnative_property_t& rhs) {
        m_objs = rhs.m_modify_objs;
    }

    void add_property(const xnative_property_t& rhs) {
        for (auto const & iter : rhs.m_objs) {
            m_objs[iter.first] = iter.second;
        }
    }

    xstring_ptr_t string_get(const std::string& prop_name) const;
    xstrdeque_ptr_t deque_get(const std::string& prop_name) const;
    xstrmap_ptr_t map_get(const std::string& prop_name) const;

    int32_t property_set(const std::string& prop_name, const xdataunit_ptr_t & property);

    int32_t do_write(base::xstream_t & stream) const override;
    int32_t do_read(base::xstream_t & stream) override;

    bool is_property_exist(const std::string& prop_name) const;

    int32_t native_string_set(const std::string& prop_name, const std::string& value);
    int32_t native_string_get(const std::string& prop_name, std::string& value) const;

    int32_t native_map_set(const std::string& prop_name, const std::string& field, const std::string & value);
    int32_t native_map_clear(const std::string & prop_name);
    int32_t native_map_get(const std::string& prop_name, const std::string& field, std::string & value) const;
    int32_t native_map_erase(const std::string& prop_name, const std::string& field);
    int32_t native_map_size(const std::string& prop_name, int32_t& size) const;

    int32_t native_deque_push_back(const std::string& prop_name, const std::string& value);
    int32_t native_deque_push_front(const std::string& prop_name, const std::string& value);
    int32_t native_deque_pop_back(const std::string& prop_name, std::string& value);
    int32_t native_deque_pop_front(const std::string& prop_name, std::string& value);
    int32_t native_deque_erase(const std::string& prop_name, const std::string& value);
    int32_t native_deque_clear(const std::string& prop_name);
    bool native_deque_exist(const std::string& prop_name, const std::string& value) const;
    int32_t native_deque_size(const std::string& prop_name, int32_t& size) const;
    int32_t native_deque_get(const std::string& prop_name, std::vector<std::string>& prop_value) const;

    bool is_empty() const {return m_objs.size() == 0;}
    bool is_dirty() const {return m_modify_objs.size() != 0;}
    void clear_dirty() {m_modify_objs.clear();}

    const std::map<std::string, xdataunit_ptr_t>& get_properties() const {
        return m_objs;
    }

 private:
    xdataunit_ptr_t property_get(const std::string& prop_name, int32_t type) const;

    std::map<std::string, xdataunit_ptr_t> m_objs;
    std::map<std::string, xdataunit_ptr_t> m_modify_objs;
};

}  // namespace data
}  // namespace top
