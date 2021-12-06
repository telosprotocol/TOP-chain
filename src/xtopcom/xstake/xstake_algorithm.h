// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xmem.h"
#include "xbase/xutl.h"
#include "xbasic/uint128_t.h"
#include "xbasic/xcrypto_key.h"
#include "xbasic/xserializable_based_on.h"
#include "xbasic/xserialize_face.h"
#include "xcommon/xaddress.h"
#include "xcommon/xlogic_time.h"
#include "xcommon/xrole_type.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xdata_common.h"
#include "xdata/xtableblock.h"
#include "xstore/xstore_error.h"
#include "xstore/xstore_face.h"

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cstdint>
#include <ctime>
#include <set>
#include <string>
#include <vector>

NS_BEG2(top, xstake)

XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_REG_KEY = "@101";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_TIME_KEY = "@102";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_WORKLOAD_KEY = "@103";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_SHARD_KEY = "@104";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_TICKETS_KEY = "@105";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_POLLABLE_KEY = "@107";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_TASK_KEY = "@111";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_VOTES_KEY_BASE = "@112";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_VOTES_KEY1 = "@112-1";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_VOTES_KEY2 = "@112-2";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_VOTES_KEY3 = "@112-3";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_VOTES_KEY4 = "@112-4";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_MAX_TIME_KEY = "@115";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_AWARD_KEY = "@118";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_VALIDATOR_KEY = "@120";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY_BASE = "@121";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY1 = "@121-1";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY2 = "@121-2";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY3 = "@121-3";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY4 = "@121-4";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_NODE_REWARD_KEY = "@124";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_VALIDATOR_WORKLOAD_KEY = "@125";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_TABLEBLOCK_HEIGHT_KEY = "@126";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_START_HEIGHT_KEY = "@127";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_REFUND_KEY = "@128";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_GENESIS_STAGE_KEY = "@129";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_TGAS_KEY = "@130";

// slash related
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY = "@131";
XINLINE_CONSTEXPR const char * XPROPERTY_CONTRACT_SLASH_INFO_KEY = "@132";
XINLINE_CONSTEXPR const char * XPROPERTY_CONTRACT_TABLEBLOCK_NUM_KEY = "@133";
XINLINE_CONSTEXPR const char * XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY = "@134";

XINLINE_CONSTEXPR const char * XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE = "@141";
XINLINE_CONSTEXPR const char * XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_YEARLY = "@142";
XINLINE_CONSTEXPR const char * XPROPERTY_LAST_READ_REC_REG_CONTRACT_BLOCK_HEIGHT = "@143";
XINLINE_CONSTEXPR const char * XPROPERTY_LAST_READ_REC_REG_CONTRACT_LOGIC_TIME = "@144";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_VOTE_REPORT_TIME_KEY = "@145";
XINLINE_CONSTEXPR const char * XPROPERTY_REWARD_DETAIL = "@146";

constexpr char const * XTRANSFER_ACTION{"transfer"};
constexpr char const * XZEC_WORKLOAD_CLEAR_WORKLOAD_ACTION{"clear_workload"};
constexpr char const * XREWARD_CLAIMING_ADD_NODE_REWARD{"recv_node_reward"};
constexpr char const * XREWARD_CLAIMING_ADD_VOTER_DIVIDEND_REWARD{"recv_voter_dividend_reward"};

const int XPROPERTY_SPLITED_NUM = 4;
const int DENOMINATOR = 10000;
const uint64_t REWARD_PRECISION = 1000000;

#ifdef DEBUG
constexpr common::xlogic_time_t REDEEM_INTERVAL = 2;  // 72 hours
#else
constexpr common::xlogic_time_t REDEEM_INTERVAL = 25920;  // 72 hours
#endif

// percent * 10^2 * 10^6,  total_issue_base / 10^2 / 10^6
constexpr uint32_t INITIAL_YEAR_PERCENT = 3040000;
constexpr uint32_t FINAL_YEAR_PERCENT = 2000000;
constexpr uint64_t TOTAL_ISSUE_BASE = TOTAL_ISSUANCE / 100 / 1e6;

