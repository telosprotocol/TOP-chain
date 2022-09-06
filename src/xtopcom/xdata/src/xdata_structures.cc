// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xdata/xsystem_contract/xdata_structures.h"

#include "xbasic/xutility.h"
#include "xbasic/xversion.h"
#include "xdata/xnative_contract_address.h"

NS_BEG3(top, data, system_contract)

bool check_registered_nodes_active(std::map<std::string, std::string> const & nodes) {
    uint32_t auditor_num = 0;
    uint32_t validator_num = 0;
    uint32_t archive_num = 0;
    uint32_t fullnode_num = 0;
    uint32_t edge_num = 0;
    uint64_t total_votes = 0;

    for (auto const & entity : nodes) {
        auto const & account = entity.first;
        auto const & reg_node_str = entity.second;
        if (reg_node_str.empty()) {
            continue;
        }
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)reg_node_str.data(), reg_node_str.size());
        xreg_node_info reg_node_info;
        reg_node_info.serialize_from(stream);

        xdbg("[check_registered_nodes_active] account: %s, if_adv: %d, if_validator: %d, if_archive: %d, if_edge: %d, votes: %llu",
             reg_node_info.m_account.c_str(),
             reg_node_info.can_be_auditor(),
             reg_node_info.can_be_validator(),
             reg_node_info.can_be_archive(),
             reg_node_info.can_be_edge(),
             reg_node_info.m_vote_amount);

        if (reg_node_info.is_invalid_node())
            continue;

        if (reg_node_info.can_be_auditor()) {
            auditor_num++;
        }
        if (reg_node_info.can_be_validator()) {
            validator_num++;
        }
        if (reg_node_info.can_be_archive()) {
            archive_num++;
        }
        if (reg_node_info.can_be_fullnode()) {
            fullnode_num++;
        }
        if (reg_node_info.can_be_edge()) {
            edge_num++;
        }
        total_votes += reg_node_info.m_vote_amount;
    }

    auto CFG_AUDITORS_MIN = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_auditors);
    auto CFG_VALIDATORS_MIN = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_validators);
    auto CFG_EDGES_MIN = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_edges);
    auto CFG_ARCHIVES_MIN = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_archives);
    auto CFG_TOTAL_VOTES_MIN = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_votes);

    xinfo(
        "[check_registered_nodes_active] auditor_num: %u, AUDITORS_MIN: %u, validator_num: %u, VALIDATORS_MIN: %u, archive_num: %u, ARCHIVES_MIN: %u, edge_num: %u, EDGES_MIN: %u, "
        "fullnode_num: %u, total_votes: %llu, TOTAL_VOTES_MIN: %llu",
        auditor_num,
        CFG_AUDITORS_MIN,
        validator_num,
        CFG_VALIDATORS_MIN,
        archive_num,
        CFG_ARCHIVES_MIN,
        edge_num,
        CFG_EDGES_MIN,
        fullnode_num,
        total_votes,
        CFG_TOTAL_VOTES_MIN);

    return auditor_num >= CFG_AUDITORS_MIN && validator_num >= CFG_VALIDATORS_MIN && edge_num >= CFG_EDGES_MIN && archive_num >= CFG_ARCHIVES_MIN &&
           total_votes >= CFG_TOTAL_VOTES_MIN;
}

int32_t xreward_node_record::do_write(base::xstream_t & stream) const {
    const int32_t begin_pos = stream.size();
    stream << m_accumulated;
    stream << m_unclaimed;
    stream << m_last_claim_time;
    stream << m_issue_time;
    const int32_t end_pos = stream.size();
    return (end_pos - begin_pos);
}

int32_t xreward_node_record::do_read(base::xstream_t & stream) {
    const int32_t begin_pos = stream.size();
    stream >> m_accumulated;
    stream >> m_unclaimed;
    stream >> m_last_claim_time;
    stream >> m_issue_time;
    const int32_t end_pos = stream.size();
    return (begin_pos - end_pos);
}

int32_t node_record_t::do_write(base::xstream_t & stream) const {
    const int32_t begin_pos = stream.size();
    stream << account;
    stream << accumulated;
    stream << unclaimed;
    stream << last_claim_time;
    stream << issue_time;
    const int32_t end_pos = stream.size();
    return (end_pos - begin_pos);
}

