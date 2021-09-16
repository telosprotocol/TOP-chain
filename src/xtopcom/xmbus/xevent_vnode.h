// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xmbus/xevent.h"
#include "xstore/xstore_face.h"
#include "xtxpool_service_v2/xrequest_tx_receiver_face.h"
#include "xvnetwork/xvnetwork_driver_face.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>

NS_BEG2(top, mbus)

struct xevent_vnode_t : public xbus_event_t {
    xevent_vnode_t(bool _destory, std::shared_ptr<xtxpool_service_v2::xrequest_tx_receiver_face> const & _unit_service, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> const & _driver)
      : xbus_event_t(xevent_major_type_vnode), destory(_destory), unit_service(_unit_service), driver(_driver) {}

    void wait() {
        std::unique_lock<std::mutex> lk(lock);
        cond_var.wait(lk, [this] { return this->done; });
    }

    void notify() {
        {
            std::unique_lock<std::mutex> lk(lock);
            done = true;
        }
        cond_var.notify_all();
    }

    bool                                               destory{};
    std::shared_ptr<xtxpool_service_v2::xrequest_tx_receiver_face>   unit_service{};
    std::shared_ptr<vnetwork::xvnetwork_driver_face_t> driver{};

    std::mutex              lock;
    std::condition_variable cond_var;
    bool                    done{false};
};

using xevent_vnode_ptr_t = xobject_ptr_t<xevent_vnode_t>;

NS_END2
