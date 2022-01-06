#include "../xdb_reset.h"

#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xslash.h"
#include "xstake/xstake_algorithm.h"
#include "xvledger/xvledger.h"

#include <fstream>
#include <iostream>

using namespace top::base;
using namespace top::data;
using namespace top::xstake;

NS_BEG2(top, db_reset)

std::vector<std::pair<std::string, std::string>> stake_map_string_string_pair_list = {
    std::make_pair(sys_contract_rec_registration_addr, XPORPERTY_CONTRACT_TICKETS_KEY),
    std::make_pair(sys_contract_rec_registration_addr, XPORPERTY_CONTRACT_REG_KEY),
    std::make_pair(sys_contract_rec_registration_addr, XPROPERTY_CONTRACT_SLASH_INFO_KEY),
    std::make_pair(sys_contract_rec_registration_addr, XPORPERTY_CONTRACT_REFUND_KEY),
    std::make_pair(sys_contract_zec_reward_addr, XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE),
    std::make_pair(sys_contract_zec_reward_addr, XPORPERTY_CONTRACT_TASK_KEY),
    std::make_pair(sys_contract_zec_reward_addr, XPORPERTY_CONTRACT_WORKLOAD_KEY),
    std::make_pair(sys_contract_zec_reward_addr, XPORPERTY_CONTRACT_VALIDATOR_WORKLOAD_KEY),
    std::make_pair(sys_contract_zec_slash_info_addr, XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY),
    std::make_pair(sys_contract_zec_vote_addr, XPORPERTY_CONTRACT_TICKETS_KEY),
};

std::vector<std::pair<std::string, std::string>> stake_string_pair_list = {
    std::make_pair(sys_contract_rec_registration_addr, XPORPERTY_CONTRACT_GENESIS_STAGE_KEY),
    std::make_pair(sys_contract_zec_reward_addr, XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_YEARLY),
};

std::vector<std::pair<std::string, std::string>> table_stake_string_pair_list = {};

std::vector<std::pair<std::string, std::string>> table_stake_map_string_string_pair_list = {
    std::make_pair(sys_contract_sharding_reward_claiming_addr, XPORPERTY_CONTRACT_NODE_REWARD_KEY),
    std::make_pair(sys_contract_sharding_reward_claiming_addr, XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY1),
    std::make_pair(sys_contract_sharding_reward_claiming_addr, XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY2),
    std::make_pair(sys_contract_sharding_reward_claiming_addr, XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY3),
    std::make_pair(sys_contract_sharding_reward_claiming_addr, XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY4),
    std::make_pair(sys_contract_sharding_vote_addr, XPORPERTY_CONTRACT_POLLABLE_KEY),
    std::make_pair(sys_contract_sharding_vote_addr, XPORPERTY_CONTRACT_VOTES_KEY1),
    std::make_pair(sys_contract_sharding_vote_addr, XPORPERTY_CONTRACT_VOTES_KEY2),
    std::make_pair(sys_contract_sharding_vote_addr, XPORPERTY_CONTRACT_VOTES_KEY3),
    std::make_pair(sys_contract_sharding_vote_addr, XPORPERTY_CONTRACT_VOTES_KEY4),
};

xdb_reset_t::xdb_reset_t(observer_ptr<base::xvblockstore_t> const & blockstore) : m_blockstore(blockstore) {
}

void xdb_reset_t::generate_reset_check_file(std::vector<std::string> const & sys_contract_accounts_vec, std::vector<std::string> const & accounts) {
    json property_json;
    get_contract_stake_property_map_string_string(property_json);
    get_contract_stake_property_string(property_json);
    get_contract_table_stake_property_map_string_string(property_json);
    get_contract_table_stake_property_string(property_json);
    get_unit_set_property(sys_contract_accounts_vec, accounts, property_json);
    std::ofstream out_json("all_property_check.json");
    out_json << std::setw(4) << property_json["contract_account_parse"];
    out_json << std::setw(4) << property_json["user_account_parse"];
    std::cout << "===> all_property_check.json generated success!" << std::endl;
}