int32_t node_record_t::do_read(base::xstream_t & stream) {
    const int32_t begin_pos = stream.size();
    stream >> account;
    stream >> accumulated;
    stream >> unclaimed;
    stream >> last_claim_time;
    stream >> issue_time;
    const int32_t end_pos = stream.size();
    return (begin_pos - end_pos);
}

int32_t xreward_record::do_write(base::xstream_t & stream) const {
    const int32_t begin_pos = stream.size();
    stream << accumulated;
    stream << unclaimed;
    stream << last_claim_time;
    stream << issue_time;
    VECTOR_OBJECT_SERIALIZE2(stream, node_rewards);
    const int32_t end_pos = stream.size();
    return (end_pos - begin_pos);
}

int32_t xreward_record::do_read(base::xstream_t & stream) {
    const int32_t begin_pos = stream.size();
    stream >> accumulated;
    stream >> unclaimed;
    stream >> last_claim_time;
    stream >> issue_time;
    VECTOR_OBJECT_DESERIALZE2(stream, node_rewards);
    const int32_t end_pos = stream.size();
    return (begin_pos - end_pos);
}

int32_t xaccumulated_reward_record::do_write(base::xstream_t & stream) const {
    const int32_t begin_pos = stream.size();
    stream << last_issuance_time;
    stream << issued_until_last_year_end;
    const int32_t end_pos = stream.size();
    return (end_pos - begin_pos);
}

int32_t xaccumulated_reward_record::do_read(base::xstream_t & stream) {
    const int32_t begin_pos = stream.size();
    stream >> last_issuance_time;
    stream >> issued_until_last_year_end;
    const int32_t end_pos = stream.size();
    return (begin_pos - end_pos);
}

int32_t xrefund_info::do_write(base::xstream_t & stream) const {
    const int32_t begin_pos = stream.size();
    stream << refund_amount;
    stream << create_time;
    const int32_t end_pos = stream.size();
    return (end_pos - begin_pos);
}

int32_t xrefund_info::do_read(base::xstream_t & stream) {
    const int32_t begin_pos = stream.size();
    stream >> refund_amount;
    stream >> create_time;
    const int32_t end_pos = stream.size();
    return (begin_pos - end_pos);
}

std::int32_t xnode_vote_percent_t::do_write(base::xstream_t & stream) const {
    KEEP_SIZE();
    stream << block_count;
    stream << subset_count;
    return CALC_LEN();
}

std::int32_t xnode_vote_percent_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    stream >> block_count;
    stream >> subset_count;
    return CALC_LEN();
}

int32_t xunqualified_node_info_v1_t::do_write(base::xstream_t & stream) const {
    KEEP_SIZE();
    MAP_OBJECT_SERIALIZE2(stream, auditor_info);
    MAP_OBJECT_SERIALIZE2(stream, validator_info);
    return CALC_LEN();
}

int32_t xunqualified_node_info_v1_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    MAP_OBJECT_DESERIALZE2(stream, auditor_info);
    MAP_OBJECT_DESERIALZE2(stream, validator_info);
    return CALC_LEN();
}

int32_t xunqualified_filter_info_t::do_write(base::xstream_t & stream) const {
    KEEP_SIZE();
    stream << node_id;
    stream << node_type;
    stream << vote_percent;
    return CALC_LEN();
}

int32_t xunqualified_filter_info_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    stream >> node_id;
    stream >> node_type;
    stream >> vote_percent;
    return CALC_LEN();
}

int32_t xaction_node_info_t::do_write(base::xstream_t & stream) const {
    KEEP_SIZE();
    stream << node_id;
    stream << node_type;
    stream << action_type;
    return CALC_LEN();
}

int32_t xaction_node_info_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    stream >> node_id;
    stream >> node_type;
    stream >> action_type;
    return CALC_LEN();
}

template <>
uint64_t minimal_deposit_of<common::xminer_type_t::edge>() {
    return XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_edge_deposit);
}

template <>
uint64_t minimal_deposit_of<common::xminer_type_t::archive>() {
    return XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_archive_deposit);
}

template <>
uint64_t minimal_deposit_of<common::xminer_type_t::exchange>() {
    return 0;
}

template <>
uint64_t minimal_deposit_of<common::xminer_type_t::advance>() {
    return XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_auditor_deposit);
}

