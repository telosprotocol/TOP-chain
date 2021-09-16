#pragma once

#include "xmbus/xevent.h"
#include "xvnetwork/xaddress.h"

NS_BEG2(top, mbus)

class xevent_role_t : public xbus_event_t {
public:
    enum _minor_type_ {
        none,
        add_role,
        remove_role,
    };

    xevent_role_t(_minor_type_ type, const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> &vnetwork_driver)
    : xbus_event_t(xevent_major_type_role, type, to_listener, true)
    , m_vnetwork_driver(vnetwork_driver) {
    }

    std::shared_ptr<vnetwork::xvnetwork_driver_face_t> m_vnetwork_driver;
};

using xevent_role_ptr_t = xobject_ptr_t<xevent_role_t>;

class xevent_role_add_t : public xevent_role_t {
public:
    xevent_role_add_t(const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> &vnetwork_driver):
    xevent_role_t(add_role, vnetwork_driver) {
    }
};

using xevent_role_add_ptr_t = xobject_ptr_t<xevent_role_add_t>;

class xevent_role_remove_t : public xevent_role_t {
public:
    xevent_role_remove_t(const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> &vnetwork_driver):
    xevent_role_t(remove_role, vnetwork_driver) {
    }
};

using xevent_role_remove_ptr_t = xobject_ptr_t<xevent_role_remove_t>;

NS_END2