constexpr uint64_t TOTAL_RESERVE = TOTAL_ISSUANCE * 38 / 100;
constexpr uint64_t TIMER_BLOCK_HEIGHT_PER_YEAR = 3155815;

/**
 * @brief         check if mainnet can be activated
 *
 * @param nodes   node registration property map
 * @return true
 * @return false
 */
bool check_registered_nodes_active(std::map<std::string, std::string> const & nodes, bool const enable_archive_miner);

/*#if defined(__LINUX_PLATFORM__) || defined(__MAC_PLATFORM__)
typedef __uint128_t top::xstake::uint128_t;
#else
typedef uint64_t    top::xstake::uint128_t;
#endif
*/
// typedef uint128_t top::xstake::uint128_t;

struct xreward_node_record final : xserializable_based_on<void> {
    // common::xminer_type_t m_registered_role {common::xminer_type_t::invalid};
    top::xstake::uint128_t m_accumulated{0};
    top::xstake::uint128_t m_unclaimed{0};
    uint64_t m_last_claim_time{0};
    uint64_t m_issue_time{0};

private:
    /**
     * @brief           write to stream
     *
     * @param stream
     * @return int32_t
     */
    int32_t do_write(base::xstream_t & stream) const override;
    /**
     * @brief           read from stream
     *
     * @param stream
     * @return int32_t
     */
    int32_t do_read(base::xstream_t & stream) override;
};

struct node_record_t final : public xserializable_based_on<void> {
    std::string account;
    top::xstake::uint128_t accumulated{0};
    top::xstake::uint128_t unclaimed{0};
    common::xlogic_time_t last_claim_time{0};
    uint64_t issue_time{0};

private:
    /**
     * @brief           write to stream
     *
     * @param stream
     * @return int32_t
     */
    int32_t do_write(base::xstream_t & stream) const override;
    /**
     * @brief           read from stream
     *
     * @param stream
     * @return int32_t
     */
    int32_t do_read(base::xstream_t & stream) override;
};

struct xreward_record final : public xserializable_based_on<void> {
    top::xstake::uint128_t accumulated{0};
    top::xstake::uint128_t unclaimed{0};
    std::vector<node_record_t> node_rewards;
    common::xlogic_time_t last_claim_time{0};
    uint64_t issue_time{0};

private:
    /**
     * @brief           write to stream
     *
     * @param stream
     * @return int32_t
     */
    int32_t do_write(base::xstream_t & stream) const override;
    /**
     * @brief           read from stream
     *
     * @param stream
     * @return int32_t
     */
    int32_t do_read(base::xstream_t & stream) override;
};

struct xaccumulated_reward_record final : public xserializable_based_on<void> {
    common::xlogic_time_t last_issuance_time{0};
    top::xstake::uint128_t issued_until_last_year_end{0};

private:
    /**
     * @brief           write to stream
     *
     * @param stream
     * @return int32_t
     */
    int32_t do_write(base::xstream_t & stream) const override;
    /**
     * @brief           read from stream
     *
     * @param stream
     * @return int32_t
     */
    int32_t do_read(base::xstream_t & stream) override;
};

struct xrefund_info final : public xserializable_based_on<void> {
    uint64_t refund_amount{0};
    common::xlogic_time_t create_time{0};

private:
    /**
     * @brief           write to stream
     *
     * @param stream
     * @return int32_t
     */
    int32_t do_write(base::xstream_t & stream) const override;
    /**
     * @brief           read from stream
     *
     * @param stream
     * @return int32_t
     */
    int32_t do_read(base::xstream_t & stream) override;
};

struct account_stake_t final {
    account_stake_t(account_stake_t const &) = default;
    account_stake_t & operator=(account_stake_t const &) = default;
    account_stake_t(account_stake_t &&) = default;
    account_stake_t & operator=(account_stake_t &&) = default;
    ~account_stake_t() = default;