template <>
uint64_t minimal_deposit_of<common::xminer_type_t::validator>() {
    return XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_validator_deposit);
}

template <>
bool could_be<common::xnode_type_t::rec>(common::xminer_type_t const miner_type) {
    return common::has<common::xminer_type_t::advance>(miner_type);
}

template <>
bool could_be<common::xnode_type_t::zec>(common::xminer_type_t const miner_type) {
    return common::has<common::xminer_type_t::advance>(miner_type);
}

template <>
bool could_be<common::xnode_type_t::consensus_auditor>(common::xminer_type_t const miner_type) {
    return common::has<common::xminer_type_t::advance>(miner_type);
}

template <>
bool could_be<common::xnode_type_t::consensus_validator>(common::xminer_type_t const miner_type) {
    return common::has<common::xminer_type_t::validator>(miner_type) || common::has<common::xminer_type_t::advance>(miner_type);
}

template <>
bool could_be<common::xnode_type_t::storage_archive>(common::xminer_type_t const miner_type) {
    return common::has<common::xminer_type_t::archive>(miner_type);
}

template <>
bool could_be<common::xnode_type_t::storage_exchange>(common::xminer_type_t const miner_type) {
    return common::has<common::xminer_type_t::exchange>(miner_type);
}

template <>
bool could_be<common::xnode_type_t::edge>(common::xminer_type_t const miner_type) {
    return common::has<common::xminer_type_t::edge>(miner_type);
}

template <>
bool could_be<common::xnode_type_t::fullnode>(common::xminer_type_t const miner_type) {
    return common::has<common::xminer_type_t::advance>(miner_type);
}

template <>
bool could_be<common::xnode_type_t::evm_auditor>(common::xminer_type_t const miner_type) {
    return common::has<common::xminer_type_t::advance>(miner_type);
}

template <>
bool could_be<common::xnode_type_t::evm_validator>(common::xminer_type_t const miner_type) {
#if defined (XBUILD_DEV)
    return common::has<common::xminer_type_t::validator>(miner_type);
#else
    return common::has<common::xminer_type_t::advance>(miner_type);
#endif
}

template <>
bool could_be<common::xnode_type_t::relay>(common::xminer_type_t const miner_type) {
    return common::has<common::xminer_type_t::advance>(miner_type);
}

bool xreg_node_info::could_be_rec() const noexcept {
    return could_be<common::xnode_type_t::rec>(m_registered_miner_type);
}

bool xreg_node_info::could_be_zec() const noexcept {
    return could_be<common::xnode_type_t::zec>(m_registered_miner_type);
}

bool xreg_node_info::could_be_auditor() const noexcept {
    return could_be<common::xnode_type_t::consensus_auditor>(m_registered_miner_type);
}

bool xreg_node_info::could_be_validator() const noexcept {
    return could_be<common::xnode_type_t::consensus_validator>(m_registered_miner_type);
}

bool xreg_node_info::could_be_archive() const noexcept {
    return could_be<common::xnode_type_t::storage_archive>(m_registered_miner_type);
}

bool xreg_node_info::legacy_could_be_archive() const noexcept {
    return could_be_auditor() || could_be_archive();
}

bool xreg_node_info::could_be_edge() const noexcept {
    return could_be<common::xnode_type_t::edge>(m_registered_miner_type);
}

bool xreg_node_info::could_be_exchange() const noexcept {
    return could_be<common::xnode_type_t::storage_exchange>(m_registered_miner_type);
}

bool xreg_node_info::could_be_fullnode() const noexcept {
    return could_be<common::xnode_type_t::fullnode>(m_registered_miner_type);
}

bool xreg_node_info::could_be_evm_auditor() const noexcept {
    return could_be<common::xnode_type_t::evm_auditor>(m_registered_miner_type);
}

bool xreg_node_info::could_be_evm_validator() const noexcept {
    return could_be<common::xnode_type_t::evm_validator>(m_registered_miner_type);
}

bool xreg_node_info::could_be_relay() const noexcept {
    return could_be<common::xnode_type_t::relay>(m_registered_miner_type);
}

bool xreg_node_info::can_be_rec() const noexcept {
    return could_be_rec();
}

bool xreg_node_info::can_be_zec() const noexcept {
    return could_be_zec();
}

bool xreg_node_info::can_be_edge() const noexcept {
    return could_be_edge();
}

