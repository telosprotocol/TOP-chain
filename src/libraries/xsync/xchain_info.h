// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xdata_common.h"
#include "xconfig/xpredefined_configurations.h"
#include "xconfig/xconfig_register.h"
#include "xsyncbase/xsync_policy.h"

NS_BEG2(top, sync)

class xchain_info_t {
public:
    xchain_info_t(const std::string &_address, enum_chain_sync_policy _sync_policy):
    address(_address),
    sync_policy(_sync_policy) {
    }

    xchain_info_t() {
    }

    xchain_info_t(const xchain_info_t &o) {
        address = o.address;
        sync_policy = o.sync_policy;
    }

    void merge(const xchain_info_t &other) {
        if (other.sync_policy == enum_chain_sync_policy_full)
            sync_policy = enum_chain_sync_policy_full;
    }

    bool operator ==(const xchain_info_t &other) const {
        return address==other.address && sync_policy==other.sync_policy;
    }

    bool operator !=(const xchain_info_t &other) const {
        return !(*this==other);
    }

    std::string address;
    enum_chain_sync_policy sync_policy{enum_chain_sync_policy_full};
};

NS_END2
