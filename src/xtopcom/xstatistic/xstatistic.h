// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xdata.h"
#include "xbasic/xmemory.hpp"

#include <atomic>
#include <string>

NS_BEG2(top, xstatistic)

enum enum_statistic_class_type {
    enum_statistic_send_tx = 0,
    // enum_statistic_receipts = 1,
    // enum_statistic_qcert = 2,
    enum_statistic_max,
};

class xstatistic_obj_face_t {
public:
    xstatistic_obj_face_t();  // push "this" ptr to statistic set.
    ~xstatistic_obj_face_t(); // remove "this" ptr from statistic set.
#ifdef CACHE_SIZE_STATISTIC
    int64_t create_time() const {return m_create_time;}
    const int32_t get_object_size() const;
    virtual enum_statistic_class_type get_class_type() const = 0;
private:
    virtual const int32_t get_object_size_real() const = 0;
private:
    int64_t m_create_time{0};
    mutable uint64_t m_size{0};
#endif
};

#ifdef CACHE_SIZE_STATISTIC
class xstatistic_obj_comp {
public:
    bool operator()(const xstatistic_obj_face_t * left, const xstatistic_obj_face_t * right) const {
        return left->create_time() < right->create_time();
    }
};

class xobject_statistic_base_t {
public:
    void add_object(xstatistic_obj_face_t * object);
    void del_object(xstatistic_obj_face_t * object);
    void refresh();
private:
    void refresh_inner(int64_t now);
    virtual metrics::E_SIMPLE_METRICS_TAG get_num_metrics_tag() const = 0;
    virtual metrics::E_SIMPLE_METRICS_TAG get_size_metrics_tag() const = 0;
private:
    std::set<xstatistic_obj_face_t *, xstatistic_obj_comp> m_not_calc_object_set;
    mutable std::mutex m_mutex;
};

class xobject_statistic_send_tx_t : public xobject_statistic_base_t {
private:
    virtual metrics::E_SIMPLE_METRICS_TAG get_num_metrics_tag() const override {return metrics::statistic_send_tx_num;}
    virtual metrics::E_SIMPLE_METRICS_TAG get_size_metrics_tag() const override {return metrics::statistic_send_tx_size;}
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