bool xreg_node_info::can_be_archive() const noexcept {
    return could_be_archive();
}

bool xreg_node_info::legacy_can_be_archive() const noexcept {
    return can_be_auditor() || can_be_archive();
}

bool xreg_node_info::can_be_auditor() const noexcept {
    return could_be_auditor() && has_enough_tickets();
}

bool xreg_node_info::can_be_validator() const noexcept {
    return could_be_validator();
}

bool xreg_node_info::can_be_exchange() const noexcept {
    return could_be_exchange();
}

bool xreg_node_info::can_be_fullnode() const noexcept {
    return could_be_auditor();
}

bool xreg_node_info::can_be_evm_auditor() const noexcept {
    return could_be_evm_auditor() && has_enough_tickets();
}

bool xreg_node_info::can_be_evm_validator() const noexcept {
    return could_be_evm_validator();
}

bool xreg_node_info::can_be_relay() const noexcept {
    return could_be_relay() && has_enough_tickets();
}

bool xreg_node_info::has_enough_tickets() const noexcept {
    if (XGET_ONCHAIN_GOVERNANCE_PARAMETER(toggle_register_whitelist) == 1)
        return true;

#if defined(XENABLE_MOCK_ZEC_STAKE)
    return true;
#else
    return m_vote_amount * TOP_UNIT >= deposit();
#endif
}

uint64_t xreg_node_info::deposit() const noexcept {
    return m_account_mortgage;
}

uint64_t xreg_node_info::get_required_min_deposit() const noexcept {
    uint64_t min_deposit = 0;
    if (miner_type_has<common::xminer_type_t::edge>()) {
        min_deposit = std::max(min_deposit, minimal_deposit_of<common::xminer_type_t::edge>());
    }

    if (miner_type_has<common::xminer_type_t::validator>()) {
        min_deposit = std::max(min_deposit, minimal_deposit_of<common::xminer_type_t::validator>());
    }

    if (miner_type_has<common::xminer_type_t::advance>()) {
        min_deposit = std::max(min_deposit, minimal_deposit_of<common::xminer_type_t::advance>());
    }

    if (miner_type_has<common::xminer_type_t::archive>()) {
        min_deposit = std::max(min_deposit, minimal_deposit_of<common::xminer_type_t::archive>());
    }

    if (miner_type_has<common::xminer_type_t::exchange>()) {
        min_deposit = std::max(min_deposit, minimal_deposit_of<common::xminer_type_t::exchange>());
    }

    return min_deposit;
}

uint64_t xreg_node_info::rec_stake() const noexcept {
    uint64_t stake = 0;
    if (could_be_rec()) {
        stake = m_account_mortgage / TOP_UNIT + m_vote_amount / 2;
    }
    return stake;
}

uint64_t xreg_node_info::zec_stake() const noexcept {
    uint64_t stake = 0;
    if (could_be_zec()) {
        stake = m_account_mortgage / TOP_UNIT + m_vote_amount / 2;
    }
    return stake;
}

uint64_t xreg_node_info::auditor_stake() const noexcept {
    return get_auditor_stake();
}

uint64_t xreg_node_info::validator_stake() const noexcept {
    return get_validator_stake();
}

uint64_t xreg_node_info::edge_stake() const noexcept {
    return 0;
}

uint64_t xreg_node_info::archive_stake() const noexcept {
    return 0;
}

uint64_t xreg_node_info::exchange_stake() const noexcept {
    return 0;
}

uint64_t xreg_node_info::fullnode_stake() const noexcept {
    return 0;
}

uint64_t xreg_node_info::evm_auditor_stake() const noexcept {
    return auditor_stake();
}

uint64_t xreg_node_info::evm_validator_stake() const noexcept {
    return validator_stake();
}

uint64_t xreg_node_info::relay_stake() const noexcept {
    return auditor_stake();
}

common::xminer_type_t xreg_node_info::miner_type() const noexcept {
    return m_registered_miner_type;
}

void xreg_node_info::miner_type(common::xminer_type_t new_miner_type) noexcept {
    if (m_registered_miner_type != new_miner_type) {
        m_registered_miner_type = new_miner_type;
    }
}

