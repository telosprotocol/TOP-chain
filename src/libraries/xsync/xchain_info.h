#pragma once

#include "xdata/xdata_common.h"
#include "xconfig/xpredefined_configurations.h"
#include "xconfig/xconfig_register.h"

NS_BEG2(top, sync)

class xchain_info_t {
public:
    xchain_info_t(const std::string &_address)
    : address(_address) {
    }

    xchain_info_t() {
    }

    xchain_info_t(const xchain_info_t &o) {
        address = o.address;
        is_sys_account = o.is_sys_account;
    }

    void merge(const xchain_info_t &other) {
    }

    bool operator ==(const xchain_info_t &other) const {
        return address==other.address && is_sys_account==other.is_sys_account;
    }

    bool operator !=(const xchain_info_t &other) const {
        return !(*this==other);
    }

    std::string address;
    bool is_sys_account{true};
};

NS_END2