    /**
     * @brief Construct a new account stake t object
     *
     * @param in_account
     * @param in_stake
     */
    account_stake_t(std::string const & in_account, uint64_t in_stake) : account{in_account}, stake{in_stake} {
    }

    /**
     * @brief less than other's stake
     *
     * @param other
     * @return true
     * @return false
     */
    bool operator<(account_stake_t const & other) const noexcept {
        if (stake != other.stake) {
            return stake < other.stake;
        }

        return account < other.account;
    }

    /**
     * @brief greater than other's stake
     *
     * @param other
     * @return true
     * @return false
     */
    bool operator>(account_stake_t const & other) const noexcept {
        return other < *this;
    }

    std::string account;
    uint64_t stake;
};

template <common::xminer_type_t MinerTypeV>
uint64_t minimal_deposit_of();

template <>
uint64_t minimal_deposit_of<common::xminer_type_t::edge>();

template <>
uint64_t minimal_deposit_of<common::xminer_type_t::archive>();

template <>
uint64_t minimal_deposit_of<common::xminer_type_t::exchange>();

template <>
uint64_t minimal_deposit_of<common::xminer_type_t::advance>();

template <>
uint64_t minimal_deposit_of<common::xminer_type_t::validator>();

template <common::xnode_type_t NodeTypeV>
bool could_be(common::xminer_type_t const miner_type);

template <>
bool could_be<common::xnode_type_t::rec>(common::xminer_type_t const miner_type);

template <>
bool could_be<common::xnode_type_t::zec>(common::xminer_type_t const miner_type);

template <>
bool could_be<common::xnode_type_t::consensus_auditor>(common::xminer_type_t const miner_type);

template <>
bool could_be<common::xnode_type_t::auditor>(common::xminer_type_t const miner_type);

template <>
bool could_be<common::xnode_type_t::consensus_validator>(common::xminer_type_t const miner_type);

template <>
bool could_be<common::xnode_type_t::validator>(common::xminer_type_t const miner_type);

template <>
bool could_be<common::xnode_type_t::storage_archive>(common::xminer_type_t const miner_type);

template <>
bool could_be<common::xnode_type_t::archive>(common::xminer_type_t const miner_type);

template <>
bool could_be<common::xnode_type_t::storage_exchange>(common::xminer_type_t const miner_type);

template <>
bool could_be<common::xnode_type_t::exchange>(common::xminer_type_t const miner_type);

template <>
bool could_be<common::xnode_type_t::edge>(common::xminer_type_t const miner_type);

struct xreg_node_info final : public xserializable_based_on<void> {
public:
    xreg_node_info() = default;
    xreg_node_info(xreg_node_info const &) = default;
    xreg_node_info & operator=(xreg_node_info const &) = default;
    xreg_node_info(xreg_node_info &&) = default;
    xreg_node_info & operator=(xreg_node_info &&) = default;
    ~xreg_node_info() override = default;

    /// @brief Check to see if this node could be an rec based on miner type.
    bool could_be_rec() const noexcept;

    /// @brief Check to see if this node could be a zec based on miner type.
    bool could_be_zec() const noexcept;

    /// @brief Check to see if this node could be an auditor based on miner type.
    bool could_be_auditor() const noexcept;

    /// @brief Check to see if this node could be a validator based on miner type.
    bool could_be_validator() const noexcept;

    /// @brief Check to see if this node could be an archive based on miner type.
    bool could_be_archive() const noexcept;

    /// @brief Check to see if this node could be an archive based on minter type.
    bool legacy_could_be_archive() const noexcept;

    /// @brief Check to see if this node could be an edge based on miner type.
    bool could_be_edge() const noexcept;

    /// @brief Check to see if this node could be a exchange node based on miner type.
    bool could_be_exchange() const noexcept;

    /// @brief Check to see if this node can be an rec based on miner type and other information (e.g. deposit, amount of received tickets).
    bool can_be_rec() const noexcept;