int32_t xreg_node_info::do_write(base::xstream_t & stream) const {
    const int32_t begin_pos = stream.size();
    stream << m_account.value();
    stream << m_account_mortgage;
    ENUM_SERIALIZE(stream, m_registered_miner_type);
    stream << m_vote_amount;
    stream << m_auditor_credit_numerator;
    stream << m_auditor_credit_denominator;
    stream << m_validator_credit_numerator;
    stream << m_validator_credit_denominator;
    stream << m_support_ratio_numerator;
    stream << m_support_ratio_denominator;
    // stream << m_stake;
    // m_stake_info.serialize_to(stream);
    stream << m_last_update_time;
    stream << m_genesis;
    stream << m_network_ids;
    stream << nickname;
    stream << consensus_public_key;
    const int32_t end_pos = stream.size();
    return (end_pos - begin_pos);
}

int32_t xreg_node_info::do_read(base::xstream_t & stream) {
    const int32_t begin_pos = stream.size();
    std::string account;
    stream >> account;
    m_account = common::xaccount_address_t{account};
    stream >> m_account_mortgage;
    ENUM_DESERIALIZE(stream, m_registered_miner_type);
    stream >> m_vote_amount;
    stream >> m_auditor_credit_numerator;
    stream >> m_auditor_credit_denominator;
    stream >> m_validator_credit_numerator;
    stream >> m_validator_credit_denominator;
    stream >> m_support_ratio_numerator;
    stream >> m_support_ratio_denominator;
    // stream >> m_stake;
    // m_stake_info.serialize_from(stream);
    stream >> m_last_update_time;
    stream >> m_genesis;
    stream >> m_network_ids;
    stream >> nickname;
    stream >> consensus_public_key;
    const int32_t end_pos = stream.size();
    return (begin_pos - end_pos);
}

bool xreg_node_info::genesis() const noexcept {
    return m_genesis;
}

void xreg_node_info::genesis(bool v) noexcept {
    if (m_genesis != v) {
        m_genesis = v;
    }
}

xgroup_workload_t & xgroup_workload_t::operator+=(xgroup_workload_t const & rhs) {
    if (group_address_str != rhs.group_address_str) {
        return *this;
    }
    for (auto const & count : rhs.m_leader_count) {
        m_leader_count[count.first] += count.second;
        group_total_workload += count.second;
    }
    return *this;
}

std::int32_t xgroup_workload_t::do_write(base::xstream_t & stream) const {
    auto const begin = stream.size();
    stream << group_address_str;
    stream << group_total_workload;
    MAP_SERIALIZE_SIMPLE(stream, m_leader_count);
    auto const end = stream.size();
    return end - begin;
}

std::int32_t xgroup_workload_t::do_read(base::xstream_t & stream) {
    auto const begin = stream.size();
    stream >> group_address_str;
    stream >> group_total_workload;
    MAP_DESERIALIZE_SIMPLE(stream, m_leader_count);
    auto const end = stream.size();
    return begin - end;
}

int32_t xactivation_record::do_write(base::xstream_t & stream) const {
    const int32_t begin_pos = stream.size();
    stream << activated;
    stream << activation_time;
    const int32_t end_pos = stream.size();
    return (end_pos - begin_pos);
}

int32_t xactivation_record::do_read(base::xstream_t & stream) {
    const int32_t begin_pos = stream.size();
    stream >> activated;
    stream >> activation_time;
    const int32_t end_pos = stream.size();
    return (begin_pos - end_pos);
}

int32_t xslash_info::do_write(base::xstream_t & stream) const {
    const int32_t begin_pos = stream.size();
    stream << m_punish_time;
    stream << m_staking_lock_time;
    stream << m_punish_staking;
    const int32_t end_pos = stream.size();
    return (end_pos - begin_pos);
}

int32_t xslash_info::do_read(base::xstream_t & stream) {
    const int32_t begin_pos = stream.size();
    stream >> m_punish_time;
    stream >> m_staking_lock_time;
    stream >> m_punish_staking;
    const int32_t end_pos = stream.size();
    return (begin_pos - end_pos);
}

uint64_t xreg_node_info::raw_credit_score_data(common::xnode_type_t const node_type) const noexcept {
    switch (node_type) {
    case common::xnode_type_t::consensus_auditor:
        return m_auditor_credit_numerator;

    case common::xnode_type_t::consensus_validator:
        return m_validator_credit_numerator;

    default:
        return 0;
    }
}