void xdb_reset_t::verify(json const & contract, json const & user) {
    std::vector<std::pair<uint32_t, std::string>> balance_property = {
        std::make_pair(0, XPROPERTY_BALANCE_AVAILABLE),
        std::make_pair(1, XPROPERTY_BALANCE_BURN),
        std::make_pair(2, XPROPERTY_BALANCE_LOCK),
        std::make_pair(3, XPROPERTY_BALANCE_PLEDGE_TGAS),
        std::make_pair(4, XPROPERTY_BALANCE_PLEDGE_VOTE),
        std::make_pair(5, XPROPERTY_LOCK_TGAS),
        std::make_pair(6, XPROPERTY_EXPIRE_VOTE_TOKEN_KEY),
        std::make_pair(7, XPROPERTY_UNVOTE_NUM),
        std::make_pair(8, XPROPERTY_LOCK_TOKEN_KEY),
    };
    std::vector<std::string> reward_property = {
        XPORPERTY_CONTRACT_NODE_REWARD_KEY,
        XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY1,
        XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY2,
        XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY3,
        XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY4,
    };
    std::vector<std::string> vote_property = {
        XPORPERTY_CONTRACT_POLLABLE_KEY,
        XPORPERTY_CONTRACT_VOTES_KEY1,
        XPORPERTY_CONTRACT_VOTES_KEY2,
        XPORPERTY_CONTRACT_VOTES_KEY3,
        XPORPERTY_CONTRACT_VOTES_KEY4,
    };
    const uint32_t check_table_num = 256;
    // balance
    std::cout << "===> balance info: " << std::endl;
    std::array<uint64_t, 9> balance = {0};
    for (auto it = contract.begin(); it != contract.end(); it++) {
        for (size_t i = 0; i < balance_property.size(); i++) {
            if (it.value().count(balance_property[i].second)) {
                balance[balance_property[i].first] += static_cast<uint64_t>(it->at(balance_property[i].second));
            }
        }
    }
    std::cout << "contract:" << std::endl;
    for (size_t i = 0; i < balance_property.size(); i++) {
        std::cout << "total " << balance_property[i].second << ": " << balance[i] << std::endl;
    }
    for (auto it = user.begin(); it != user.end(); it++) {
        for (size_t i = 0; i < balance_property.size(); i++) {
            if (it.value().count(balance_property[i].second)) {
                balance[balance_property[i].first] += static_cast<uint64_t>(it->at(balance_property[i].second));
            }
        }
    }
    std::cout << "user:" << std::endl << std::endl;
    for (size_t i = 0; i < balance_property.size(); i++) {
        std::cout << "total " << balance_property[i].second << ": " << balance[i] << std::endl;
    }
    // reward
    std::cout << "===> reward info: " << std::endl;
    uint64_t total_reward{0};
    uint64_t total_reward_decimals{0};
    for (size_t i = 0; i < check_table_num; i++) {
        std::string address = std::string{sys_contract_sharding_reward_claiming_addr} + "@" + std::to_string(i);
        auto it = contract.find(address);
        if (it == contract.end()) {
            continue;
        }
        uint64_t table_reward{0};
        uint32_t decimals{0};
        for (auto const & p : reward_property) {
            for (auto it_a = it.value()[p].begin(); it_a != it.value()[p].end(); it_a++) {
                table_reward += static_cast<uint64_t>(it_a.value()["unclaimed"]);
                decimals += static_cast<uint32_t>(it_a.value()["unclaimed_decimals"]);
            }
        }
        total_reward += table_reward;
        total_reward_decimals += decimals;
        table_reward += decimals / REWARD_PRECISION;
        if (decimals % REWARD_PRECISION != 0) {
            table_reward += 1;
        }
        if (static_cast<uint64_t>(it->at("$0")) != 0 || table_reward != 0) {
            // std::cout << "table " << i << ", $0: " << static_cast<uint64_t>(it->at("$0")) << ", calc_table: " << table_reward << ", miss: " << abs(table_reward -
            // static_cast<uint64_t>(it->at("$0"))) << std::endl;
        }
    }
    total_reward += total_reward_decimals / REWARD_PRECISION;
    total_reward_decimals %= REWARD_PRECISION;
    std::cout << "calc total reward: " << total_reward << "." << total_reward_decimals << std::endl;
    // vote info
    std::cout << std::endl << "===> vote info: " << std::endl;
    uint64_t total_vote_107{0};
    uint64_t total_vote_121{0};
    for (size_t i = 0; i < check_table_num; i++) {
        std::string address = std::string{sys_contract_sharding_vote_addr} + "@" + std::to_string(i);
        auto it = contract.find(address);
        if (it == contract.end()) {
            continue;
        }
        uint64_t table_vote_107{0};
        uint64_t table_vote_121{0};
        for (auto const & p : vote_property) {
            for (auto it_a = it.value()[p].begin(); it_a != it.value()[p].end(); it_a++) {
                if (p == XPORPERTY_CONTRACT_POLLABLE_KEY) {
                    table_vote_107 += static_cast<uint64_t>(it_a.value());
                } else {
                    for (auto it_v = it_a.value()["vote_infos"].begin(); it_v != it_a.value()["vote_infos"].end(); it_v++) {
                        table_vote_121 += static_cast<uint64_t>(it_v.value());
                    }
                }
            }
        }
        total_vote_107 += table_vote_107;
        total_vote_121 += table_vote_121;
        if (table_vote_107 != 0 || table_vote_121 != 0) {
            // std::cout << "table " << i << ", @107: " << table_vote_107 << ", @112: " << table_vote_121 << std::endl;
        }
    }
    std::cout << "calc total vote, @107: " << total_vote_107 << ", @112: " << total_vote_121 << std::endl;
}

