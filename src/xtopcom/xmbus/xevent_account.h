#pragma once

#include "xmbus/xevent.h"
#include "xdata/xblock.h"

NS_BEG2(top, mbus)

class xevent_account_t : public xevent_t {
public:
    enum _minor_type_ {
        none,
        add_role,
        remove_role,
    };

    xevent_account_t(_minor_type_ type)
    : xevent_t(xevent_major_type_account, type, to_listener, true) {
    }
};

DEFINE_SHARED_PTR(xevent_account);

class xevent_account_add_role_t : public xevent_account_t {
public:

    xevent_account_add_role_t(
            const std::string &_address,
            direction_type dir = to_listener,
            bool _sync = true) :
    xevent_account_t(add_role),
    address(_address) {
    }

    std::string address;
};

class xevent_account_remove_role_t : public xevent_account_t {
public:

    xevent_account_remove_role_t(
            const std::string &_address,
            direction_type dir = to_listener,
            bool _sync = true) :
    xevent_account_t(remove_role),
    address(_address) {
    }

    std::string address;
};

NS_END2