void xreg_node_info::slash_credit_score(common::xnode_type_t node_type) {
    uint64_t slash_creditscore_numerator{0};
    if (common::has<common::xnode_type_t::consensus_validator>(node_type)) {
        slash_creditscore_numerator = XGET_ONCHAIN_GOVERNANCE_PARAMETER(validator_slash_creditscore);
        auto config_min = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_creditscore);

        if (slash_creditscore_numerator > m_validator_credit_numerator) {
            xwarn("[slash_credit_score] slash validator credit to min!");
            m_validator_credit_numerator = config_min;
            return;
        }

        m_validator_credit_numerator -= slash_creditscore_numerator;
        if (m_validator_credit_numerator < config_min) {
            m_validator_credit_numerator = config_min;
        }


    } else if (common::has<common::xnode_type_t::consensus_auditor>(node_type)) {
        slash_creditscore_numerator = XGET_ONCHAIN_GOVERNANCE_PARAMETER(auditor_slash_creditscore);
        auto config_min = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_creditscore);

        if (slash_creditscore_numerator > m_auditor_credit_numerator) {
            xwarn("[slash_credit_score] slash auditor credit to min!");
            m_auditor_credit_numerator = config_min;
            return;
        }

        m_auditor_credit_numerator -= slash_creditscore_numerator;
        if (m_auditor_credit_numerator < config_min) {
            m_auditor_credit_numerator = config_min;
        }
    }
}

void xreg_node_info::award_credit_score(common::xnode_type_t node_type) {
    uint64_t award_creditscore_numerator{0};
    if (common::has<common::xnode_type_t::consensus_validator>(node_type)) {
        award_creditscore_numerator = XGET_ONCHAIN_GOVERNANCE_PARAMETER(validator_award_creditscore);
        m_validator_credit_numerator += award_creditscore_numerator;
        if (m_validator_credit_numerator > m_validator_credit_denominator) {
            xwarn("[award_credit_score] award validator credit up to max!");
            m_validator_credit_numerator = m_validator_credit_denominator;
            return;
        }

    } else if (common::has<common::xnode_type_t::consensus_auditor>(node_type)) {
        award_creditscore_numerator = XGET_ONCHAIN_GOVERNANCE_PARAMETER(auditor_award_creditscore);
        m_auditor_credit_numerator += award_creditscore_numerator;
        if (m_auditor_credit_numerator > m_auditor_credit_denominator) {
            xwarn("[award_credit_score] award auditor credit up to max!");
            m_auditor_credit_numerator = m_auditor_credit_denominator;
            return;
        }
    }
}

int32_t reward_detail_v1::do_write(base::xstream_t & stream) const {
    const int32_t begin_pos = stream.size();
    stream << m_edge_reward;
    stream << m_archive_reward;
    stream << m_validator_reward;
    stream << m_auditor_reward;
    stream << m_vote_reward;
    stream << m_self_reward;
    const int32_t end_pos = stream.size();
    return (end_pos - begin_pos);
}

int32_t reward_detail_v1::do_read(base::xstream_t & stream) {
    const int32_t begin_pos = stream.size();
    stream >> m_edge_reward;
    stream >> m_archive_reward;
    stream >> m_validator_reward;
    stream >> m_auditor_reward;
    stream >> m_vote_reward;
    stream >> m_self_reward;
    const int32_t end_pos = stream.size();
    return (begin_pos - end_pos);
}

int32_t reward_detail_v2::do_write(base::xstream_t & stream) const {
    const int32_t begin_pos = stream.size();
    stream << m_edge_reward;
    stream << m_archive_reward;
    stream << m_validator_reward;
    stream << m_auditor_reward;
    stream << m_vote_reward;
    stream << m_self_reward;
    stream << m_evm_validator_reward;
    stream << m_evm_auditor_reward;
    const int32_t end_pos = stream.size();
    return (end_pos - begin_pos);
}

int32_t reward_detail_v2::do_read(base::xstream_t & stream) {
    const int32_t begin_pos = stream.size();
    stream >> m_edge_reward;
    stream >> m_archive_reward;
    stream >> m_validator_reward;
    stream >> m_auditor_reward;
    stream >> m_vote_reward;
    stream >> m_self_reward;
    if (stream.size() > 0) {
        stream >> m_evm_validator_reward;
    }
    if (stream.size() > 0) {
        stream >> m_evm_auditor_reward;
    }
    const int32_t end_pos = stream.size();
    return (begin_pos - end_pos);
}