void xdb_reset_t::get_unit_set_property(std::vector<std::string> const & sys_contract_accounts_vec, std::vector<std::string> const & accounts_vec, json & accounts_json) {
    std::set<std::string> accounts_set(accounts_vec.begin(), accounts_vec.end());
    // 1. get all new sys contract address
    std::set<std::string> sys_contract_accounts_set;
    for (auto const & account : sys_contract_accounts_vec) {
        sys_contract_accounts_set.insert(account);
    }
    // 2. delete all old sys contract contract
    for (auto const & account : accounts_set) {
        auto account_address = common::xaccount_address_t{account};
        if (!is_account_address(account_address)) {
            accounts_set.erase(account);
        }
    }
    // 3. add new sys contract address
    for (auto const & account : sys_contract_accounts_vec) {
        accounts_set.insert(account);
    }
    for (auto const & account : accounts_set) {
        base::xvaccount_t _vaddr(account);
        auto _block = m_blockstore->load_block_object(_vaddr, 0, 0, false);
        if (_block == nullptr) {
            std::cout << account << " height " << 0 << " block null!" << std::endl;
            continue;
        }
        if (_block->is_genesis_block() && _block->get_block_class() == base::enum_xvblock_class_nil) {
            // std::cout << account << " height " << 0 << " block genesis and nil!" << std::endl;
            continue;
        }
        base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(_block.get());
        if (bstate == nullptr) {
            std::cout << account << " height " << 0 << " bstate null!" << std::endl;
            continue;
        }
        xaccount_ptr_t unitstate = std::make_shared<xunit_bstate_t>(bstate.get());
        if (unitstate == nullptr) {
            std::cout << account << " height " << 0 << " unitstate null!" << std::endl;
            continue;
        }

        std::string key_j;
        auto account_address = common::xaccount_address_t{account};
        if (is_account_address(account_address)) {
            key_j = "user_account_parse";
        } else if (is_sys_contract_address(account_address) || is_sys_sharding_contract_address(account_address)) {
            key_j = "contract_account_parse";
        } else {
            continue;
        }
        // $0
        auto balance = unitstate->token_get(XPROPERTY_BALANCE_AVAILABLE);
        if (!is_account_address(account_address)) {
            if (!accounts_json["contract_account_parse"].count(account) && unitstate->token_get(XPROPERTY_BALANCE_AVAILABLE) == 0) {
                // std::cout << account << " check fail2" << std::endl;
                continue;
            }
        }
        // std::cout << account << " check ok" << std::endl;
        accounts_json[key_j][account][XPROPERTY_BALANCE_AVAILABLE] = unitstate->token_get(XPROPERTY_BALANCE_AVAILABLE) + unitstate->token_get(XPROPERTY_BALANCE_LOCK);
        // $a
        accounts_json[key_j][account][XPROPERTY_BALANCE_BURN] = unitstate->token_get(XPROPERTY_BALANCE_BURN);
        // $c
        accounts_json[key_j][account][XPROPERTY_BALANCE_PLEDGE_TGAS] = unitstate->token_get(XPROPERTY_BALANCE_PLEDGE_TGAS);
        // $d
        accounts_json[key_j][account][XPROPERTY_BALANCE_PLEDGE_VOTE] = unitstate->token_get(XPROPERTY_BALANCE_PLEDGE_VOTE);
        // $00
        accounts_json[key_j][account][XPROPERTY_LOCK_TGAS] = unitstate->uint64_property_get(XPROPERTY_LOCK_TGAS);
        // $03
        std::map<std::string, std::string> pledge_votes;
        json j;
        if (unitstate->map_get(XPROPERTY_PLEDGE_VOTE_KEY, pledge_votes)) {
            int cnt{0};
            for (auto & v : pledge_votes) {
                uint64_t vote_num{0};
                uint16_t duration{0};
                uint64_t lock_time{0};
                base::xstream_t key{xcontext_t::instance(), (uint8_t *)v.first.data(), static_cast<uint32_t>(v.first.size())};
                key >> duration;
                key >> lock_time;
                base::xstream_t val{xcontext_t::instance(), (uint8_t *)v.second.data(), static_cast<uint32_t>(v.second.size())};
                val >> vote_num;
                json jv;
                jv["vote_num"] = vote_num;
                jv["duration"] = duration;
                jv["lock_time"] = lock_time;
                j[base::xstring_utl::tostring(++cnt)] = jv;
            }
        }
        accounts_json[key_j][account][XPROPERTY_PLEDGE_VOTE_KEY] = j;
        // $04
        std::string expire_votes;
        json j2 = 0;
        if (unitstate->string_get(XPROPERTY_EXPIRE_VOTE_TOKEN_KEY, expire_votes)) {
            if (expire_votes.empty()) {
                j2 = base::xstring_utl::touint64(expire_votes);
            }
        }
        accounts_json[key_j][account][XPROPERTY_EXPIRE_VOTE_TOKEN_KEY] = j2;
        // $05
        accounts_json[key_j][account][XPROPERTY_UNVOTE_NUM] = unitstate->uint64_property_get(XPROPERTY_UNVOTE_NUM);
        // $07
        if (is_account_address(account_address)) {
            accounts_json[key_j][account][XPROPERTY_ACCOUNT_CREATE_TIME] = unitstate->uint64_property_get(XPROPERTY_ACCOUNT_CREATE_TIME);
        }
    }
}