    /// @brief Check to see if this node can be a zec based on miner type and other information (e.g. deposit, amount of received tickets).
    bool can_be_zec() const noexcept;

    /// @brief Check to see if this node can be a validator based on miner type and other information (e.g. deposit, amount of received tickets).
    bool can_be_validator() const noexcept;

    /// @brief Check to see if this node can be an auditor based on miner type and other information (e.g. deposit, amount of received tickets).
    bool can_be_auditor() const noexcept;

    /// @brief Check to see if this node can be an archive based on miner type and other information (e.g. deposit, amount of received tickets).
    bool can_be_archive() const noexcept;

    /// @brief Check to see if this node can be an archive based on miner type and other information (e.g. deposit, amount of received tickets).
    bool legacy_can_be_archive() const noexcept;

    /// @brief Check to see if this node can be an edge based on miner type and other information (e.g. deposit, amount of received tickets).
    bool can_be_edge() const noexcept;

    /// @brief Check to see if this node can be a exchange based on miner type and other information (e.g. deposit, amount of received tickets).
    bool can_be_exchange() const noexcept;

    /**
     * @brief check if self is an invlid node
     *
     * @return true
     * @return false
     */
    bool is_invalid_node() const noexcept {
        return m_registered_role == common::xminer_type_t::invalid;
    }

    /**
     * @brief check if self is a genesis node
     *
     * @return true
     * @return false
     */
    bool is_genesis_node() const noexcept {
        return m_genesis_node;
    }

    uint64_t deposit() const noexcept;

    /**
     * @brief get rec stake
     *
     * @return uint64_t
     */
    uint64_t rec_stake() const noexcept;
    /**
     * @brief get zec stake
     *
     * @return uint64_t
     */
    uint64_t zec_stake() const noexcept;
    /**
     * @brief get auditor stake
     *
     * @return uint64_t
     */
    uint64_t auditor_stake() const noexcept;
    /**
     * @brief get validator stake
     *
     * @return uint64_t
     */
    uint64_t validator_stake() const noexcept;
    /**
     * @brief get edge stake
     *
     * @return uint64_t
     */
    uint64_t edge_stake() const noexcept;
    /**
     * @brief get archive stake
     *
     * @return uint64_t
     */
    uint64_t archive_stake() const noexcept;

    uint64_t exchange_stake() const noexcept;

    /**
     * @brief Get role type
     *
     * @return common::xminer_type_t
     */
    common::xminer_type_t get_role_type() const noexcept {
        return m_registered_role;
    }

    /**
     * @brief Get auditor stake
     *
     * @return uint64_t
     */
    uint64_t get_auditor_stake() const noexcept {
        uint64_t stake = 0;
        if (could_be_auditor()) {
            stake = (m_account_mortgage / TOP_UNIT + m_vote_amount / 2) * m_auditor_credit_numerator / m_auditor_credit_denominator;
        }
        return stake;
    }

    /**
     * @brief Get validator stake
     *
     * @return uint64_t
     */
    uint64_t get_validator_stake() const noexcept {
        uint64_t stake = 0;
        if (could_be_validator()) {
            auto max_validator_stake = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_validator_stake);
            stake = (uint64_t)sqrt((m_account_mortgage / TOP_UNIT + m_vote_amount / 2) * m_validator_credit_numerator / m_validator_credit_denominator);
            stake = stake < max_validator_stake ? stake : max_validator_stake;
        }
        return stake;
    }

    template <common::xminer_type_t MinerTypeV>
    bool miner_type_has() const noexcept {
        return common::has<MinerTypeV>(m_registered_role);
    }

    /**
     * @brief Get required min deposit
     *
     * @return uint64_t
     */
    uint64_t get_required_min_deposit() const noexcept;

    /**
     * @brief deduce credit score
     *
     * @param node_type
     */
    void slash_credit_score(common::xnode_type_t node_type);
    /**
     * @brief increase credit score
     *
     * @param node_type
     */
    void award_credit_score(common::xnode_type_t node_type);

    common::xaccount_address_t m_account{};
    uint64_t m_account_mortgage{0};
    common::xminer_type_t m_registered_role{common::xminer_type_t::invalid};
    uint64_t m_vote_amount{0};
    uint64_t m_auditor_credit_numerator{0};
    uint64_t m_auditor_credit_denominator{1000000};
    uint64_t m_validator_credit_numerator{0};
    uint64_t m_validator_credit_denominator{1000000};

    uint m_support_ratio_numerator{0};  // dividends to voters
    uint m_support_ratio_denominator{100};
    // uint64_t    m_stake {0};
    // stake_info  m_stake_info {};
    common::xlogic_time_t m_last_update_time{0};
    bool m_genesis_node{false};
    // uint32_t    m_network_id {0};
    std::set<common::xnetwork_id_t> m_network_ids;
    std::string nickname;
    xpublic_key_t consensus_public_key;