reward_detail_v2::operator reward_detail_v1() const {
    reward_detail_v1 v1;
    v1.m_edge_reward = m_edge_reward;
    v1.m_archive_reward = m_archive_reward;
    v1.m_validator_reward = m_validator_reward;
    v1.m_auditor_reward = m_auditor_reward;
    v1.m_vote_reward = m_vote_reward;
    v1.m_self_reward = m_self_reward;
    return v1;
}

std::string xissue_detail_v1::to_string() const {
    base::xstream_t stream(base::xcontext_t::instance());
    serialize_to(stream);
    return std::string((const char *)stream.data(), stream.size());
}

int32_t xissue_detail_v1::from_string(std::string const & s) {
    if (s.empty()) {
        xwarn("xissue_detail::from_string invalid input");
        return -1;
    }

    base::xstream_t _stream(base::xcontext_t::instance(), (uint8_t *)s.data(), (int32_t)s.size());
    int32_t ret = serialize_from(_stream);
    if (ret <= 0) {
        xerror("serialize_from_string fail. ret=%d,bin_data_size=%d", ret, s.size());
    }
    return ret;
}

int32_t xissue_detail_v1::do_write(base::xstream_t & stream) const {
    const int32_t begin_pos = stream.size();
    stream << onchain_timer_round;
    stream << m_zec_vote_contract_height;
    stream << m_zec_workload_contract_height;
    stream << m_zec_reward_contract_height;
    stream << m_edge_reward_ratio;
    stream << m_archive_reward_ratio;
    stream << m_validator_reward_ratio;
    stream << m_auditor_reward_ratio;
    stream << m_vote_reward_ratio;
    stream << m_governance_reward_ratio;
    stream << m_auditor_group_count;
    stream << m_validator_group_count;
    MAP_OBJECT_SERIALIZE2(stream, m_node_rewards);
    const int32_t end_pos = stream.size();
    return (end_pos - begin_pos);
}

int32_t xissue_detail_v1::do_read(base::xstream_t & stream) {
    const int32_t begin_pos = stream.size();
    stream >> onchain_timer_round;
    stream >> m_zec_vote_contract_height;
    stream >> m_zec_workload_contract_height;
    stream >> m_zec_reward_contract_height;
    stream >> m_edge_reward_ratio;
    stream >> m_archive_reward_ratio;
    stream >> m_validator_reward_ratio;
    stream >> m_auditor_reward_ratio;
    stream >> m_vote_reward_ratio;
    stream >> m_governance_reward_ratio;
    stream >> m_auditor_group_count;
    stream >> m_validator_group_count;
    MAP_OBJECT_DESERIALZE2(stream, m_node_rewards);
    const int32_t end_pos = stream.size();
    return (begin_pos - end_pos);
}

std::string xissue_detail_v2::to_string() const {
    base::xstream_t stream(base::xcontext_t::instance());
    serialize_to(stream);
    return std::string((const char *)stream.data(), stream.size());
}

int32_t xissue_detail_v2::from_string(std::string const & s) {
    if (s.empty()) {
        xwarn("xissue_detail::from_string invalid input");
        return -1;
    }

    base::xstream_t _stream(base::xcontext_t::instance(), (uint8_t *)s.data(), (int32_t)s.size());
    int32_t ret = serialize_from(_stream);
    if (ret <= 0) {
        xerror("serialize_from_string fail. ret=%d,bin_data_size=%d", ret, s.size());
    }
    return ret;
}

int32_t xissue_detail_v2::do_write(base::xstream_t & stream) const {
    const int32_t begin_pos = stream.size();
    stream << onchain_timer_round;
    stream << m_zec_vote_contract_height;
    stream << m_zec_workload_contract_height;
    stream << m_zec_reward_contract_height;
    stream << m_edge_reward_ratio;
    stream << m_archive_reward_ratio;
    stream << m_validator_reward_ratio;
    stream << m_auditor_reward_ratio;
    stream << m_vote_reward_ratio;
    stream << m_governance_reward_ratio;
    stream << m_auditor_group_count;
    stream << m_validator_group_count;
    MAP_OBJECT_SERIALIZE2(stream, m_node_rewards);
    stream << m_evm_auditor_reward_ratio;
    stream << m_evm_validator_reward_ratio;
    stream << m_evm_auditor_group_count;
    stream << m_evm_validator_group_count;
    const int32_t end_pos = stream.size();
    return (end_pos - begin_pos);
}

