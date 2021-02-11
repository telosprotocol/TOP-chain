#pragma once

#include "xmbus/xevent.h"
#include "xvnetwork/xaddress.h"

NS_BEG2(top, mbus)

class xevent_role_t : public xevent_t {
public:
    enum _minor_type_ {
        none,
        add_role,
        remove_role,
    };

    xevent_role_t(_minor_type_ type, const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> &vnetwork_driver)
    : xevent_t(xevent_major_type_role, type, to_listener, true)
    , m_vnetwork_driver(vnetwork_driver) {
    }

    std::shared_ptr<vnetwork::xvnetwork_driver_face_t> m_vnetwork_driver;
};

DEFINE_SHARED_PTR(xevent_role);

class xevent_role_add_t : public xevent_role_t {
public:
    xevent_role_add_t(const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> &vnetwork_driver):
    xevent_role_t(add_role, vnetwork_driver) {
    }
};

DEFINE_SHARED_PTR(xevent_role_add);

class xevent_role_remove_t : public xevent_role_t {
public:
    xevent_role_remove_t(const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> &vnetwork_driver):
    xevent_role_t(remove_role, vnetwork_driver) {
    }
};

DEFINE_SHARED_PTR(xevent_role_remove);

NS_END2