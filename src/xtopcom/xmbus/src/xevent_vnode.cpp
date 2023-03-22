// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xmbus/xevent_vnode.h"

#include "xstatistic/xbasic_size.hpp"

NS_BEG2(top, mbus)

xevent_vnode_t::xevent_vnode_t(bool const destory, std::shared_ptr<xtxpool_service_v2::xrequest_tx_receiver_face> txpool_proxy, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> driver)
  : xbus_event_t{xevent_major_type_vnode}
  , xstatistic::xstatistic_obj_face_t{xstatistic::enum_statistic_event_vnode}
  , destory_{destory}
  , txpool_proxy_{std::move(txpool_proxy)}
  , driver_{std::move(driver)} {
}

xevent_vnode_t::~xevent_vnode_t() {
    statistic_del();
}

void xevent_vnode_t::wait() {
    std::unique_lock<std::mutex> lk(lock_);
    cond_var_.wait(lk, [this] { return this->done_; });
}

void xevent_vnode_t::notify() {
    {
        std::unique_lock<std::mutex> lk(lock_);
        done_ = true;
    }
    cond_var_.notify_all();
}

int32_t xevent_vnode_t::get_class_type() const {
    return xstatistic::enum_statistic_event_vnode;
}

size_t xevent_vnode_t::get_object_size_real() const {
    return sizeof(xevent_vnode_t) + get_size(get_result_data());
}

NS_END2
