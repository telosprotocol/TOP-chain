// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xmbus/xevent.h"
#include "xstatistic/xstatistic.h"
#include "xtxpool_service_v2/xrequest_tx_receiver_face.h"
#include "xvnetwork/xvnetwork_driver_face.h"

#include <condition_variable>
#include <memory>
#include <mutex>

NS_BEG2(top, mbus)

class xevent_vnode_t final : public xbus_event_t, public xstatistic::xstatistic_obj_face_t {
public:
    xevent_vnode_t(xevent_vnode_t const &) = delete;
    xevent_vnode_t & operator=(xevent_vnode_t const &) = delete;
    xevent_vnode_t(xevent_vnode_t &&) = delete;
    xevent_vnode_t & operator=(xevent_vnode_t &&) = delete;

    xevent_vnode_t(bool destory, std::shared_ptr<xtxpool_service_v2::xrequest_tx_receiver_face> txpool_proxy, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> driver);
    ~xevent_vnode_t() override;

    void wait();
    void notify();

    int32_t get_class_type() const override;

    bool                                               destory_{};
    std::shared_ptr<xtxpool_service_v2::xrequest_tx_receiver_face>   txpool_proxy_{};
    std::shared_ptr<vnetwork::xvnetwork_driver_face_t> driver_{};

    std::mutex              lock_;
    std::condition_variable cond_var_;
    bool                    done_{false};

private:
    size_t get_object_size_real() const override;
};

using xevent_vnode_ptr_t = xobject_ptr_t<xevent_vnode_t>;

NS_END2