void xdb_reset_t::get_contract_stake_property_string(json & stake_json) {
    for (auto const & _pair : stake_string_pair_list) {
        std::string const addr = _pair.first;
        std::string const property_key = _pair.second;

        base::xvaccount_t _vaddr(addr);
        auto _block = m_blockstore->load_block_object(_vaddr, 0, 0, false);
        if (_block == nullptr) {
            std::cout << addr << " height " << 0 << " block null!" << std::endl;
            continue;
        }
        if (_block->is_genesis_block() && _block->get_block_class() == base::enum_xvblock_class_nil) {
            std::cout << addr << " height " << 0 << " block genesis and nil!" << std::endl;
            continue;
        }
        base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(_block.get());
        if (bstate == nullptr) {
            std::cout << addr << " height " << 0 << " bstate null!" << std::endl;
            continue;
        }
        xaccount_ptr_t unitstate = std::make_shared<xunit_bstate_t>(bstate.get());
        if (unitstate == nullptr) {
            std::cout << addr << " height " << 0 << " unitstate null!" << std::endl;
            continue;
        }

        std::string value;
        if (!unitstate->string_get(property_key, value)) {
            printf("[get_genesis_stage] contract_address: %s, property_name: %s, error", addr.c_str(), property_key.c_str());
            continue;
        }
        json j;
        if (property_key == XPORPERTY_CONTRACT_GENESIS_STAGE_KEY) {
            xactivation_record record;
            if (!value.empty()) {
                base::xstream_t stream{xcontext_t::instance(), (uint8_t *)value.data(), static_cast<uint32_t>(value.size())};
                record.serialize_from(stream);
            }
            j["activated"] = record.activated;
            j["activation_time"] = record.activation_time;
        } else if (property_key == XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_YEARLY) {
            xaccumulated_reward_record record;
            if (!value.empty()) {
                base::xstream_t stream{xcontext_t::instance(), (uint8_t *)value.data(), static_cast<uint32_t>(value.size())};
                record.serialize_from(stream);
            }
            j["last_issuance_time"] = record.last_issuance_time;
            j["issued_until_last_year_end"] = static_cast<uint64_t>(record.issued_until_last_year_end / REWARD_PRECISION);
            j["issued_until_last_year_end_decimals"] = static_cast<uint32_t>(record.issued_until_last_year_end % REWARD_PRECISION);
        }
        stake_json["contract_account_parse"][addr][property_key] = j;
    }
}

