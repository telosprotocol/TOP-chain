// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xutl.h"
#include "xmetrics/xmetrics.h"
#include "xstatistic/xstatistic.h"

NS_BEG2(top, xstatistic)

#define CALCULATE_SIZE_DELAY_TIME_MS (1000)

#ifdef CACHE_SIZE_STATISTIC
xstatistic_t * xstatistic_hub_t::_static_statistic = nullptr;

xstatistic_t* xstatistic_hub_t::instance() {
    if(_static_statistic)
        return _static_statistic;

    _static_statistic = new xstatistic_t();
    return _static_statistic;
}
#endif

xstatistic_obj_face_t::xstatistic_obj_face_t() {
#ifdef CACHE_SIZE_STATISTIC
    m_create_time = base::xtime_utl::gmttime_ms();
    xstatistic_hub_t::instance()->add_object(this);
#endif
}

xstatistic_obj_face_t::~xstatistic_obj_face_t() {
#ifdef CACHE_SIZE_STATISTIC
    xstatistic_hub_t::instance()->del_object(this);
#endif
}

#ifdef CACHE_SIZE_STATISTIC
const int32_t xstatistic_obj_face_t::get_object_size() const {
    if (m_size == 0) {
        m_size = get_object_size_real();
    }
    return m_size;
}

void xobject_statistic_base_t::add_object(xstatistic_obj_face_t * object) {
    std::lock_guard<std::mutex> lck(m_mutex);
    refresh_inner(object->create_time());
    m_not_calc_object_set.insert(object);
}

void xobject_statistic_base_t::del_object(xstatistic_obj_face_t * object) {
    std::lock_guard<std::mutex> lck(m_mutex);
    auto iter = m_not_calc_object_set.find(object);
    if (iter != m_not_calc_object_set.end()) {
        m_not_calc_object_set.erase(iter);
    } else {
        XMETRICS_GAUGE(get_num_metrics_tag(), -1);
        XMETRICS_GAUGE(get_size_metrics_tag(), -object->get_object_size());
    }
    refresh_inner(base::xtime_utl::gmttime_ms());
}

void xobject_statistic_base_t::refresh() {
    auto now = base::xtime_utl::gmttime_ms();
    std::lock_guard<std::mutex> lck(m_mutex);
    refresh_inner(now);
}

void xobject_statistic_base_t::refresh_inner(int64_t now) {
    for (auto iter = m_not_calc_object_set.begin(); iter != m_not_calc_object_set.end();) {
        if ((*iter)->create_time() + CALCULATE_SIZE_DELAY_TIME_MS <= now) {
            XMETRICS_GAUGE(get_num_metrics_tag(), 1);
            XMETRICS_GAUGE(get_size_metrics_tag(), (*iter)->get_object_size());
            iter = m_not_calc_object_set.erase(iter);
        } else {
            return;
        }
    }
}

xstatistic_t::xstatistic_t() {
    m_object_statistic_vec.push_back(new xobject_statistic_send_tx_t());
}

void xstatistic_t::add_object(xstatistic_obj_face_t * object) {
    uint32_t idx = (uint32_t)object->get_class_type();
    m_object_statistic_vec[idx]->add_object(object);
}

void xstatistic_t::del_object(xstatistic_obj_face_t * object) {
    uint32_t idx = (uint32_t)object->get_class_type();
    m_object_statistic_vec[idx]->del_object(object);
}

void xstatistic_t::refresh() {
    for (auto & object_statistic : m_object_statistic_vec) {
        object_statistic->refresh();
    }
}
#endif

NS_END2
