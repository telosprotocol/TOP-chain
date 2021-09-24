// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xcrypto_key.h"
#include "xbasic/xproper_fraction.hpp"
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

    bool operator==(xtop_rec_registration_node_info const & other) const noexcept{
        return m_registration_type == other.m_registration_type && 
               m_account_mortgage == other.m_account_mortgage && 
               m_public_key == other.m_public_key &&
               is_genesis_node == other.is_genesis_node;
    }

    bool operator!=(xtop_rec_registration_node_info const & other) const noexcept{
        return !(*this==other);
    }
};
using xrec_registration_node_info_t = xtop_rec_registration_node_info;

// xzec_registration_node_info contains too many main chain logical.
// todo charles might be better using a separate file.
using xzec_registration_credit = top::xproper_fraction_t<uint64_t, double>;
class xtop_zec_registration_node_info /*: public xrec_registration_node_info_t  */ {
public:
    xrec_registration_node_info_t m_rec_registration_node_info;
    common::xrole_type_t m_role_type;
    std::string m_nickname;
    uint64_t m_vote_amount;

    xzec_registration_credit m_auditor_credit{0, 1000000};
    xzec_registration_credit m_validator_credit{0, 1000000};

    // uint64_t m_auditor_credit_numerator{0};
    // uint64_t m_auditor_credit_denominator{1000000};
    // uint64_t m_validator_credit_numerator{0};
    // uint64_t m_validator_credit_denominator{1000000};

    // dividenRatio
    // .... so on
    uint64_t account_mortgage() const noexcept;

    
    bool is_auditor_node() const noexcept;

    bool is_valid_auditor_node() const noexcept;

    bool is_validator_node() const noexcept;

    bool is_valid_archive_node() const noexcept;

    bool is_archive_node() const noexcept;

    bool is_full_node() const noexcept;

    bool is_edge_node() const noexcept;

    bool is_genesis_node() const noexcept;

    xpublic_key_t public_key() const noexcept;

    uint64_t auditor_stake() const noexcept;

    uint64_t validator_stake() const noexcept;

    uint64_t archive_stake() const noexcept;

    uint64_t edge_stake() const noexcept;

    uint64_t full_node_stake() const noexcept;

    uint64_t get_required_min_deposit() const noexcept;
};
using xzec_registration_node_info_t = xtop_zec_registration_node_info;

NS_END3