void xdb_reset_t::get_contract_stake_property_map_string_string(json & stake_json) {
    for (auto const & _pair : stake_map_string_string_pair_list) {
        std::string const addr = _pair.first;
        std::string const property_key = _pair.second;

        base::xvaccount_t _vaddr(addr);
        auto _block = m_blockstore->load_block_object(_vaddr, 0, 0, false);
        if (_block == nullptr) {
            std::cout << addr << " height " << 0 << " block null!" << std::endl;
            continue;
        }
        if (_block->is_genesis_block() && _block->get_block_class() == base::enum_xvblock_class_nil) {
            std::cout << addr << " height " << 0 << " block genesis and nil!" << std::endl;
            continue;
        }
        base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(_block.get());
        if (bstate == nullptr) {
            std::cout << addr << " height " << 0 << " bstate null!" << std::endl;
            continue;
        }
        xaccount_ptr_t unitstate = std::make_shared<xunit_bstate_t>(bstate.get());
        if (unitstate == nullptr) {
            std::cout << addr << " height " << 0 << " unitstate null!" << std::endl;
            continue;
        }

        std::map<std::string, std::string> value;
        if (!unitstate->map_get(property_key, value)) {
            printf("[get_genesis_stage] contract_address: %s, property_name: %s, error", addr.c_str(), property_key.c_str());
            continue;
        }

        json deser_res;
        if (property_key == XPORPERTY_CONTRACT_TICKETS_KEY) {
            for (auto m : value) {
                std::map<std::string, std::string> votes_table;
                auto detail = m.second;
                if (!detail.empty()) {
                    votes_table.clear();
                    base::xstream_t stream{xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
                    stream >> votes_table;
                }
                json jv;
                json jvn;
                for (auto v : votes_table) {
                    jvn[v.first] = base::xstring_utl::touint64(v.second);
                }
                jv["vote_infos"] = jvn;
                deser_res[m.first] = jv;
            }
        } else if (property_key == XPORPERTY_CONTRACT_REG_KEY) {
            for (auto m : value) {
                xreg_node_info reg_node_info;
                xstream_t stream(xcontext_t::instance(), (uint8_t *)m.second.data(), m.second.size());
                reg_node_info.serialize_from(stream);
                json j;
                j["account_addr"] = reg_node_info.m_account.value();
                j["node_deposit"] = static_cast<unsigned long long>(reg_node_info.m_account_mortgage);
                if (reg_node_info.m_genesis_node) {
                    j["registered_node_type"] = std::string{"advance,validator,edge"};
                } else {
                    j["registered_node_type"] = common::to_string(reg_node_info.miner_type());
                }
                j["vote_amount"] = static_cast<unsigned long long>(reg_node_info.m_vote_amount);
                j["auditor_credit"] = static_cast<double>(reg_node_info.m_auditor_credit_numerator) / reg_node_info.m_auditor_credit_denominator;
                j["validator_credit"] = static_cast<double>(reg_node_info.m_validator_credit_numerator) / reg_node_info.m_validator_credit_denominator;
                j["dividend_ratio"] = reg_node_info.m_support_ratio_numerator * 100 / reg_node_info.m_support_ratio_denominator;
                // j["m_stake"] = static_cast<unsigned long long>(reg_node_info.m_stake);
                j["auditor_stake"] = static_cast<unsigned long long>(reg_node_info.get_auditor_stake());
                j["validator_stake"] = static_cast<unsigned long long>(reg_node_info.get_validator_stake());
                j["rec_stake"] = static_cast<unsigned long long>(reg_node_info.rec_stake());
                j["zec_stake"] = static_cast<unsigned long long>(reg_node_info.zec_stake());
                std::string network_ids;
                for (auto const & net_id : reg_node_info.m_network_ids) {
                    network_ids += net_id.to_string() + ' ';
                }
                j["network_id"] = network_ids;
                j["nodename"] = reg_node_info.nickname;
                j["node_sign_key"] = reg_node_info.consensus_public_key.to_string();
                deser_res[m.first] = j;
            }
        } else if (property_key == XPROPERTY_CONTRACT_SLASH_INFO_KEY) {
            for (auto const & m : value) {
                xslash_info s_info;
                auto detail = m.second;
                if (!detail.empty()) {
                    base::xstream_t stream{xcontext_t::instance(), (uint8_t *)detail.data(), (uint32_t)detail.size()};
                    s_info.serialize_from(stream);
                }

                json jvn;
                jvn["punish_time"] = s_info.m_punish_time;
                jvn["staking_lock_time"] = s_info.m_staking_lock_time;
                jvn["punish_staking"] = s_info.m_punish_staking;
                deser_res[m.first] = jvn;
            }
        } else if (property_key == XPORPERTY_CONTRACT_REFUND_KEY) {
            for (auto m : value) {
                xrefund_info refund;
                auto detail = m.second;
                base::xstream_t stream{xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
                refund.serialize_from(stream);
                json jv;
                jv["refund_amount"] = refund.refund_amount;
                jv["create_time"] = refund.create_time;
                deser_res[m.first] = jv;
            }
        } else if (property_key == XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE) {
            for (auto const & m : value) {
                deser_res[m.first] = base::xstring_utl::touint64(m.second);
            }
        } else if (property_key == XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY) {
            for (auto const & m : value) {
                xunqualified_node_info_t summarize_info;
                auto detail = m.second;
                if (!detail.empty()) {
                    base::xstream_t stream{xcontext_t::instance(), (uint8_t *)detail.data(), (uint32_t)detail.size()};
                    summarize_info.serialize_from(stream);
                }
                json jvn;
                json jvn_auditor;
                for (auto const & v : summarize_info.auditor_info) {
                    json auditor_info;
                    auditor_info["vote_num"] = v.second.block_count;
                    auditor_info["subset_num"] = v.second.subset_count;
                    jvn_auditor[v.first.value()] = auditor_info;
                }

                json jvn_validator;
                for (auto const & v : summarize_info.validator_info) {
                    json validator_info;
                    validator_info["vote_num"] = v.second.block_count;
                    validator_info["subset_num"] = v.second.subset_count;
                    jvn_validator[v.first.value()] = validator_info;
                }

                jvn["auditor"] = jvn_auditor;
                jvn["validator"] = jvn_validator;
                deser_res["unqualified_node"] = jvn;
            }
        } else if (property_key == XPORPERTY_CONTRACT_TASK_KEY) {
            for (auto m : value) {
                auto const & detail = m.second;
                if (detail.empty())
                    continue;

                xreward_dispatch_task task;
                base::xstream_t stream{xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
                task.serialize_from(stream);
                json jv;
                json jvn;
                int no = 0;
                jv["task_id"] = m.first;
                jv["onchain_timer_round"] = (xJson::UInt64)task.onchain_timer_round;
                jv["contract"] = task.contract;
                jv["action"] = task.action;
                if (task.action == XREWARD_CLAIMING_ADD_NODE_REWARD || task.action == XREWARD_CLAIMING_ADD_VOTER_DIVIDEND_REWARD) {
                    base::xstream_t stream_params{xcontext_t::instance(), (uint8_t *)task.params.data(), static_cast<uint32_t>(task.params.size())};
                    uint64_t onchain_timer_round;
                    std::map<std::string, top::xstake::uint128_t> rewards;
                    stream_params >> onchain_timer_round;
                    stream_params >> rewards;

                    for (auto v : rewards) {
                        jvn[v.first] = std::to_string(static_cast<uint64_t>(v.second / REWARD_PRECISION)) + std::string(".") +
                                       std::to_string(static_cast<uint32_t>(v.second % REWARD_PRECISION));
                    }

                    jv["rewards"] = jvn;
                } else if (task.action == XTRANSFER_ACTION) {
                    std::map<std::string, uint64_t> issuances;
                    base::xstream_t seo_stream(base::xcontext_t::instance(), (uint8_t *)task.params.c_str(), (uint32_t)task.params.size());
                    seo_stream >> issuances;
                    for (auto const & issue : issuances) {
                        jvn[issue.first] = std::to_string(issue.second);
                    }
                }
                deser_res += jv;
            }
        }
        stake_json["contract_account_parse"][addr][property_key] = deser_res;
    }
}

void xdb_reset_t::get_contract_table_stake_property_string(json & stake_json) {
    for (auto const & _pair : table_stake_string_pair_list) {
        std::string const addr = _pair.first;
        std::string const property_key = _pair.second;
        for (auto index = 0; index < enum_vledger_const::enum_vbucket_has_tables_count; ++index) {
            std::string table_addr = addr + "@" + std::to_string(index);
            json j;
            stake_json["contract_account_parse"][addr + "@" + std::to_string(index)][property_key] = j;
        }
    }
}

void xdb_reset_t::get_contract_table_stake_property_map_string_string(json & stake_json) {
    for (auto const & _pair : table_stake_map_string_string_pair_list) {
        std::string const addr = _pair.first;
        std::string const property_key = _pair.second;
        for (auto index = 0; index < enum_vledger_const::enum_vbucket_has_tables_count; ++index) {
            std::string table_addr = addr + "@" + std::to_string(index);
            base::xvaccount_t _vaddr(table_addr);
            auto _block = m_blockstore->load_block_object(_vaddr, 0, 0, false);
            if (_block == nullptr) {
                std::cout << table_addr << " height " << 0 << " block null!" << std::endl;
                continue;
            }
            if (_block->is_genesis_block() && _block->get_block_class() == base::enum_xvblock_class_nil) {
                std::cout << table_addr << " height " << 0 << " block genesis and nil!" << std::endl;
                continue;
            }
            base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(_block.get());
            if (bstate == nullptr) {
                std::cout << table_addr << " height " << 0 << " bstate null!" << std::endl;
                continue;
            }
            xaccount_ptr_t unitstate = std::make_shared<xunit_bstate_t>(bstate.get());
            if (unitstate == nullptr) {
                std::cout << table_addr << " height " << 0 << " unitstate null!" << std::endl;
                continue;
            }

            std::map<std::string, std::string> value;
            if (!unitstate->map_get(property_key, value)) {
                printf("[get_genesis_stage] contract_address: %s, property_name: %s, error", addr.c_str(), property_key.c_str());
                continue;
            }
            json j;
            if (property_key == XPORPERTY_CONTRACT_NODE_REWARD_KEY) {
                for (auto m : value) {
                    json jv;
                    xreward_node_record record;
                    auto detail = m.second;
                    base::xstream_t stream{xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
                    record.serialize_from(stream);
                    jv["accumulated"] = static_cast<uint64_t>(record.m_accumulated / REWARD_PRECISION);
                    jv["accumulated_decimals"] = static_cast<uint32_t>(record.m_accumulated % REWARD_PRECISION);
                    jv["unclaimed"] = static_cast<uint64_t>(record.m_unclaimed / REWARD_PRECISION);
                    jv["unclaimed_decimals"] = static_cast<uint32_t>(record.m_unclaimed % REWARD_PRECISION);
                    jv["last_claim_time"] = record.m_last_claim_time;
                    jv["issue_time"] = record.m_issue_time;
                    j[m.first] = jv;
                }
            } else if (property_key == XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY1 || property_key == XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY2 ||
                       property_key == XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY3 || property_key == XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY4) {
                for (auto m : value) {
                    xreward_record record;
                    auto detail = m.second;
                    base::xstream_t stream{xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
                    record.serialize_from(stream);
                    json jv;
                    jv["accumulated"] = static_cast<uint64_t>(record.accumulated / REWARD_PRECISION);
                    jv["accumulated_decimals"] = static_cast<uint32_t>(record.accumulated % REWARD_PRECISION);
                    jv["unclaimed"] = static_cast<uint64_t>(record.unclaimed / REWARD_PRECISION);
                    jv["unclaimed_decimals"] = static_cast<uint32_t>(record.unclaimed % REWARD_PRECISION);
                    jv["last_claim_time"] = record.last_claim_time;
                    jv["issue_time"] = record.issue_time;
                    json jvm;
                    int no = 0;
                    for (auto n : record.node_rewards) {
                        json jvn;
                        jvn["account_addr"] = n.account;
                        jvn["accumulated"] = static_cast<uint64_t>(n.accumulated / REWARD_PRECISION);
                        jvn["accumulated_decimals"] = static_cast<uint32_t>(n.accumulated % REWARD_PRECISION);
                        jvn["unclaimed"] = static_cast<uint64_t>(n.unclaimed / REWARD_PRECISION);
                        jvn["unclaimed_decimals"] = static_cast<uint32_t>(n.unclaimed % REWARD_PRECISION);
                        jvn["last_claim_time"] = n.last_claim_time;
                        jvn["issue_time"] = n.issue_time;
                        jvm[no++] = jvn;
                    }
                    jv["node_dividend"] = jvm;

                    j[m.first] = jv;
                }
            } else if (property_key == XPORPERTY_CONTRACT_POLLABLE_KEY) {
                for (auto m : value) {
                    j[m.first] = base::xstring_utl::touint64(m.second);
                }
            } else if (property_key == XPORPERTY_CONTRACT_VOTES_KEY1 || property_key == XPORPERTY_CONTRACT_VOTES_KEY2 || property_key == XPORPERTY_CONTRACT_VOTES_KEY3 ||
                       property_key == XPORPERTY_CONTRACT_VOTES_KEY4) {
                for (auto m : value) {
                    std::map<std::string, uint64_t> vote_info;
                    auto detail = m.second;
                    if (!detail.empty()) {
                        base::xstream_t stream{xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
                        stream >> vote_info;
                    }

                    json jv;
                    json jvn;
                    for (auto v : vote_info) {
                        jvn[v.first] = v.second;
                    }
                    jv["vote_infos"] = jvn;
                    j[m.first] = jv;
                }
            }
            stake_json["contract_account_parse"][addr + "@" + std::to_string(index)][property_key] = j;
        }
    }
}

NS_END2