// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xcrypto_key.h"
#include "xcommon/xnode_id.h"
#include "xcommon/xrole_type.h"

#include <string>
#include <vector>

NS_BEG3(top, data, registration)

class xtop_rec_registration_node_info {
public:
    common::xregistration_type_t m_registration_type;
    // todo might use more human-readable class to represent `account_mortgage`.(BTW this name might also need alter)
    uint64_t m_account_mortgage;
    xpublic_key_t m_public_key;
    bool is_genesis_node{false};
};
using xrec_registration_node_info_t = xtop_rec_registration_node_info;

class xtop_zec_registration_node_info /*: public xrec_registration_node_info_t  */ {
public:
    xrec_registration_node_info_t m_rec_registration_node_info;
    common::xrole_type_t m_role_type;
    std::string m_nickname;
    // dividenRatio
    // .... so on
};
using xzec_registration_node_info_t = xtop_zec_registration_node_info;

NS_END3