private:
    /**
     * @brief write to stream
     *
     * @param stream
     * @return int32_t
     */
    int32_t do_write(base::xstream_t & stream) const override;
    /**
     * @brief read from stream
     *
     * @param stream
     * @return int32_t
     */
    int32_t do_read(base::xstream_t & stream) override;
};

class xtop_account_registration_info final : public xserializable_based_on<void> {
private:
    common::xaccount_address_t m_account{};
    uint64_t m_account_mortgage{0};
    common::xminer_type_t m_registered_role{common::xminer_type_t::invalid};
    uint64_t m_vote_amount{0};
    uint64_t m_auditor_credit_numerator{0};
    uint64_t m_auditor_credit_denominator{1000000};
    uint64_t m_validator_credit_numerator{0};
    uint64_t m_validator_credit_denominator{1000000};

    uint m_support_ratio_numerator{0};  // dividends to voters
    uint m_support_ratio_denominator{100};
    common::xlogic_time_t m_last_update_time{0};
    bool m_genesis_node{false};
    std::set<common::xnetwork_id_t> m_network_ids;
    std::string nickname;
    xpublic_key_t consensus_public_key;

public:
    bool rec() const noexcept;
    bool zec() const noexcept;
    bool auditor() const noexcept;
    bool validator() const noexcept;
    bool edge() const noexcept;
    bool archive() const noexcept;

    uint64_t rec_stake() const noexcept;
    uint64_t zec_stake() const noexcept;
    uint64_t auditor_stake() const noexcept;
    uint64_t validator_stake() const noexcept;
    uint64_t edge_stake() const noexcept;
    uint64_t archive_stake() const noexcept;

    common::xminer_type_t role() const noexcept;

    /**
     * @brief write to stream
     *
     * @param stream
     * @return int32_t
     */
    int32_t do_write(base::xstream_t & stream) const override {
        return 0;
    }
    /**
     * @brief read from stream
     *
     * @param stream
     * @return int32_t
     */
    int32_t do_read(base::xstream_t & stream) override {
        return 0;
    }
};
using xaccount_registration_info_t = xtop_account_registration_info;

/**
 * @brief Get the reg info object from node_addr
 *
 * @param store store
 * @param node_addr node address
 * @return xreg_node_info
 */
xreg_node_info get_reg_info(observer_ptr<store::xstore_face_t> const & store, common::xaccount_address_t const & node_addr);

struct xslash_info final : public xserializable_based_on<void> {
public:
    common::xlogic_time_t m_punish_time{0};
    common::xlogic_time_t m_staking_lock_time{0};
    uint32_t m_punish_staking{0};

private:
    /**
     * @brief write to stream
     *
     * @param stream
     * @return int32_t
     */
    int32_t do_write(base::xstream_t & stream) const override;
    /**
     * @brief read from stream
     *
     * @param stream
     * @return int32_t
     */
    int32_t do_read(base::xstream_t & stream) override;
};

