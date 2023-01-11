// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xdata.h"
#include "xbasic/xmemory.hpp"

#include <atomic>
#include <string>

// #define USE_MULTISET_ONLY

NS_BEG2(top, xstatistic)

enum enum_statistic_class_type {
    enum_statistic_none = 0,
    enum_statistic_begin = 1,
    enum_statistic_tx_v2 = enum_statistic_begin,
    enum_statistic_tx_v3,
    enum_statistic_receipt,
    enum_statistic_vqcert,
    enum_statistic_max,
};

#ifndef CACHE_SIZE_STATISTIC
class xstatistic_obj_face_t {
public:
    xstatistic_obj_face_t(enum_statistic_class_type type) {}
    xstatistic_obj_face_t(const xstatistic_obj_face_t & obj) {}
    virtual ~xstatistic_obj_face_t(){}
    void statistic_del();
private:
    virtual int32_t get_object_size_real() const = 0;
};
#else
// template <int type_value>
class xstatistic_obj_face_t {
// protected:
//     enum { class_type_value = type_value };
public:
    xstatistic_obj_face_t(enum_statistic_class_type type);
    xstatistic_obj_face_t(const xstatistic_obj_face_t & obj);
    virtual ~xstatistic_obj_face_t();
    void statistic_del();

    int64_t create_time() const {return m_create_time;}
    const int32_t get_object_size() const;
    int32_t get_class_type() const {return m_type;}

private:
    virtual int32_t get_object_size_real() const = 0;

private:
    int64_t m_create_time{0};
    enum_statistic_class_type m_type{enum_statistic_none};
    mutable int32_t m_size{0};
};

class xstatistic_obj_comp {
public:
    bool operator()(const xstatistic_obj_face_t * left, const xstatistic_obj_face_t * right) const {
        return left->create_time() < right->create_time();
    }
};

using xnot_calc_object_set_t = std::multiset<xstatistic_obj_face_t *, xstatistic_obj_comp>;
using xnot_calc_object_map_t = std::map<xstatistic_obj_face_t *, xnot_calc_object_set_t::iterator>;

class xobject_statistic_base_t {
public:
    xobject_statistic_base_t(int64_t delay_time) : m_delay_time(delay_time) {}
    void add_object(xstatistic_obj_face_t * object);
    void del_object(xstatistic_obj_face_t * object);
    void refresh();
private:
    void refresh_inner(int64_t now);
    void update_metrics(int32_t type, int32_t change_num, int32_t change_size);
private:
    xnot_calc_object_set_t m_not_calc_object_set;
#ifndef USE_MULTISET_ONLY
    xnot_calc_object_map_t m_not_calc_object_map;
#endif
    int64_t m_delay_time;
    mutable std::mutex m_mutex;
};

class xstatistic_t {
public:
    xstatistic_t();
    ~xstatistic_t();
    void add_object(xstatistic_obj_face_t * object);
    void del_object(xstatistic_obj_face_t * object);
    void refresh();
private:
    std::vector<xobject_statistic_base_t *> m_object_statistic_vec;
};

class xstatistic_hub_t {
public:
    static xstatistic_t* instance();
private:
    static xstatistic_t * _static_statistic;
};
#endif

NS_END2
