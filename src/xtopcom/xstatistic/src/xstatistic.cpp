// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xutl.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xmetrics/xmetrics.h"
#include "xstatistic/xstatistic.h"

NS_BEG2(top, xstatistic)

#if defined(CACHE_SIZE_STATISTIC) || defined(CACHE_SIZE_STATISTIC_MORE_DETAIL)

#if defined(XBUILD_CI) || defined(XBUILD_DEV)
#define STATISTIC_DELAY_TIME (100)
#else
#define STATISTIC_DELAY_TIME (1500)  // delay 1500 ms to calculate object size.
#endif


int32_t relative_time() {
    static int64_t _base_time = base::xtime_utl::gmttime_ms();
    int64_t r_time = base::xtime_utl::gmttime_ms() - _base_time;
    xassert(r_time < 0x7FFFFFFF);
    return r_time;
}

xstatistic_obj_face_t::xstatistic_obj_face_t(enum_statistic_class_type class_type) {
    // xdbg("xstatistic_obj_face_t::xstatistic_obj_face_t this:%p,type:%d", this, type);
    if (class_type == enum_statistic_undetermined) {
        return;
    }
    m_create_time = relative_time();
    xstatistic_t::instance().add_object(this, class_type);
}

xstatistic_obj_face_t::xstatistic_obj_face_t(const xstatistic_obj_face_t & obj) {
    // xdbg("xstatistic_obj_face_t::xstatistic_obj_face_t copy this:%p from:%p", this, &obj, obj.get_class_type());
    if (obj.get_class_type() == enum_statistic_undetermined) {
        return;
    }
    m_create_time = relative_time();
    m_size = 0;
    xstatistic_t::instance().add_object(this, obj.get_class_type());
}

xstatistic_obj_face_t & xstatistic_obj_face_t::operator = (const xstatistic_obj_face_t & obj) {
    // do nothing. for not change value of m_create_time and m_size.
    xassert(obj.get_class_type() != enum_statistic_undetermined);
    return *this;
}

void xstatistic_obj_face_t::modify_class_type(enum_statistic_class_type class_type) {
    m_create_time = relative_time();
    xstatistic_t::instance().add_object(this, class_type);
}

void xstatistic_obj_face_t::statistic_del() {
    // xdbg("xstatistic_obj_face_t::statistic_del copy this:%p,type:%d", this, class_type);
    if (get_class_type() == enum_statistic_undetermined) {
        return;
    }
    xstatistic_t::instance().del_object(this, get_class_type());
}

const int32_t xstatistic_obj_face_t::get_object_size() const {
    if (m_size == 0) {
        m_size = get_object_size_real();
        return m_size;
    }
    return m_size;
}

void xobject_statistic_base_t::add_object(xstatistic_obj_face_t * object, int32_t class_type) {
    std::lock_guard<std::mutex> lck(m_mutex);
    refresh_inner(class_type, object->create_time());
    m_not_calc_object_set.insert(object);
}

void xobject_statistic_base_t::del_object(xstatistic_obj_face_t * object, int32_t class_type) {
    std::lock_guard<std::mutex> lck(m_mutex);
    auto cur_time = relative_time();
    auto ret = m_not_calc_object_set.equal_range(object);
    for (auto it = ret.first; it != ret.second; it++) {
        if ((*it) == object) {
            m_not_calc_object_set.erase(it);
            refresh_inner(class_type, cur_time);
            return;
        }
    }

    update_metrics(class_type, -1, -object->get_object_size());
    refresh_inner(class_type, cur_time);
}

void xobject_statistic_base_t::refresh(int32_t class_type) {
    std::lock_guard<std::mutex> lck(m_mutex);
    refresh_inner(class_type, relative_time());
}

void xobject_statistic_base_t::refresh_inner(int32_t class_type, int64_t now) {
    for (auto iter = m_not_calc_object_set.begin(); iter != m_not_calc_object_set.end();) {
        // xdbg("xobject_statistic_base_t::refresh_inner object:%p, type:%d, create time:%llu, delay_time:%d, now:%llu",
        //      (*iter),
        //      class_type,
        //      (*iter)->create_time(),
        //      STATISTIC_DELAY_TIME,
        //      now);
        if ((*iter)->create_time() + STATISTIC_DELAY_TIME <= now) {
            auto size = (*iter)->get_object_size();
            update_metrics(class_type, 1, size);
            // xdbg("xobject_statistic_base_t::refresh_inner object:%p, size:%d, type:%d", (*iter), size, class_type);
            iter = m_not_calc_object_set.erase(iter);
        } else {
            return;
        }
    }
}

void xobject_statistic_base_t::update_metrics(int32_t type, int32_t change_num, int32_t change_size) {
    int num_metrics_tag = metrics::statistic_tx_v2_num + type - enum_statistic_begin;
    int size_metrics_tag = metrics::statistic_tx_v2_size + type - enum_statistic_begin;

    XMETRICS_GAUGE((metrics::E_SIMPLE_METRICS_TAG)num_metrics_tag, change_num);
    XMETRICS_GAUGE((metrics::E_SIMPLE_METRICS_TAG)size_metrics_tag, change_size);
    XMETRICS_GAUGE(metrics::statistic_total_size, change_size);
}

xstatistic_t & xstatistic_t::instance() {
    static xstatistic_t statistic;
    return statistic;
}

void xstatistic_t::add_object(xstatistic_obj_face_t * object, int32_t class_type) {
    m_object_statistic_arr[class_type].add_object(object, class_type);
    // xdbg("xstatistic_t::add_object object:%p, type:%d", object, class_type);
}

void xstatistic_t::del_object(xstatistic_obj_face_t * object, int32_t class_type) {
    m_object_statistic_arr[class_type].del_object(object, class_type);
    // xdbg("xstatistic_t::del_object object:%p, type:%d", object, class_type);
}

void xstatistic_t::refresh() {
    int32_t class_type = enum_statistic_begin;
    for (int32_t class_type = enum_statistic_begin; class_type < enum_statistic_max; class_type++) {
        m_object_statistic_arr[class_type].refresh(class_type);
    }
}
#endif

NS_END2
