// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xregistration/xregistration_node_info.h"

#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"

NS_BEG3(top, data, registration)

uint64_t xtop_zec_registration_node_info::account_mortgage() const noexcept {
    return m_rec_registration_node_info.m_account_mortgage;
}

bool xtop_zec_registration_node_info::is_auditor_node() const noexcept {
    return common::has<common::xrole_type_t::advance>(m_role_type);
}

bool xtop_zec_registration_node_info::is_valid_auditor_node() const noexcept {
    return common::has<common::xrole_type_t::advance>(m_role_type) && m_vote_amount * TOP_UNIT >= account_mortgage();
}

bool xtop_zec_registration_node_info::is_validator_node() const noexcept {
    return common::has<common::xrole_type_t::validator>(m_role_type) || common::has<common::xrole_type_t::advance>(m_role_type);
}

bool xtop_zec_registration_node_info::is_valid_archive_node() const noexcept {
    return common::has<common::xrole_type_t::archive>(m_role_type) || (common::has<common::xrole_type_t::advance>(m_role_type) && m_vote_amount * TOP_UNIT >= account_mortgage());
}

bool xtop_zec_registration_node_info::is_archive_node() const noexcept {
    return common::has<common::xrole_type_t::archive>(m_role_type) || common::has<common::xrole_type_t::advance>(m_role_type);
}

bool xtop_zec_registration_node_info::is_full_node() const noexcept {
    return common::has<common::xrole_type_t::full_node>(m_role_type);
}

bool xtop_zec_registration_node_info::is_edge_node() const noexcept {
    return common::has<common::xrole_type_t::edge>(m_role_type);
}

bool xtop_zec_registration_node_info::is_genesis_node() const noexcept {
    return m_rec_registration_node_info.is_genesis_node;
}

xpublic_key_t xtop_zec_registration_node_info::public_key() const noexcept {
    return m_rec_registration_node_info.m_public_key;
}

uint64_t xtop_zec_registration_node_info::auditor_stake() const noexcept {
    uint64_t stake = 0;
    if (is_auditor_node()) {
        stake = (account_mortgage() / TOP_UNIT + m_vote_amount / 2) * m_auditor_credit.value();
    }
    return stake;
}

uint64_t xtop_zec_registration_node_info::validator_stake() const noexcept {
    uint64_t stake = 0;
    if (is_validator_node()) {
        auto max_validator_stake = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_validator_stake);
        stake = (uint64_t)sqrt((account_mortgage() / TOP_UNIT + m_vote_amount / 2) * m_validator_credit.value());
        stake = stake < max_validator_stake ? stake : max_validator_stake;
    }
    return stake;
}

uint64_t xtop_zec_registration_node_info::archive_stake() const noexcept {
    return 0;
}

uint64_t xtop_zec_registration_node_info::edge_stake() const noexcept {
    return 0;
}

uint64_t xtop_zec_registration_node_info::full_node_stake() const noexcept {
    return 0;
}

uint64_t xtop_zec_registration_node_info::get_required_min_deposit() const noexcept {
    uint64_t min_deposit = 0;
    if (is_edge_node()) {
        uint64_t deposit = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_edge_deposit);
        if (deposit > min_deposit)
            min_deposit = deposit;
    }

    if (is_validator_node()) {
        uint64_t deposit = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_validator_deposit);
        if (deposit > min_deposit)
            min_deposit = deposit;
    }

    if (is_auditor_node()) {
        uint64_t deposit = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_auditor_deposit);
        if (deposit > min_deposit)
            min_deposit = deposit;
    }

    if (is_archive_node()) {
        uint64_t deposit = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_archive_deposit);
        if (deposit > min_deposit)
            min_deposit = deposit;
    }
    return min_deposit;
}

NS_END3