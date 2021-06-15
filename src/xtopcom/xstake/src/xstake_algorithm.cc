// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xstake/xstake_algorithm.h"

#include "xbasic/xutility.h"
#include "xdata/xnative_contract_address.h"

NS_BEG2(top, xstake)

bool check_registered_nodes_active(std::map<std::string, std::string> const & nodes) {
    uint32_t auditor_num = 0;
    uint32_t validator_num = 0;
    uint32_t archive_num = 0;
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

        xdbg("[check_registered_nodes_active] account: %s, if_adv: %d, if_validator: %d, if_archive: %d, if_edge: %d, votes: %llu\n",
             reg_node_info.m_account.c_str(),
             reg_node_info.is_valid_auditor_node(),
             reg_node_info.is_validator_node(),
             reg_node_info.is_archive_node(),
             reg_node_info.is_edge_node(),
             reg_node_info.m_vote_amount);

        if (reg_node_info.is_invalid_node())
            continue;

        if (reg_node_info.is_valid_auditor_node()) {
            auditor_num++;
        }
        if (reg_node_info.is_validator_node()) {
            validator_num++;
        }
        if (reg_node_info.is_archive_node()) {
            archive_num++;
        }
        if (reg_node_info.is_edge_node()) {
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
        "total_votes: %llu, TOTAL_VOTES_MIN: %llu",
        auditor_num,
        CFG_AUDITORS_MIN,
        validator_num,
        CFG_VALIDATORS_MIN,
        archive_num,
        CFG_ARCHIVES_MIN,
        edge_num,
        CFG_EDGES_MIN,
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

bool xreg_node_info::rec() const noexcept { return is_rec_node(); }
bool xreg_node_info::zec() const noexcept { return is_zec_node(); }
bool xreg_node_info::auditor() const noexcept { return is_valid_auditor_node(); }
bool xreg_node_info::validator() const noexcept { return is_validator_node(); }
bool xreg_node_info::edge() const noexcept { return is_edge_node(); }
bool xreg_node_info::archive() const noexcept { return is_valid_archive_node(); }
bool xreg_node_info::full_node() const noexcept {
    return common::has<common::xrole_type_t::full_node>(m_registered_role);
}

uint64_t xreg_node_info::rec_stake() const noexcept {
    uint64_t stake = 0;
    if (is_rec_node()) {
        stake = m_account_mortgage / TOP_UNIT + m_vote_amount / 2;
    }
    return stake;
}

uint64_t xreg_node_info::zec_stake() const noexcept {
    uint64_t stake = 0;
    if (is_zec_node()) {
        stake = m_account_mortgage / TOP_UNIT + m_vote_amount / 2;
    }
    return stake;
}
uint64_t xreg_node_info::auditor_stake() const noexcept { return get_auditor_stake(); }
uint64_t xreg_node_info::validator_stake() const noexcept { return get_validator_stake(); };
uint64_t xreg_node_info::edge_stake() const noexcept { return 0; }
uint64_t xreg_node_info::archive_stake() const noexcept { return 0; }
uint64_t xreg_node_info::full_node_stake() const noexcept { return 0; }

int32_t xreg_node_info::do_write(base::xstream_t & stream) const {
    const int32_t begin_pos = stream.size();
    stream << m_account.value();
    stream << m_account_mortgage;
    ENUM_SERIALIZE(stream, m_registered_role);
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
    stream << m_genesis_node;
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
    ENUM_DESERIALIZE(stream, m_registered_role);
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
    stream >> m_genesis_node;
    stream >> m_network_ids;
    stream >> nickname;
    stream >> consensus_public_key;
    const int32_t end_pos = stream.size();
    return (begin_pos - end_pos);
}

std::int32_t cluster_workload_t::do_write(base::xstream_t & stream) const {
    auto const begin = stream.size();
    stream << cluster_id;
    stream << cluster_total_workload;
    MAP_SERIALIZE_SIMPLE(stream, m_leader_count);
    auto const end = stream.size();
    return end - begin;
}

std::int32_t cluster_workload_t::do_read(base::xstream_t & stream) {
    auto const begin = stream.size();
    stream >> cluster_id;
    stream >> cluster_total_workload;
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

void xreg_node_info::slash_credit_score(common::xnode_type_t node_type) {
    uint64_t slash_numerator{0};
    if (common::has<common::xnode_type_t::validator>(node_type)) {
        slash_numerator = XGET_ONCHAIN_GOVERNANCE_PARAMETER(backward_validator_slash_credit);
        auto config_min = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_credit);
        if (slash_numerator > m_validator_credit_numerator) {
            xwarn("[slash_credit_score] slash validator credit to min!");
            m_validator_credit_numerator = config_min;
            return;
        }

        m_validator_credit_numerator -= slash_numerator;
        if (m_validator_credit_numerator < config_min) {
            m_validator_credit_numerator = config_min;
        }


    } else if (common::has<common::xnode_type_t::auditor>(node_type)) {
        slash_numerator = XGET_ONCHAIN_GOVERNANCE_PARAMETER(backward_auditor_slash_credit);
        auto config_min = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_credit);
        if (slash_numerator > m_auditor_credit_numerator) {
            xwarn("[slash_credit_score] slash auditor credit to min!");
            m_auditor_credit_numerator = config_min;
            return;
        }

        m_auditor_credit_numerator -= slash_numerator;
        if (m_auditor_credit_numerator < config_min) {
            m_auditor_credit_numerator = config_min;
        }

    }

}

void xreg_node_info::award_credit_score(common::xnode_type_t node_type) {
    uint64_t award_numerator{0};
    if (common::has<common::xnode_type_t::validator>(node_type)) {
        award_numerator = XGET_ONCHAIN_GOVERNANCE_PARAMETER(award_validator_credit);
        m_validator_credit_numerator += award_numerator;
        if (m_validator_credit_numerator > m_validator_credit_denominator) {
            xwarn("[award_credit_score] award validator credit up to max!");
            m_validator_credit_numerator = m_validator_credit_denominator;
            return;
        }

    } else if (common::has<common::xnode_type_t::auditor>(node_type)) {
        award_numerator = XGET_ONCHAIN_GOVERNANCE_PARAMETER(award_auditor_credit);
        m_auditor_credit_numerator += award_numerator;
        if (m_auditor_credit_numerator > m_auditor_credit_denominator) {
            xwarn("[award_credit_score] award auditor credit up to max!");
            m_auditor_credit_numerator = m_auditor_credit_denominator;
            return;
        }
    }


}

xreg_node_info get_reg_info(observer_ptr<store::xstore_face_t> const & store, common::xaccount_address_t const & node_addr) {
    std::string value_str;
    int         ret = store->map_get(top::sys_contract_rec_registration_addr, xstake::XPORPERTY_CONTRACT_REG_KEY, node_addr.value(), value_str);

    if (ret != store::xstore_success || value_str.empty()) {
        xwarn("[get_reg_info] get node register info fail, node_addr: %s", node_addr.value().c_str());
        return xreg_node_info{};
    }

    xstake::xreg_node_info node_info;
    base::xstream_t        stream(base::xcontext_t::instance(), (uint8_t *)value_str.c_str(), (uint32_t)value_str.size());

    node_info.serialize_from(stream);
    return node_info;


}

std::string xissue_detail::to_string() const {
    base::xstream_t stream(base::xcontext_t::instance());
    serialize_to(stream);
    return std::string((const char *)stream.data(), stream.size());
}

int32_t xissue_detail::from_string(std::string const & s) {
    base::xstream_t _stream(base::xcontext_t::instance(), (uint8_t *)s.data(), (int32_t)s.size());
    int32_t ret = serialize_from(_stream);
    if (ret <= 0) {
        xerror("serialize_from_string fail. ret=%d,bin_data_size=%d", ret, s.size());
    }
    return ret;
}

int32_t xissue_detail::do_write(base::xstream_t & stream) const {
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

int32_t xissue_detail::do_read(base::xstream_t & stream) {
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

NS_END2