int32_t xissue_detail_v2::do_read(base::xstream_t & stream) {
    const int32_t begin_pos = stream.size();
    stream >> onchain_timer_round;
    stream >> m_zec_vote_contract_height;
    stream >> m_zec_workload_contract_height;
    stream >> m_zec_reward_contract_height;
    stream >> m_edge_reward_ratio;
    stream >> m_archive_reward_ratio;
    stream >> m_validator_reward_ratio;
    stream >> m_auditor_reward_ratio;
    stream >> m_vote_reward_ratio;
    stream >> m_governance_reward_ratio;
    stream >> m_auditor_group_count;
    stream >> m_validator_group_count;
    MAP_OBJECT_DESERIALZE2(stream, m_node_rewards);
    if (stream.size() > 0) {
        stream >> m_evm_auditor_reward_ratio;
    }
    if (stream.size() > 0) {
        stream >> m_evm_validator_reward_ratio;
    }
    if (stream.size() > 0) {
        stream >> m_evm_auditor_group_count;
    }
    if (stream.size() > 0) {
        stream >> m_evm_validator_group_count;
    }
    const int32_t end_pos = stream.size();
    return (begin_pos - end_pos);
}

xissue_detail_v2::operator xissue_detail_v1() const {
    xissue_detail_v1 v1;
    v1.onchain_timer_round = onchain_timer_round;
    v1.m_zec_vote_contract_height = m_zec_vote_contract_height;
    v1.m_zec_workload_contract_height = m_zec_workload_contract_height;
    v1.m_zec_reward_contract_height = m_zec_reward_contract_height;
    v1.m_edge_reward_ratio = m_edge_reward_ratio;
    v1.m_archive_reward_ratio = m_archive_reward_ratio;
    v1.m_validator_reward_ratio = m_validator_reward_ratio;
    v1.m_auditor_reward_ratio = m_auditor_reward_ratio;
    v1.m_vote_reward_ratio = m_vote_reward_ratio;
    v1.m_governance_reward_ratio = m_governance_reward_ratio;
    v1.m_auditor_group_count = m_auditor_group_count;
    v1.m_validator_group_count = m_validator_group_count;
    for (auto const & r : m_node_rewards) {
        v1.m_node_rewards[r.first] = static_cast<reward_detail_v1>(r.second);
    }
    return v1;
}

xtop_allowance::xtop_allowance(data_type d) noexcept(std::is_nothrow_move_constructible<data_type>::value) : data_{std::move(d)} {
}

xtop_allowance::iterator xtop_allowance::begin() noexcept {
    return data_.begin();
}

xtop_allowance::const_iterator xtop_allowance::begin() const noexcept {
    return data_.begin();
}

xtop_allowance::const_iterator xtop_allowance::cbegin() const noexcept {
    return data_.cbegin();
}

xtop_allowance::iterator xtop_allowance::end() noexcept {
    return data_.end();
}

xtop_allowance::const_iterator xtop_allowance::end() const noexcept {
    return data_.end();
}

xtop_allowance::const_iterator xtop_allowance::cend() const noexcept {
    return data_.cend();
}

bool xtop_allowance::empty() const noexcept {
    return data_.empty();
}

xtop_allowance::size_type xtop_allowance::size() const noexcept {
    return data_.size();
}

std::pair<xtop_allowance::iterator, bool> xtop_allowance::insert(value_type const & value) {
    return data_.insert(value);
}

xtop_allowance::iterator xtop_allowance::insert(const_iterator hint, const value_type & value) {
    return data_.insert(hint, value);
}

xtop_allowance::size_type xtop_allowance::count(key_type const & key) const {
    return data_.count(key);
}

xtop_allowance::iterator xtop_allowance::find(key_type const & key) {
    return data_.find(key);
}

xtop_allowance::const_iterator xtop_allowance::find(key_type const & key) const {
    return data_.find(key);
}

xtop_allowance::data_type const & xtop_allowance::raw_data() const noexcept {
    return data_;
}

NS_END3