struct cluster_workload_t final : public xserializable_based_on<void> {
    std::string cluster_id;
    uint32_t cluster_total_workload{0};
    std::map<std::string, uint32_t> m_leader_count;

private:
    /**
     * @brief write to stream
     *
     * @param stream
     * @return std::int32_t
     */
    std::int32_t do_write(base::xstream_t & stream) const override;
    /**
     * @brief read from stream
     *
     * @param stream
     * @return std::int32_t
     */
    std::int32_t do_read(base::xstream_t & stream) override;
};
using xgroup_workload_t = cluster_workload_t;

struct xactivation_record final : public xserializable_based_on<void> {
    int activated{0};
    common::xlogic_time_t activation_time{0};

private:
    /**
     * @brief write to stream
     *
     * @param stream
     * @return int32_t
     */
    int32_t do_write(base::xstream_t & stream) const override;
    /**
     * @brief read from stream
     *
     * @param stream
     * @return int32_t
     */
    int32_t do_read(base::xstream_t & stream) override;
};
struct xreward_dispatch_task final : public xserializable_based_on<void> {
    uint64_t onchain_timer_round;
    std::string contract;
    std::string action;
    std::string params;

private:
    /**
     * @brief write to stream
     *
     * @param stream
     * @return int32_t
     */
    int32_t do_write(base::xstream_t & stream) const override {
        const int32_t begin_pos = stream.size();
        stream << onchain_timer_round;
        stream << contract;
        stream << action;
        stream << params;
        const int32_t end_pos = stream.size();
        return (end_pos - begin_pos);
    }

    /**
     * @brief read from stream
     *
     * @param stream
     * @return int32_t
     */
    int32_t do_read(base::xstream_t & stream) override {
        const int32_t begin_pos = stream.size();
        stream >> onchain_timer_round;
        stream >> contract;
        stream >> action;
        stream >> params;
        const int32_t end_pos = stream.size();
        return (begin_pos - end_pos);
    }
};

struct reward_detail final : public xserializable_based_on<void> {
    top::xstake::uint128_t m_edge_reward{0};
    top::xstake::uint128_t m_archive_reward{0};
    top::xstake::uint128_t m_validator_reward{0};
    top::xstake::uint128_t m_auditor_reward{0};
    top::xstake::uint128_t m_vote_reward{0};
    top::xstake::uint128_t m_self_reward{0};

private:
    int32_t do_write(base::xstream_t & stream) const override {
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

    /**
     * @brief read from stream
     *
     * @param stream
     * @return int32_t
     */
    int32_t do_read(base::xstream_t & stream) override {
        const int32_t begin_pos = stream.size();
        stream >> m_edge_reward;
        stream >> m_archive_reward;
        stream >> m_validator_reward;
        stream >> m_auditor_reward;
        stream >> m_vote_reward;
        if (stream.size() > 0) {
            stream >> m_self_reward;
        }
        const int32_t end_pos = stream.size();
        return (begin_pos - end_pos);
    }
};

class xissue_detail final
  : public xenable_to_string_t<xissue_detail>
  , public xserializable_based_on<void> {
public:
    uint64_t onchain_timer_round{0};
    uint64_t m_zec_vote_contract_height{0};
    uint64_t m_zec_workload_contract_height{0};
    uint64_t m_zec_reward_contract_height{0};
    uint16_t m_edge_reward_ratio{0};
    uint16_t m_archive_reward_ratio{0};
    uint16_t m_validator_reward_ratio{0};
    uint16_t m_auditor_reward_ratio{0};
    uint16_t m_vote_reward_ratio{0};
    uint16_t m_governance_reward_ratio{0};
    uint64_t m_auditor_group_count{0};
    uint64_t m_validator_group_count{0};
    std::map<std::string, reward_detail> m_node_rewards;

public:
    std::string to_string() const override;
    int32_t from_string(std::string const & s) override;
    using xenable_to_string_t<xissue_detail>::to_string;
    using xenable_to_string_t<xissue_detail>::from_string;

private:
    int32_t do_write(base::xstream_t & stream) const override;

    /**
     * @brief read from stream
     *
     * @param stream
     * @return int32_t
     */
    int32_t do_read(base::xstream_t & stream) override;
};

NS_END2
