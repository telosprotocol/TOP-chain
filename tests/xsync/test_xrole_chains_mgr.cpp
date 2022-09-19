#include "xdata/xblocktool.h"
#include "xdata/xgenesis_data.h"
#include "xmbus/xmessage_bus.h"

#include "xsync/xrole_chains_mgr.h"
#include "tests/xsync/common.h"

#include <gtest/gtest.h>

using namespace top;
using namespace top::sync;
using namespace top::data;
#if 0
#if 0
class validator_add_remove_archive_checker {
private:
    enum enum_role_state {
        enum_role_op_add_first,
        enum_role_op_add_second,
        enum_role_op_remove_second,
        enum_role_op_remove_first,
    };

private:
    xrole_chains_mgr_t * m_mgr;
    std::vector<std::uint16_t> m_validator_table_ids;
    std::vector<std::uint16_t> m_archive_table_ids;
    enum_role_state m_state{enum_role_op_add_first};

public:
    validator_add_remove_archive_checker(xrole_chains_mgr_t * mgr, std::vector<std::uint16_t> & validator_table_ids, std::vector<std::uint16_t> & archive_table_ids)
      : m_mgr(mgr), m_validator_table_ids(validator_table_ids), m_archive_table_ids(archive_table_ids) {}

    void add_validator() {
        ASSERT_EQ(m_state, enum_role_op_add_first);
        checker(*m_mgr, m_validator_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_add_second;
    }

    void add_archive() {
        ASSERT_EQ(m_state, enum_role_op_add_second);
        checker(*m_mgr, m_validator_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_remove_second;
    }

    void remove_archive() {
        ASSERT_EQ(m_state, enum_role_op_remove_second);
        checker(*m_mgr, m_validator_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_remove_first;
    }

    void remove_validator() {
        ASSERT_EQ(m_state, enum_role_op_remove_first);
        checker(*m_mgr, m_validator_table_ids, m_archive_table_ids, m_state);
    }

private:
    void checker(xrole_chains_mgr_t & mgr, const std::vector<std::uint16_t> & validator_table_ids, const std::vector<std::uint16_t> & archive_table_ids, enum_role_state state) {
        bool ret = false;

        map_chain_info_t tmp_chains = mgr.get_all_chains();
        map_chain_info_t chains = tmp_chains;

        // beacon table
        {
            std::string address = xdatautil::serialize_owner_str(sys_contract_beacon_table_block_addr, 0);

            if (state == enum_role_op_add_first || state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_24h);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_second) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);
            }
        }

        // zec table
        {
            std::string address = xdatautil::serialize_owner_str(sys_contract_zec_table_block_addr, 0);

            if (state == enum_role_op_add_first || state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_24h);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);
            } else if (state == enum_role_op_remove_second) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);
            }
        }

        // shard table
        for (auto table_id : validator_table_ids) {
            std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, table_id);

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_24h);
                ASSERT_EQ(it->second.active_policy, active_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_none);

                chains.erase(it);
            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_24h);
                ASSERT_EQ(it->second.active_policy, active_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_full);
                chains.erase(it);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        if (state == enum_role_op_add_second) {
            for (int i = (int)validator_table_ids.size(); i < (int)archive_table_ids.size(); i++) {
                std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, i);

                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_24h);
                ASSERT_EQ(it->second.active_policy, active_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);
            }
        }

        // sharding system contract
        std::vector<std::string> sharding_contract = {sys_contract_sharding_vote_addr};

        for (auto & contract : sharding_contract) {
            for (int j = 0; j < (int)validator_table_ids.size(); j++) {
                std::string address = xdatautil::serialize_owner_str(contract, j);

                if (state == enum_role_op_add_first || state == enum_role_op_remove_second) {
                    xchain_info_t info;
                    ret = mgr.get_chain(address, info);
                    ASSERT_EQ(ret, true);

                    auto it = chains.find(address);
                    ASSERT_NE(it, chains.end());
                    ASSERT_EQ(it->second, info);

                    ASSERT_EQ(it->second.address, address);
                    ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                    ASSERT_EQ(it->second.active_policy, latest_policy_none);
                    ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                    ASSERT_EQ(it->second.history_policy, history_policy_none);
                    chains.erase(address);

                } else if (state == enum_role_op_add_second) {
                    xchain_info_t info;
                    ret = mgr.get_chain(address, info);
                    ASSERT_EQ(ret, true);

                    auto it = chains.find(address);
                    ASSERT_NE(it, chains.end());
                    ASSERT_EQ(it->second, info);

                    ASSERT_EQ(it->second.address, address);
                    ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                    ASSERT_EQ(it->second.active_policy, latest_policy_none);
                    ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                    ASSERT_EQ(it->second.history_policy, history_policy_full);
                    chains.erase(address);
                } else if (state == enum_role_op_remove_first) {
                    ret = mgr.exists(address);
                    ASSERT_EQ(ret, false);

                    auto it = chains.find(address);
                    ASSERT_EQ(it, chains.end());
                }
            }
        }

        if (state == enum_role_op_add_second) {
            for (int i = 0; i < (int)sharding_contract.size(); i++) {
                const std::string & contract = sharding_contract[i];

                for (int j = (int)validator_table_ids.size(); j < (int)archive_table_ids.size(); j++) {
                    std::string address = xdatautil::serialize_owner_str(contract, j);

                    xchain_info_t info;
                    ret = mgr.get_chain(address, info);
                    ASSERT_EQ(ret, true);

                    auto it = chains.find(address);
                    ASSERT_NE(it, chains.end());
                    ASSERT_EQ(it->second, info);

                    ASSERT_EQ(it->second.address, address);
                    ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                    ASSERT_EQ(it->second.active_policy, latest_policy_none);
                    ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                    ASSERT_EQ(it->second.history_policy, history_policy_full);

                    chains.erase(address);
                }
            }
        }

        std::vector<std::string> elect_block_chain = {
            sys_contract_rec_elect_edge_addr, sys_contract_rec_elect_archive_addr, sys_contract_zec_elect_consensus_addr, sys_contract_zec_group_assoc_addr};

        for (auto pair : elect_block_chain) {
            const std::string & address = pair;

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        // root elect
        {
            const std::string & address = sys_contract_rec_elect_rec_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);
            }
        }

        // timer block
        {
            const std::string & address = sys_contract_beacon_timer_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_none);

                chains.erase(address);
            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);
            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        {
            const std::string & address = sys_contract_rec_elect_zec_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_none);

                chains.erase(address);

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        {
            const std::string & address = sys_contract_rec_standby_pool_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second || state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);
            }
        }

        // fixed system contract
        std::vector<std::string> fixed_accounts = {
            sys_contract_rec_tcc_addr,
            sys_contract_rec_registration_addr,
        };
        for (auto & address : fixed_accounts) {
            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        std::vector<std::string> beacon_contract = {
            sys_contract_zec_slash_info_addr,
        };
        for (int i = 0; i < (int)beacon_contract.size(); i++) {
            const std::string & address = beacon_contract[i];

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second || state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);
            }
        }

        {
            const std::string & address = sys_contract_zec_workload_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_none);

                chains.erase(address);

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);
            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        ASSERT_EQ(chains.size(), 0);
    }
};

TEST(xrole_chains_mgr, validator_add_remove_archive) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t validator_addr = create_validator_addr(1, "aa", 1);
    std::vector<std::uint16_t> validator_table_ids;
    for (uint32_t i = 0; i < 256; i++) {
        validator_table_ids.push_back(i);
    }

    vnetwork::xvnode_address_t archive_addr = create_archive_addr(1, "aa", 1);
    std::vector<std::uint16_t> archive_table_ids;
    for (uint32_t i = 0; i < 1024; i++) {
        archive_table_ids.push_back(i);
    }

    validator_add_remove_archive_checker checker(&mgr, validator_table_ids, archive_table_ids);

    {
        mgr.add_role(validator_addr, validator_table_ids);
        checker.add_validator();
    }

    {
        mgr.add_role(archive_addr, archive_table_ids);
        checker.add_archive();
    }

    {
        mgr.remove_role(archive_addr);
        checker.remove_archive();
    }

    {
        mgr.remove_role(validator_addr);
        checker.remove_validator();
    }
}

TEST(xrole_chains_mgr, auditor_add_remove_archive) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t auditor_addr = create_auditor_addr(1, "aa", 1);
    std::vector<std::uint16_t> auditor_table_ids;
    for (uint32_t i = 0; i < 256; i++) {
        auditor_table_ids.push_back(i);
    }

    vnetwork::xvnode_address_t archive_addr = create_archive_addr(1, "aa", 1);
    std::vector<std::uint16_t> archive_table_ids;
    for (uint32_t i = 0; i < 1024; i++) {
        archive_table_ids.push_back(i);
    }

    validator_add_remove_archive_checker checker(&mgr, auditor_table_ids, archive_table_ids);

    {
        mgr.add_role(auditor_addr, auditor_table_ids);
        checker.add_validator();
    }

    {
        mgr.add_role(archive_addr, archive_table_ids);
        checker.add_archive();
    }

    {
        mgr.remove_role(archive_addr);
        checker.remove_archive();
    }

    {
        mgr.remove_role(auditor_addr);
        checker.remove_validator();
    }
}

class beacon_add_remove_archive_checker {
private:
    enum enum_role_state {
        enum_role_op_add_first,
        enum_role_op_add_second,
        enum_role_op_remove_second,
        enum_role_op_remove_first,
    };

private:
    xrole_chains_mgr_t * m_mgr;
    std::vector<std::uint16_t> m_beacon_table_ids;
    std::vector<std::uint16_t> m_archive_table_ids;
    enum_role_state m_state{enum_role_op_add_first};

public:
    beacon_add_remove_archive_checker(xrole_chains_mgr_t * mgr, std::vector<std::uint16_t> & beacon_table_ids, std::vector<std::uint16_t> & archive_table_ids)
      : m_mgr(mgr), m_beacon_table_ids(beacon_table_ids), m_archive_table_ids(archive_table_ids) {}

    void add_validator() {
        ASSERT_EQ(m_state, enum_role_op_add_first);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_add_second;
    }

    void add_archive() {
        ASSERT_EQ(m_state, enum_role_op_add_second);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_remove_second;
    }

    void remove_archive() {
        ASSERT_EQ(m_state, enum_role_op_remove_second);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_remove_first;
    }

    void remove_validator() {
        ASSERT_EQ(m_state, enum_role_op_remove_first);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
    }

private:
    void checker(xrole_chains_mgr_t & mgr, const std::vector<std::uint16_t> & validator_table_ids, const std::vector<std::uint16_t> & archive_table_ids, enum_role_state state) {
        bool ret = false;

        map_chain_info_t tmp_chains = mgr.get_all_chains();
        map_chain_info_t chains = tmp_chains;

        // beacon table
        {
            std::string address = xdatautil::serialize_owner_str(sys_contract_beacon_table_block_addr, 0);

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_24h);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_none);

                chains.erase(address);

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_24h);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);
            }
        }

        // zec table
        {
            std::string address = xdatautil::serialize_owner_str(sys_contract_zec_table_block_addr, 0);

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second || state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_24h);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);
            }
        }

        // shard table
        for (auto table_id : validator_table_ids) {
            std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, table_id);

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second || state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_24h);
                ASSERT_EQ(it->second.active_policy, active_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_full);
                chains.erase(it);
            }
        }

        if (state == enum_role_op_add_second) {
            for (int i = (int)validator_table_ids.size(); i < (int)archive_table_ids.size(); i++) {
                std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, i);

                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_24h);
                ASSERT_EQ(it->second.active_policy, active_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);
            }
        }

        // sharding system contract
        std::vector<std::string> sharding_contract = {sys_contract_sharding_vote_addr};

        for (auto & contract : sharding_contract) {
            for (auto table_id : validator_table_ids) {
                std::string address = xdatautil::serialize_owner_str(contract, table_id);

                if (state == enum_role_op_add_first || state == enum_role_op_remove_second || state == enum_role_op_remove_first) {
                    ret = mgr.exists(address);
                    ASSERT_EQ(ret, false);

                    auto it = chains.find(address);
                    ASSERT_EQ(it, chains.end());

                } else if (state == enum_role_op_add_second) {
                    xchain_info_t info;
                    ret = mgr.get_chain(address, info);
                    ASSERT_EQ(ret, true);

                    auto it = chains.find(address);
                    ASSERT_NE(it, chains.end());
                    ASSERT_EQ(it->second, info);

                    ASSERT_EQ(it->second.address, address);
                    ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                    ASSERT_EQ(it->second.active_policy, active_policy_none);
                    ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                    ASSERT_EQ(it->second.history_policy, history_policy_full);
                    chains.erase(address);
                }
            }
        }

        if (state == enum_role_op_add_second) {
            for (auto & contract : sharding_contract) {
                for (int j = (int)validator_table_ids.size(); j < (int)archive_table_ids.size(); j++) {
                    std::string address = xdatautil::serialize_owner_str(contract, j);

                    xchain_info_t info;
                    ret = mgr.get_chain(address, info);
                    ASSERT_EQ(ret, true);

                    auto it = chains.find(address);
                    ASSERT_NE(it, chains.end());
                    ASSERT_EQ(it->second, info);

                    ASSERT_EQ(it->second.address, address);
                    ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                    ASSERT_EQ(it->second.active_policy, active_policy_none);
                    ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                    ASSERT_EQ(it->second.history_policy, history_policy_full);

                    chains.erase(address);
                }
            }
        }

        std::vector<std::string> elect_block_chain = {sys_contract_rec_elect_edge_addr,
                                                      sys_contract_rec_elect_archive_addr,
                                                      sys_contract_zec_elect_consensus_addr,
                                                      sys_contract_zec_group_assoc_addr};

        for (auto pair : elect_block_chain) {
            const std::string & address = pair;

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, active_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        // root elect
        {
            const std::string & address = sys_contract_rec_elect_rec_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);
            }
        }

        // timer block
        {
            const std::string & address = sys_contract_beacon_timer_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, active_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_none);

                chains.erase(address);
            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, active_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);
            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        {
            const std::string & address = sys_contract_rec_elect_zec_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, active_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        {
            const std::string & address = sys_contract_rec_standby_pool_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, active_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_none);

                chains.erase(address);
            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, active_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);
            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        // fixed system contract
        std::vector<std::string> fixed_accounts = {sys_contract_rec_tcc_addr, sys_contract_rec_registration_addr};
        for (auto & address : fixed_accounts) {
            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, active_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        std::vector<std::string> beacon_contract = {
            sys_contract_zec_workload_addr,
            sys_contract_zec_slash_info_addr,
        };
        for (auto & address : beacon_contract) {
            if (state == enum_role_op_add_first || state == enum_role_op_remove_second || state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);
            }
        }

        ASSERT_EQ(chains.size(), 0);
    }
};

TEST(xrole_chains_mgr, beacon_add_remove_archive) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t beacon_addr = create_beacon_addr(1, "aa", 1);
    std::vector<std::uint16_t> beacon_table_ids = {0};

    vnetwork::xvnode_address_t archive_addr = create_archive_addr(1, "aa", 1);
    std::vector<std::uint16_t> archive_table_ids;
    for (uint32_t i = 0; i < 1024; i++) {
        archive_table_ids.push_back(i);
    }

    beacon_add_remove_archive_checker checker(&mgr, beacon_table_ids, archive_table_ids);

    {
        mgr.add_role(beacon_addr, beacon_table_ids);
        checker.add_validator();
    }

    {
        mgr.add_role(archive_addr, archive_table_ids);
        checker.add_archive();
    }

    {
        mgr.remove_role(archive_addr);
        checker.remove_archive();
    }

    {
        mgr.remove_role(beacon_addr);
        checker.remove_validator();
    }
}

class zec_add_remove_archive_checker {
private:
    enum enum_role_state {
        enum_role_op_add_first,
        enum_role_op_add_second,
        enum_role_op_remove_second,
        enum_role_op_remove_first,
    };

private:
    xrole_chains_mgr_t * m_mgr;
    std::vector<std::uint16_t> m_beacon_table_ids;
    std::vector<std::uint16_t> m_archive_table_ids;
    enum_role_state m_state{enum_role_op_add_first};

public:
    zec_add_remove_archive_checker(xrole_chains_mgr_t * mgr, std::vector<std::uint16_t> & beacon_table_ids, std::vector<std::uint16_t> & archive_table_ids)
      : m_mgr(mgr), m_beacon_table_ids(beacon_table_ids), m_archive_table_ids(archive_table_ids) {}

    void add_validator() {
        ASSERT_EQ(m_state, enum_role_op_add_first);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_add_second;
    }

    void add_archive() {
        ASSERT_EQ(m_state, enum_role_op_add_second);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_remove_second;
    }

    void remove_archive() {
        ASSERT_EQ(m_state, enum_role_op_remove_second);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_remove_first;
    }

    void remove_validator() {
        ASSERT_EQ(m_state, enum_role_op_remove_first);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
    }

private:
    void checker(xrole_chains_mgr_t & mgr, const std::vector<std::uint16_t> & validator_table_ids, const std::vector<std::uint16_t> & archive_table_ids, enum_role_state state) {
        bool ret = false;

        map_chain_info_t tmp_chains = mgr.get_all_chains();
        map_chain_info_t chains = tmp_chains;

        // beacon table
        {
            std::string address = xdatautil::serialize_owner_str(sys_contract_beacon_table_block_addr, 0);

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second || state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_24h);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);
            }
        }

        // zec table
        {
            std::string address = xdatautil::serialize_owner_str(sys_contract_zec_table_block_addr, 0);

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_24h);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_none);

                chains.erase(address);

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_24h);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);
            }
        }

        // shard table
        for (auto table_id : validator_table_ids) {
            std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, table_id);

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second || state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_24h);
                ASSERT_EQ(it->second.active_policy, active_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_full);
                chains.erase(it);
            }
        }

        if (state == enum_role_op_add_second) {
            for (int i = (int)validator_table_ids.size(); i < (int)archive_table_ids.size(); i++) {
                std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, i);

                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_24h);
                ASSERT_EQ(it->second.active_policy, active_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);
            }
        }

        // sharding system contract
        std::vector<std::string> sharding_contract = {sys_contract_sharding_vote_addr};

        for (auto & contract : sharding_contract) {
            for (auto table_id : validator_table_ids) {
                std::string address = xdatautil::serialize_owner_str(contract, table_id);

                if (state == enum_role_op_add_first || state == enum_role_op_remove_second || state == enum_role_op_remove_first) {
                    ret = mgr.exists(address);
                    ASSERT_EQ(ret, false);

                    auto it = chains.find(address);
                    ASSERT_EQ(it, chains.end());

                } else if (state == enum_role_op_add_second) {
                    xchain_info_t info;
                    ret = mgr.get_chain(address, info);
                    ASSERT_EQ(ret, true);

                    auto it = chains.find(address);
                    ASSERT_NE(it, chains.end());
                    ASSERT_EQ(it->second, info);

                    ASSERT_EQ(it->second.address, address);
                    ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                    ASSERT_EQ(it->second.active_policy, latest_policy_none);
                    ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                    ASSERT_EQ(it->second.history_policy, history_policy_full);
                    chains.erase(address);
                }
            }
        }

        if (state == enum_role_op_add_second) {
            for (auto & contract : sharding_contract) {
                for (int j = (int)validator_table_ids.size(); j < (int)archive_table_ids.size(); j++) {
                    std::string address = xdatautil::serialize_owner_str(contract, j);

                    xchain_info_t info;
                    ret = mgr.get_chain(address, info);
                    ASSERT_EQ(ret, true);

                    auto it = chains.find(address);
                    ASSERT_NE(it, chains.end());
                    ASSERT_EQ(it->second, info);

                    ASSERT_EQ(it->second.address, address);
                    ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                    ASSERT_EQ(it->second.active_policy, latest_policy_none);
                    ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                    ASSERT_EQ(it->second.history_policy, history_policy_full);

                    chains.erase(address);
                }
            }
        }

        std::vector<std::string> elect_block_chain = {sys_contract_zec_group_assoc_addr};

        for (auto pair : elect_block_chain) {
            const std::string & address = pair;

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        // root elect
        {
            const std::string & address = sys_contract_rec_elect_rec_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);
            }
        }

        // timer block
        {
            const std::string & address = sys_contract_beacon_timer_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_none);

                chains.erase(address);
            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);
            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        {
            const std::string & address = sys_contract_rec_elect_zec_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        {
            const std::string & address = sys_contract_zec_elect_consensus_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);
            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        {
            const std::string & address = sys_contract_rec_elect_edge_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_second) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);
            }
        }

        {
            const std::string & address = sys_contract_rec_elect_archive_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_second) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);
            }
        }

        {
            const std::string & address = sys_contract_rec_standby_pool_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second || state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);
            }
        }

        // fixed system contract
        std::vector<std::string> fixed_accounts = {sys_contract_rec_tcc_addr,
                                                   sys_contract_rec_registration_addr};
        for (auto & address : fixed_accounts) {
            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        std::vector<std::string> beacon_contract = {
            sys_contract_zec_workload_addr,
            sys_contract_zec_slash_info_addr,
        };
        for (auto & address : beacon_contract) {
            if (state == enum_role_op_add_first || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_none);

                chains.erase(address);

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);
            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        ASSERT_EQ(chains.size(), 0);
    }
};

TEST(xrole_chains_mgr, zec_add_remove_archive) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t zec_addr = create_zec_addr(1, "aa", 1);
    std::vector<std::uint16_t> zec_table_ids = {0};

    vnetwork::xvnode_address_t archive_addr = create_archive_addr(1, "aa", 1);
    std::vector<std::uint16_t> archive_table_ids;
    for (uint32_t i = 0; i < 1024; i++) {
        archive_table_ids.push_back(i);
    }

    zec_add_remove_archive_checker checker(&mgr, zec_table_ids, archive_table_ids);

    {
        mgr.add_role(zec_addr, zec_table_ids);
        checker.add_validator();
    }

    {
        mgr.add_role(archive_addr, archive_table_ids);
        checker.add_archive();
    }

    {
        mgr.remove_role(archive_addr);
        checker.remove_archive();
    }

    {
        mgr.remove_role(zec_addr);
        checker.remove_validator();
    }
}

class beacon_add_remove_zec_checker {
private:
    enum enum_role_state {
        enum_role_op_add_first,
        enum_role_op_add_second,
        enum_role_op_remove_second,
        enum_role_op_remove_first,
    };

private:
    xrole_chains_mgr_t * m_mgr;
    std::vector<std::uint16_t> m_beacon_table_ids;
    std::vector<std::uint16_t> m_archive_table_ids;
    enum_role_state m_state{enum_role_op_add_first};

public:
    beacon_add_remove_zec_checker(xrole_chains_mgr_t * mgr, std::vector<std::uint16_t> & beacon_table_ids, std::vector<std::uint16_t> & archive_table_ids)
      : m_mgr(mgr), m_beacon_table_ids(beacon_table_ids), m_archive_table_ids(archive_table_ids) {}

    void add_validator() {
        ASSERT_EQ(m_state, enum_role_op_add_first);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_add_second;
    }

    void add_archive() {
        ASSERT_EQ(m_state, enum_role_op_add_second);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_remove_second;
    }

    void remove_archive() {
        ASSERT_EQ(m_state, enum_role_op_remove_second);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_remove_first;
    }

    void remove_validator() {
        ASSERT_EQ(m_state, enum_role_op_remove_first);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
    }

private:
    void checker(xrole_chains_mgr_t & mgr, const std::vector<std::uint16_t> & validator_table_ids, const std::vector<std::uint16_t> & archive_table_ids, enum_role_state state) {
        bool ret = false;

        map_chain_info_t tmp_chains = mgr.get_all_chains();
        map_chain_info_t chains = tmp_chains;

        // beacon table
        {
            std::string address = xdatautil::serialize_owner_str(sys_contract_beacon_table_block_addr, 0);

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_24h);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_none);

                chains.erase(address);
            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);
            }
        }

        // zec table
        {
            std::string address = xdatautil::serialize_owner_str(sys_contract_zec_table_block_addr, 0);

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second || state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_24h);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_none);

                chains.erase(address);
            }
        }

        std::vector<std::string> elect_block_chain = {sys_contract_rec_elect_zec_addr, sys_contract_zec_elect_consensus_addr, sys_contract_zec_group_assoc_addr};

        for (auto pair : elect_block_chain) {
            const std::string & address = pair;

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        // root elect
        {
            const std::string & address = sys_contract_rec_elect_rec_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);
            }
        }

        {
            const std::string & address = sys_contract_beacon_timer_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_none);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        {
            const std::string & address = sys_contract_rec_elect_edge_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        {
            const std::string & address = sys_contract_rec_elect_archive_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        {
            const std::string & address = sys_contract_rec_standby_pool_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_none);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        // fixed system contract
        std::vector<std::string> fixed_accounts = {sys_contract_rec_tcc_addr, sys_contract_rec_registration_addr};
        for (auto & address : fixed_accounts) {
            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        std::vector<std::string> beacon_contract = {
            sys_contract_zec_workload_addr,
            sys_contract_zec_slash_info_addr,
        };
        for (auto & address : beacon_contract) {
            if (state == enum_role_op_add_first || state == enum_role_op_remove_second || state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_none);

                chains.erase(address);
            }
        }

        ASSERT_EQ(chains.size(), 0);
    }
};

TEST(xrole_chains_mgr, beacon_add_remove_zec) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t beacon_addr = create_beacon_addr(1, "aa", 1);
    std::vector<std::uint16_t> beacon_table_ids = {0};

    vnetwork::xvnode_address_t zec_addr = create_zec_addr(1, "aa", 1);
    std::vector<std::uint16_t> zec_table_ids = {0};

    beacon_add_remove_zec_checker checker(&mgr, beacon_table_ids, zec_table_ids);

    {
        mgr.add_role(beacon_addr, beacon_table_ids);
        checker.add_validator();
    }

    {
        mgr.add_role(zec_addr, zec_table_ids);
        checker.add_archive();
    }

    {
        mgr.remove_role(zec_addr);
        checker.remove_archive();
    }

    {
        mgr.remove_role(beacon_addr);
        checker.remove_validator();
    }
}

class zec_add_remove_beacon_checker {
private:
    enum enum_role_state {
        enum_role_op_add_first,
        enum_role_op_add_second,
        enum_role_op_remove_second,
        enum_role_op_remove_first,
    };

private:
    xrole_chains_mgr_t * m_mgr;
    std::vector<std::uint16_t> m_beacon_table_ids;
    std::vector<std::uint16_t> m_archive_table_ids;
    enum_role_state m_state{enum_role_op_add_first};

public:
    zec_add_remove_beacon_checker(xrole_chains_mgr_t * mgr, std::vector<std::uint16_t> & beacon_table_ids, std::vector<std::uint16_t> & archive_table_ids)
      : m_mgr(mgr), m_beacon_table_ids(beacon_table_ids), m_archive_table_ids(archive_table_ids) {}

    void add_validator() {
        ASSERT_EQ(m_state, enum_role_op_add_first);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_add_second;
    }

    void add_archive() {
        ASSERT_EQ(m_state, enum_role_op_add_second);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_remove_second;
    }

    void remove_archive() {
        ASSERT_EQ(m_state, enum_role_op_remove_second);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_remove_first;
    }

    void remove_validator() {
        ASSERT_EQ(m_state, enum_role_op_remove_first);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
    }

private:
    void checker(xrole_chains_mgr_t & mgr, const std::vector<std::uint16_t> & validator_table_ids, const std::vector<std::uint16_t> & archive_table_ids, enum_role_state state) {
        bool ret = false;

        map_chain_info_t tmp_chains = mgr.get_all_chains();
        map_chain_info_t chains = tmp_chains;

        // beacon table
        {
            std::string address = xdatautil::serialize_owner_str(sys_contract_beacon_table_block_addr, 0);

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second || state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_24h);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_none);

                chains.erase(address);
            }
        }

        // zec table
        {
            std::string address = xdatautil::serialize_owner_str(sys_contract_zec_table_block_addr, 0);

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_24h);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_none);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        std::vector<std::string> elect_block_chain = {sys_contract_rec_elect_zec_addr, sys_contract_zec_group_assoc_addr};

        for (auto pair : elect_block_chain) {
            const std::string & address = pair;

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        // root elect
        {
            const std::string & address = sys_contract_rec_elect_rec_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);
            }
        }

        {
            const std::string & address = sys_contract_beacon_timer_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_none);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        {
            const std::string & address = sys_contract_rec_elect_edge_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second || state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);
            }
        }

        {
            const std::string & address = sys_contract_rec_elect_archive_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second || state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);
            }
        }

        {
            const std::string & address = sys_contract_zec_elect_consensus_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);
            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        {
            const std::string & address = sys_contract_rec_standby_pool_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second || state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_none);

                chains.erase(address);
            }
        }

        // fixed system contract
        std::vector<std::string> fixed_accounts = {sys_contract_rec_tcc_addr, sys_contract_rec_registration_addr};
        for (auto & address : fixed_accounts) {
            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        std::vector<std::string> beacon_contract = {
            sys_contract_zec_workload_addr,
            sys_contract_zec_slash_info_addr,
        };
        for (auto & address : beacon_contract) {
            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_none);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        ASSERT_EQ(chains.size(), 0);
    }
};

TEST(xrole_chains_mgr, zec_add_remove_beacon) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t zec_addr = create_zec_addr(1, "aa", 1);
    std::vector<std::uint16_t> zec_table_ids = {0};

    vnetwork::xvnode_address_t beacon_addr = create_beacon_addr(1, "aa", 1);
    std::vector<std::uint16_t> beacon_table_ids = {0};

    zec_add_remove_beacon_checker checker(&mgr, beacon_table_ids, zec_table_ids);

    {
        mgr.add_role(zec_addr, zec_table_ids);
        checker.add_validator();
    }

    {
        mgr.add_role(beacon_addr, beacon_table_ids);
        checker.add_archive();
    }

    {
        mgr.remove_role(beacon_addr);
        checker.remove_archive();
    }

    {
        mgr.remove_role(zec_addr);
        checker.remove_validator();
    }
}

class archive_add_remove_other_checker {
private:
    enum enum_role_state {
        enum_role_op_add_first,
        enum_role_op_add_second,
        enum_role_op_remove_second,
        enum_role_op_remove_first,
    };

private:
    xrole_chains_mgr_t * m_mgr;
    std::vector<std::uint16_t> m_archive_table_ids;
    enum_role_state m_state{enum_role_op_add_first};

public:
    archive_add_remove_other_checker(xrole_chains_mgr_t * mgr, std::vector<std::uint16_t> & archive_table_ids) : m_mgr(mgr), m_archive_table_ids(archive_table_ids) {}

    void add_archive() {
        ASSERT_EQ(m_state, enum_role_op_add_first);
        checker(*m_mgr, m_archive_table_ids, m_state);
        m_state = enum_role_op_add_second;
    }

    void add_other() {
        ASSERT_EQ(m_state, enum_role_op_add_second);
        checker(*m_mgr, m_archive_table_ids, m_state);
        m_state = enum_role_op_remove_second;
    }

    void remove_other() {
        ASSERT_EQ(m_state, enum_role_op_remove_second);
        checker(*m_mgr, m_archive_table_ids, m_state);
        m_state = enum_role_op_remove_first;
    }

    void remove_archive() {
        ASSERT_EQ(m_state, enum_role_op_remove_first);
        checker(*m_mgr, m_archive_table_ids, m_state);
    }

private:
    void checker(xrole_chains_mgr_t & mgr, const std::vector<std::uint16_t> & archive_table_ids, enum_role_state state) {
        bool ret = false;

        map_chain_info_t tmp_chains = mgr.get_all_chains();
        map_chain_info_t chains = tmp_chains;

        // beacon table
        {
            std::string address = xdatautil::serialize_owner_str(sys_contract_beacon_table_block_addr, 0);

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_24h);
                ASSERT_EQ(it->second.active_policy, active_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        // zec table
        {
            std::string address = xdatautil::serialize_owner_str(sys_contract_zec_table_block_addr, 0);

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_24h);
                ASSERT_EQ(it->second.active_policy, active_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        // shard table
        for (auto & table_id : archive_table_ids) {
            std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, table_id);

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_24h);
                ASSERT_EQ(it->second.active_policy, active_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        // sharding system contract
        std::vector<std::string> sharding_contract = {sys_contract_sharding_vote_addr};

        for (auto & contract : sharding_contract) {
            for (auto table_id : archive_table_ids) {
                std::string address = xdatautil::serialize_owner_str(contract, table_id);

                if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                    xchain_info_t info;
                    ret = mgr.get_chain(address, info);
                    ASSERT_EQ(ret, true);

                    auto it = chains.find(address);
                    ASSERT_NE(it, chains.end());
                    ASSERT_EQ(it->second, info);

                    ASSERT_EQ(it->second.address, address);
                    ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                    ASSERT_EQ(it->second.active_policy, latest_policy_none);
                    ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                    ASSERT_EQ(it->second.history_policy, history_policy_full);

                    chains.erase(address);

                } else if (state == enum_role_op_remove_first) {
                    ret = mgr.exists(address);
                    ASSERT_EQ(ret, false);

                    auto it = chains.find(address);
                    ASSERT_EQ(it, chains.end());
                }
            }
        }

        std::vector<std::string> elect_block_chain = {sys_contract_rec_elect_edge_addr,
                                                      sys_contract_rec_elect_archive_addr,
                                                      sys_contract_beacon_timer_addr,
                                                      sys_contract_rec_elect_zec_addr,
                                                      sys_contract_zec_elect_consensus_addr,
                                                      sys_contract_rec_standby_pool_addr,
                                                      sys_contract_zec_group_assoc_addr};

        for (auto pair : elect_block_chain) {
            const std::string & address = pair;

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        // root elect
        {
            const std::string & address = sys_contract_rec_elect_rec_addr;

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);
            }
        }

        // fixed system contract
        std::vector<std::string> fixed_accounts = {sys_contract_rec_tcc_addr, sys_contract_rec_registration_addr};
        for (auto & address : fixed_accounts) {
            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        std::vector<std::string> beacon_contract = {
            sys_contract_zec_workload_addr,
            sys_contract_zec_slash_info_addr,
        };
        for (auto & address : beacon_contract) {
            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_full);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        ASSERT_EQ(chains.size(), 0);
    }
};

TEST(xrole_chains_mgr, archive_add_remove_beacon) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t archive_addr = create_archive_addr(1, "aa", 1);
    std::vector<std::uint16_t> archive_table_ids;
    for (int i = 0; i < 1024; i++)
        archive_table_ids.push_back(i);

    vnetwork::xvnode_address_t beacon_addr = create_beacon_addr(1, "aa", 1);
    std::vector<std::uint16_t> beacon_table_ids = {0};

    std::shared_ptr<xrole_chains_t> beacon_chains = std::make_shared<xrole_chains_t>(archive_addr, archive_table_ids);
    const map_chain_info_t & chains1 = beacon_chains->get_chains_wrapper().get_chains();

    archive_add_remove_other_checker checker(&mgr, archive_table_ids);

    {
        mgr.add_role(archive_addr, archive_table_ids);
        checker.add_archive();
    }

    {
        mgr.add_role(beacon_addr, beacon_table_ids);
        checker.add_other();
    }

    {
        mgr.remove_role(beacon_addr);
        checker.remove_other();
    }

    {
        mgr.remove_role(archive_addr);
        checker.remove_archive();
    }
}

TEST(xrole_chains_mgr, archive_add_remove_zec) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t archive_addr = create_archive_addr(1, "aa", 1);
    std::vector<std::uint16_t> archive_table_ids;
    for (int i = 0; i < 1024; i++)
        archive_table_ids.push_back(i);

    vnetwork::xvnode_address_t zec_addr = create_zec_addr(1, "aa", 1);
    std::vector<std::uint16_t> zec_table_ids = {0};

    std::shared_ptr<xrole_chains_t> beacon_chains = std::make_shared<xrole_chains_t>(archive_addr, archive_table_ids);
    const map_chain_info_t & chains1 = beacon_chains->get_chains_wrapper().get_chains();

    archive_add_remove_other_checker checker(&mgr, archive_table_ids);

    {
        mgr.add_role(archive_addr, archive_table_ids);
        checker.add_archive();
    }

    {
        mgr.add_role(zec_addr, zec_table_ids);
        checker.add_other();
    }

    {
        mgr.remove_role(zec_addr);
        checker.remove_other();
    }

    {
        mgr.remove_role(archive_addr);
        checker.remove_archive();
    }
}

TEST(xrole_chains_mgr, archive_add_remove_auditor) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t archive_addr = create_archive_addr(1, "aa", 1);
    std::vector<std::uint16_t> archive_table_ids;
    for (int i = 0; i < 1024; i++)
        archive_table_ids.push_back(i);

    vnetwork::xvnode_address_t auditor_addr = create_auditor_addr(1, "aa", 1);
    std::vector<std::uint16_t> auditor_table_ids;
    for (int i = 0; i < 256; i++)
        auditor_table_ids.push_back(i);

    std::shared_ptr<xrole_chains_t> beacon_chains = std::make_shared<xrole_chains_t>(archive_addr, archive_table_ids);
    const map_chain_info_t & chains1 = beacon_chains->get_chains_wrapper().get_chains();

    archive_add_remove_other_checker checker(&mgr, archive_table_ids);

    {
        mgr.add_role(archive_addr, archive_table_ids);
        checker.add_archive();
    }

    {
        mgr.add_role(auditor_addr, auditor_table_ids);
        checker.add_other();
    }

    {
        mgr.remove_role(auditor_addr);
        checker.remove_other();
    }

    {
        mgr.remove_role(archive_addr);
        checker.remove_archive();
    }
}

TEST(xrole_chains_mgr, archive_add_remove_validator) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t archive_addr = create_archive_addr(1, "aa", 1);
    std::vector<std::uint16_t> archive_table_ids;
    for (int i = 0; i < 1024; i++)
        archive_table_ids.push_back(i);

    vnetwork::xvnode_address_t validator_addr = create_validator_addr(1, "aa", 1);
    std::vector<std::uint16_t> validator_table_ids;
    for (int i = 0; i < 256; i++)
        validator_table_ids.push_back(i);

    archive_add_remove_other_checker checker(&mgr, archive_table_ids);

    {
        mgr.add_role(archive_addr, archive_table_ids);
        checker.add_archive();
    }

    {
        mgr.add_role(validator_addr, validator_table_ids);
        checker.add_other();
    }

    {
        mgr.remove_role(validator_addr);
        checker.remove_other();
    }

    {
        mgr.remove_role(archive_addr);
        checker.remove_archive();
    }
}

TEST(xrole_chains_mgr, archive_add_remove_multi_role) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t archive_addr = create_archive_addr(1, "aa", 1);
    std::vector<std::uint16_t> archive_table_ids;
    for (int i = 0; i < 1024; i++)
        archive_table_ids.push_back(i);

    vnetwork::xvnode_address_t auditor_addr1 = create_auditor_addr(1, "aa", 1);
    std::vector<std::uint16_t> auditor_table_ids;
    for (int i = 0; i < 256; i++)
        auditor_table_ids.push_back(i);

    vnetwork::xvnode_address_t auditor_addr2 = create_auditor_addr(1, "aa", 2);
    vnetwork::xvnode_address_t auditor_addr3 = create_auditor_addr(1, "aa", 3);
    vnetwork::xvnode_address_t auditor_addr4 = create_auditor_addr(1, "aa", 4);
    vnetwork::xvnode_address_t auditor_addr7 = create_auditor_addr(1, "aa", 7);

    mgr.add_role(archive_addr, archive_table_ids);
    mgr.add_role(auditor_addr1, auditor_table_ids);
    mgr.add_role(auditor_addr2, auditor_table_ids);
    mgr.add_role(auditor_addr3, auditor_table_ids);
    mgr.remove_role(auditor_addr1);
    mgr.add_role(auditor_addr4, auditor_table_ids);
    mgr.remove_role(auditor_addr2);
    mgr.add_role(auditor_addr7, auditor_table_ids);

    std::shared_ptr<xrole_chains_t> archive_chains = std::make_shared<xrole_chains_t>(archive_addr, archive_table_ids);
    const map_chain_info_t & chains1 = archive_chains->get_chains_wrapper().get_chains();

    std::string address = sys_contract_rec_elect_rec_addr;

    xchain_info_t info;
    bool ret = mgr.get_chain(address, info);
    ASSERT_EQ(ret, true);

    ASSERT_EQ(info.address, address);
    ASSERT_EQ(info.latest_policy, latest_policy_none);
    ASSERT_EQ(info.active_policy, latest_policy_none);
    ASSERT_EQ(info.anchor_policy, latest_policy_none);
    ASSERT_EQ(info.history_policy, history_policy_full);
}

class validator_update_checker {
private:
    xrole_chains_mgr_t * m_mgr;
    std::vector<std::uint16_t> m_table_ids;

public:
    validator_update_checker(xrole_chains_mgr_t * mgr, std::vector<std::uint16_t> & table_ids) : m_mgr(mgr), m_table_ids(table_ids) {}

    void add() { checker(*m_mgr, m_table_ids); }

private:
    void checker(xrole_chains_mgr_t & mgr, const std::vector<std::uint16_t> & table_ids) {
        bool ret = false;

        map_chain_info_t tmp_chains = mgr.get_all_chains();
        map_chain_info_t chains = tmp_chains;

        // shard table
        for (auto table_id : table_ids) {
            std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, table_id);

            auto it = chains.find(address);
            ASSERT_NE(it, chains.end());

            ASSERT_EQ(it->second.address, address);
            ASSERT_EQ(it->second.latest_policy, latest_policy_24h);
            ASSERT_EQ(it->second.active_policy, active_policy_none);
            ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
            ASSERT_EQ(it->second.history_policy, history_policy_none);
            chains.erase(it);
        }

        std::vector<std::string> fixed_accounts = {sys_contract_rec_tcc_addr, sys_contract_rec_registration_addr};

        for (auto & address : fixed_accounts) {
            auto it = chains.find(address);
            ASSERT_NE(it, chains.end());

            ASSERT_EQ(it->second.address, address);
            ASSERT_EQ(it->second.latest_policy, latest_policy_none);
            ASSERT_EQ(it->second.active_policy, latest_policy_none);
            ASSERT_EQ(it->second.anchor_policy, anchor_policy);
            ASSERT_EQ(it->second.history_policy, history_policy_full);
            chains.erase(it);
        }

        // sharding system contract
        std::vector<std::string> sharding_contract = {sys_contract_sharding_vote_addr};

        for (auto & contract : sharding_contract) {
            for (auto table_id : table_ids) {
                std::string address = xdatautil::serialize_owner_str(contract, table_id);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());

                ASSERT_EQ(it->second.address, address);
                ASSERT_EQ(it->second.latest_policy, latest_policy_none);
                ASSERT_EQ(it->second.active_policy, latest_policy_none);
                ASSERT_EQ(it->second.anchor_policy, anchor_policy);
                ASSERT_EQ(it->second.history_policy, history_policy_none);
                chains.erase(it);
            }
        }

        std::vector<std::string> elect_block_chain = {
            sys_contract_rec_elect_edge_addr, sys_contract_rec_elect_archive_addr, sys_contract_zec_elect_consensus_addr, sys_contract_zec_group_assoc_addr};

        for (auto pair : elect_block_chain) {
            const std::string & address = pair;

            auto it = chains.find(address);
            ASSERT_NE(it, chains.end());

            ASSERT_EQ(it->second.address, address);
            ASSERT_EQ(it->second.latest_policy, latest_policy_none);
            ASSERT_EQ(it->second.active_policy, latest_policy_none);
            ASSERT_EQ(it->second.anchor_policy, anchor_policy);
            ASSERT_EQ(it->second.history_policy, history_policy_full);
            chains.erase(it);
        }

        {
            const std::string & address = sys_contract_rec_elect_rec_addr;

            auto it = chains.find(address);
            ASSERT_NE(it, chains.end());

            ASSERT_EQ(it->second.address, address);
            ASSERT_EQ(it->second.latest_policy, latest_policy_none);
            ASSERT_EQ(it->second.active_policy, latest_policy_none);
            ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
            ASSERT_EQ(it->second.history_policy, history_policy_full);
            chains.erase(it);
        }

        std::vector<std::string> beacon_contract = {
            sys_contract_beacon_timer_addr,
            sys_contract_rec_elect_zec_addr,
            sys_contract_zec_workload_addr,
        };

        for (const auto & pair : beacon_contract) {
            const std::string & address = pair;

            auto it = chains.find(address);
            ASSERT_NE(it, chains.end());

            ASSERT_EQ(it->second.address, address);
            ASSERT_EQ(it->second.latest_policy, latest_policy_none);
            ASSERT_EQ(it->second.active_policy, latest_policy_none);
            ASSERT_EQ(it->second.anchor_policy, anchor_policy);
            ASSERT_EQ(it->second.history_policy, history_policy_none);
            chains.erase(it);
        }

        ASSERT_EQ(chains.size(), 0);
    }
};

TEST(xrole_chains_mgr, add_higher_version) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t validator_addr1 = create_validator_addr(1, "aa", 1);
    vnetwork::xvnode_address_t validator_addr2 = create_validator_addr(1, "aa", 2);
    std::vector<std::uint16_t> validator_table_ids;
    for (uint32_t i = 0; i < 256; i++) {
        validator_table_ids.push_back(i);
    }

    validator_update_checker checker(&mgr, validator_table_ids);

    {
        mgr.add_role(validator_addr1, validator_table_ids);
        checker.add();
    }

    const xsync_roles_t & roles1 = mgr.get_roles();
    ASSERT_EQ(roles1.size(), 1);
    auto it1 = roles1.find(validator_addr1);
    ASSERT_NE(it1, roles1.end());

    {
        mgr.add_role(validator_addr2, validator_table_ids);
        checker.add();
    }

    const xsync_roles_t & roles2 = mgr.get_roles();
    ASSERT_EQ(roles2.size(), 1);
    auto it2 = roles2.find(validator_addr2);
    ASSERT_NE(it2, roles2.end());
}

TEST(xrole_chains_mgr, remove_lower_version) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t validator_addr2 = create_validator_addr(1, "aa", 2);
    std::vector<std::uint16_t> validator_table_ids;
    for (uint32_t i = 0; i < 256; i++) {
        validator_table_ids.push_back(i);
    }

    vnetwork::xvnode_address_t validator_addr1 = create_validator_addr(1, "aa", 1);

    validator_update_checker checker(&mgr, validator_table_ids);

    {
        mgr.add_role(validator_addr2, validator_table_ids);
        checker.add();
    }

    const xsync_roles_t & roles1 = mgr.get_roles();
    ASSERT_EQ(roles1.size(), 1);
    auto it1 = roles1.find(validator_addr2);
    ASSERT_NE(it1, roles1.end());

    {
        mgr.remove_role(validator_addr1);
        checker.add();
    }

    const xsync_roles_t & roles2 = mgr.get_roles();
    ASSERT_EQ(roles2.size(), 1);
    auto it2 = roles2.find(validator_addr2);
    ASSERT_NE(it2, roles2.end());
}

TEST(xrole_chains_mgr, remove_empty_role) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t validator_addr = create_validator_addr(1, "aa", 1);
    std::vector<std::uint16_t> validator_table_ids;
    for (uint32_t i = 0; i < 256; i++) {
        validator_table_ids.push_back(i);
    }

    vnetwork::xvnode_address_t archive_addr = create_archive_addr(1, "aa", 1);

    validator_update_checker checker(&mgr, validator_table_ids);

    {
        mgr.add_role(validator_addr, validator_table_ids);
        checker.add();
    }

    const xsync_roles_t & roles1 = mgr.get_roles();
    ASSERT_EQ(roles1.size(), 1);
    auto it1 = roles1.find(validator_addr);
    ASSERT_NE(it1, roles1.end());

    {
        mgr.remove_role(archive_addr);
        checker.add();
    }

    const xsync_roles_t & roles2 = mgr.get_roles();
    ASSERT_EQ(roles2.size(), 1);
    auto it2 = roles2.find(validator_addr);
    ASSERT_NE(it2, roles2.end());
}
#endif


class validator_add_remove_archive_checker {
private:
    enum enum_role_state {
        enum_role_op_add_first,
        enum_role_op_add_second,
        enum_role_op_remove_second,
        enum_role_op_remove_first,
    };

private:
    xrole_chains_mgr_t * m_mgr;
    std::vector<std::uint16_t> m_validator_table_ids;
    std::vector<std::uint16_t> m_archive_table_ids;
    enum_role_state m_state{enum_role_op_add_first};

public:
    validator_add_remove_archive_checker(xrole_chains_mgr_t * mgr, std::vector<std::uint16_t> & validator_table_ids, std::vector<std::uint16_t> & archive_table_ids)
      : m_mgr(mgr), m_validator_table_ids(validator_table_ids), m_archive_table_ids(archive_table_ids) {}

    void add_validator() {
        ASSERT_EQ(m_state, enum_role_op_add_first);
        checker(*m_mgr, m_validator_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_add_second;
    }

    void add_archive() {
        ASSERT_EQ(m_state, enum_role_op_add_second);
        checker(*m_mgr, m_validator_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_remove_second;
    }

    void remove_archive() {
        ASSERT_EQ(m_state, enum_role_op_remove_second);
        checker(*m_mgr, m_validator_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_remove_first;
    }

    void remove_validator() {
        ASSERT_EQ(m_state, enum_role_op_remove_first);
        checker(*m_mgr, m_validator_table_ids, m_archive_table_ids, m_state);
    }

private:
    void checker(xrole_chains_mgr_t & mgr, const std::vector<std::uint16_t> & validator_table_ids, const std::vector<std::uint16_t> & archive_table_ids, enum_role_state state) {
        bool ret = false;

        map_chain_info_t tmp_chains = mgr.get_all_chains();
        map_chain_info_t chains = tmp_chains;

        // shard table
        for (auto table_id : validator_table_ids) {
            std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, table_id);

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                chains.erase(it);

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                chains.erase(it);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        if (state == enum_role_op_add_second) {
            for (int i = (int)validator_table_ids.size(); i < (int)archive_table_ids.size(); i++) {
                std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, i);

                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                chains.erase(address);
            }
        }

        ASSERT_EQ(chains.size(), 0);
    }
};

TEST(xrole_chains_mgr, validator_add_remove_archive) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t validator_addr = create_validator_addr(1, "aa", 1);
    std::vector<std::uint16_t> validator_table_ids;
    for (uint32_t i = 0; i < 256; i++) {
        validator_table_ids.push_back(i);
    }

    vnetwork::xvnode_address_t archive_addr = create_archive_addr(1, "aa", 1);
    std::vector<std::uint16_t> archive_table_ids;
    for (uint32_t i = 0; i < 1024; i++) {
        archive_table_ids.push_back(i);
    }

    validator_add_remove_archive_checker checker(&mgr, validator_table_ids, archive_table_ids);
    std::shared_ptr<xrole_chains_t> archive_chains = std::make_shared<xrole_chains_t>(archive_addr, archive_table_ids);
    std::shared_ptr<xrole_chains_t> validator_chains = std::make_shared<xrole_chains_t>(validator_addr, validator_table_ids);

    {
        mgr.add_role(validator_chains);
        checker.add_validator();
    }

    {
        mgr.add_role(archive_chains);
        checker.add_archive();
    }

    {
        mgr.remove_role(archive_chains);
        checker.remove_archive();
    }

    {
        mgr.remove_role(validator_chains);
        checker.remove_validator();
    }
}

TEST(xrole_chains_mgr, auditor_add_remove_archive) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t auditor_addr = create_auditor_addr(1, "aa", 1);
    std::vector<std::uint16_t> auditor_table_ids;
    for (uint32_t i = 0; i < 256; i++) {
        auditor_table_ids.push_back(i);
    }

    vnetwork::xvnode_address_t archive_addr = create_archive_addr(1, "aa", 1);
    std::vector<std::uint16_t> archive_table_ids;
    for (uint32_t i = 0; i < 1024; i++) {
        archive_table_ids.push_back(i);
    }

    validator_add_remove_archive_checker checker(&mgr, auditor_table_ids, archive_table_ids);
    std::shared_ptr<xrole_chains_t> archive_chains = std::make_shared<xrole_chains_t>(archive_addr, archive_table_ids);
    std::shared_ptr<xrole_chains_t> auditor_chains = std::make_shared<xrole_chains_t>(auditor_addr, auditor_table_ids);

    {
        mgr.add_role(auditor_chains);
        checker.add_validator();
    }

    {
        mgr.add_role(archive_chains);
        checker.add_archive();
    }

    {
        mgr.remove_role(archive_chains);
        checker.remove_archive();
    }

    {
        mgr.remove_role(auditor_chains);
        checker.remove_validator();
    }
}

class beacon_add_remove_archive_checker {
private:
    enum enum_role_state {
        enum_role_op_add_first,
        enum_role_op_add_second,
        enum_role_op_remove_second,
        enum_role_op_remove_first,
    };

private:
    xrole_chains_mgr_t * m_mgr;
    std::vector<std::uint16_t> m_beacon_table_ids;
    std::vector<std::uint16_t> m_archive_table_ids;
    enum_role_state m_state{enum_role_op_add_first};

public:
    beacon_add_remove_archive_checker(xrole_chains_mgr_t * mgr, std::vector<std::uint16_t> & beacon_table_ids, std::vector<std::uint16_t> & archive_table_ids)
      : m_mgr(mgr), m_beacon_table_ids(beacon_table_ids), m_archive_table_ids(archive_table_ids) {}

    void add_validator() {
        ASSERT_EQ(m_state, enum_role_op_add_first);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_add_second;
    }

    void add_archive() {
        ASSERT_EQ(m_state, enum_role_op_add_second);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_remove_second;
    }

    void remove_archive() {
        ASSERT_EQ(m_state, enum_role_op_remove_second);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_remove_first;
    }

    void remove_validator() {
        ASSERT_EQ(m_state, enum_role_op_remove_first);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
    }

private:
    void checker(xrole_chains_mgr_t & mgr, const std::vector<std::uint16_t> & validator_table_ids, const std::vector<std::uint16_t> & archive_table_ids, enum_role_state state) {
        bool ret = false;

        map_chain_info_t tmp_chains = mgr.get_all_chains();
        map_chain_info_t chains = tmp_chains;

        // beacon table
        {
            std::string address = xdatautil::serialize_owner_str(sys_contract_beacon_table_block_addr, 0);

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                chains.erase(address);

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);
            }
        }

        // shard table
        for (auto table_id : validator_table_ids) {
            std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, table_id);

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second || state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                chains.erase(it);
            }
        }

        if (state == enum_role_op_add_second) {
            for (int i = (int)validator_table_ids.size(); i < (int)archive_table_ids.size(); i++) {
                std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, i);

                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                chains.erase(address);
            }
        }

        ASSERT_EQ(chains.size(), 0);
    }
};

TEST(xrole_chains_mgr, beacon_add_remove_archive) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t beacon_addr = create_beacon_addr(1, "aa", 1);
    std::vector<std::uint16_t> beacon_table_ids = {0};

    vnetwork::xvnode_address_t archive_addr = create_archive_addr(1, "aa", 1);
    std::vector<std::uint16_t> archive_table_ids;
    for (uint32_t i = 0; i < 1024; i++) {
        archive_table_ids.push_back(i);
    }

    beacon_add_remove_archive_checker checker(&mgr, beacon_table_ids, archive_table_ids);

    std::shared_ptr<xrole_chains_t> archive_chains = std::make_shared<xrole_chains_t>(archive_addr, archive_table_ids);
    std::shared_ptr<xrole_chains_t> beacon_chains = std::make_shared<xrole_chains_t>(beacon_addr, beacon_table_ids);

    {
        mgr.add_role(beacon_chains);
        checker.add_validator();
    }

    {
        mgr.add_role(archive_chains);
        checker.add_archive();
    }

    {
        mgr.remove_role(archive_chains);
        checker.remove_archive();
    }

    {
        mgr.remove_role(beacon_chains);
        checker.remove_validator();
    }
}

class zec_add_remove_archive_checker {
private:
    enum enum_role_state {
        enum_role_op_add_first,
        enum_role_op_add_second,
        enum_role_op_remove_second,
        enum_role_op_remove_first,
    };

private:
    xrole_chains_mgr_t * m_mgr;
    std::vector<std::uint16_t> m_beacon_table_ids;
    std::vector<std::uint16_t> m_archive_table_ids;
    enum_role_state m_state{enum_role_op_add_first};

public:
    zec_add_remove_archive_checker(xrole_chains_mgr_t * mgr, std::vector<std::uint16_t> & beacon_table_ids, std::vector<std::uint16_t> & archive_table_ids)
      : m_mgr(mgr), m_beacon_table_ids(beacon_table_ids), m_archive_table_ids(archive_table_ids) {}

    void add_validator() {
        ASSERT_EQ(m_state, enum_role_op_add_first);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_add_second;
    }

    void add_archive() {
        ASSERT_EQ(m_state, enum_role_op_add_second);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_remove_second;
    }

    void remove_archive() {
        ASSERT_EQ(m_state, enum_role_op_remove_second);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_remove_first;
    }

    void remove_validator() {
        ASSERT_EQ(m_state, enum_role_op_remove_first);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
    }

private:
    void checker(xrole_chains_mgr_t & mgr, const std::vector<std::uint16_t> & validator_table_ids, const std::vector<std::uint16_t> & archive_table_ids, enum_role_state state) {
        bool ret = false;

        map_chain_info_t tmp_chains = mgr.get_all_chains();
        map_chain_info_t chains = tmp_chains;

        // zec table
        {
            std::string address = xdatautil::serialize_owner_str(sys_contract_zec_table_block_addr, 0);

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                chains.erase(address);

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);
            }
        }

        // shard table
        for (auto table_id : validator_table_ids) {
            std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, table_id);

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second || state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                chains.erase(it);
            }
        }

        if (state == enum_role_op_add_second) {
            for (int i = (int)validator_table_ids.size(); i < (int)archive_table_ids.size(); i++) {
                std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, i);

                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                chains.erase(address);
            }
        }

        ASSERT_EQ(chains.size(), 0);
    }
};

TEST(xrole_chains_mgr, zec_add_remove_archive) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t zec_addr = create_zec_addr(1, "aa", 1);
    std::vector<std::uint16_t> zec_table_ids = {0};

    vnetwork::xvnode_address_t archive_addr = create_archive_addr(1, "aa", 1);
    std::vector<std::uint16_t> archive_table_ids;
    for (uint32_t i = 0; i < 1024; i++) {
        archive_table_ids.push_back(i);
    }

    zec_add_remove_archive_checker checker(&mgr, zec_table_ids, archive_table_ids);
    std::shared_ptr<xrole_chains_t> archive_chains = std::make_shared<xrole_chains_t>(archive_addr, archive_table_ids);
    std::shared_ptr<xrole_chains_t> zec_chains = std::make_shared<xrole_chains_t>(zec_addr, zec_table_ids);

    {
        mgr.add_role(zec_chains);
        checker.add_validator();
    }

    {
        mgr.add_role(archive_chains);
        checker.add_archive();
    }

    {
        mgr.remove_role(archive_chains);
        checker.remove_archive();
    }

    {
        mgr.remove_role(zec_chains);
        checker.remove_validator();
    }
}

class beacon_add_remove_zec_checker {
private:
    enum enum_role_state {
        enum_role_op_add_first,
        enum_role_op_add_second,
        enum_role_op_remove_second,
        enum_role_op_remove_first,
    };

private:
    xrole_chains_mgr_t * m_mgr;
    std::vector<std::uint16_t> m_beacon_table_ids;
    std::vector<std::uint16_t> m_archive_table_ids;
    enum_role_state m_state{enum_role_op_add_first};

public:
    beacon_add_remove_zec_checker(xrole_chains_mgr_t * mgr, std::vector<std::uint16_t> & beacon_table_ids, std::vector<std::uint16_t> & archive_table_ids)
      : m_mgr(mgr), m_beacon_table_ids(beacon_table_ids), m_archive_table_ids(archive_table_ids) {}

    void add_validator() {
        ASSERT_EQ(m_state, enum_role_op_add_first);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_add_second;
    }

    void add_archive() {
        ASSERT_EQ(m_state, enum_role_op_add_second);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_remove_second;
    }

    void remove_archive() {
        ASSERT_EQ(m_state, enum_role_op_remove_second);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_remove_first;
    }

    void remove_validator() {
        ASSERT_EQ(m_state, enum_role_op_remove_first);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
    }

private:
    void checker(xrole_chains_mgr_t & mgr, const std::vector<std::uint16_t> & validator_table_ids, const std::vector<std::uint16_t> & archive_table_ids, enum_role_state state) {
        bool ret = false;

        map_chain_info_t tmp_chains = mgr.get_all_chains();
        map_chain_info_t chains = tmp_chains;

        // beacon table
        {
            std::string address = xdatautil::serialize_owner_str(sys_contract_beacon_table_block_addr, 0);

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                chains.erase(address);
            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);
            }
        }

        // zec table
        {
            std::string address = xdatautil::serialize_owner_str(sys_contract_zec_table_block_addr, 0);

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second || state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                chains.erase(address);
            }
        }

        ASSERT_EQ(chains.size(), 0);
    }
};

TEST(xrole_chains_mgr, beacon_add_remove_zec) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t beacon_addr = create_beacon_addr(1, "aa", 1);
    std::vector<std::uint16_t> beacon_table_ids = {0};

    vnetwork::xvnode_address_t zec_addr = create_zec_addr(1, "aa", 1);
    std::vector<std::uint16_t> zec_table_ids = {0};

    beacon_add_remove_zec_checker checker(&mgr, beacon_table_ids, zec_table_ids);
    std::shared_ptr<xrole_chains_t> beacon_chains = std::make_shared<xrole_chains_t>(beacon_addr, beacon_table_ids);
    std::shared_ptr<xrole_chains_t> zec_chains = std::make_shared<xrole_chains_t>(zec_addr, zec_table_ids);

    {
        mgr.add_role(beacon_chains);
        checker.add_validator();
    }

    {
        mgr.add_role(zec_chains);
        checker.add_archive();
    }

    {
        mgr.remove_role(zec_chains);
        checker.remove_archive();
    }

    {
        mgr.remove_role(beacon_chains);
        checker.remove_validator();
    }
}

class zec_add_remove_beacon_checker {
private:
    enum enum_role_state {
        enum_role_op_add_first,
        enum_role_op_add_second,
        enum_role_op_remove_second,
        enum_role_op_remove_first,
    };

private:
    xrole_chains_mgr_t * m_mgr;
    std::vector<std::uint16_t> m_beacon_table_ids;
    std::vector<std::uint16_t> m_archive_table_ids;
    enum_role_state m_state{enum_role_op_add_first};

public:
    zec_add_remove_beacon_checker(xrole_chains_mgr_t * mgr, std::vector<std::uint16_t> & beacon_table_ids, std::vector<std::uint16_t> & archive_table_ids)
      : m_mgr(mgr), m_beacon_table_ids(beacon_table_ids), m_archive_table_ids(archive_table_ids) {}

    void add_validator() {
        ASSERT_EQ(m_state, enum_role_op_add_first);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_add_second;
    }

    void add_archive() {
        ASSERT_EQ(m_state, enum_role_op_add_second);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_remove_second;
    }

    void remove_archive() {
        ASSERT_EQ(m_state, enum_role_op_remove_second);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
        m_state = enum_role_op_remove_first;
    }

    void remove_validator() {
        ASSERT_EQ(m_state, enum_role_op_remove_first);
        checker(*m_mgr, m_beacon_table_ids, m_archive_table_ids, m_state);
    }

private:
    void checker(xrole_chains_mgr_t & mgr, const std::vector<std::uint16_t> & validator_table_ids, const std::vector<std::uint16_t> & archive_table_ids, enum_role_state state) {
        bool ret = false;

        map_chain_info_t tmp_chains = mgr.get_all_chains();
        map_chain_info_t chains = tmp_chains;

        // beacon table
        {
            std::string address = xdatautil::serialize_owner_str(sys_contract_beacon_table_block_addr, 0);

            if (state == enum_role_op_add_first || state == enum_role_op_remove_second || state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());

            } else if (state == enum_role_op_add_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                chains.erase(address);
            }
        }

        // zec table
        {
            std::string address = xdatautil::serialize_owner_str(sys_contract_zec_table_block_addr, 0);

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        ASSERT_EQ(chains.size(), 0);
    }
};

TEST(xrole_chains_mgr, zec_add_remove_beacon) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t zec_addr = create_zec_addr(1, "aa", 1);
    std::vector<std::uint16_t> zec_table_ids = {0};

    vnetwork::xvnode_address_t beacon_addr = create_beacon_addr(1, "aa", 1);
    std::vector<std::uint16_t> beacon_table_ids = {0};

    zec_add_remove_beacon_checker checker(&mgr, beacon_table_ids, zec_table_ids);
    std::shared_ptr<xrole_chains_t> beacon_chains = std::make_shared<xrole_chains_t>(beacon_addr, beacon_table_ids);
    std::shared_ptr<xrole_chains_t> zec_chains = std::make_shared<xrole_chains_t>(zec_addr, zec_table_ids);

    {
        mgr.add_role(zec_chains);
        checker.add_validator();
    }

    {
        mgr.add_role(beacon_chains);
        checker.add_archive();
    }

    {
        mgr.remove_role(beacon_chains);
        checker.remove_archive();
    }

    {
        mgr.remove_role(zec_chains);
        checker.remove_validator();
    }
}

class archive_add_remove_other_checker {
private:
    enum enum_role_state {
        enum_role_op_add_first,
        enum_role_op_add_second,
        enum_role_op_remove_second,
        enum_role_op_remove_first,
    };

private:
    xrole_chains_mgr_t * m_mgr;
    std::vector<std::uint16_t> m_archive_table_ids;
    enum_role_state m_state{enum_role_op_add_first};

public:
    archive_add_remove_other_checker(xrole_chains_mgr_t * mgr, std::vector<std::uint16_t> & archive_table_ids) : m_mgr(mgr), m_archive_table_ids(archive_table_ids) {}

    void add_archive() {
        ASSERT_EQ(m_state, enum_role_op_add_first);
        checker(*m_mgr, m_archive_table_ids, m_state);
        m_state = enum_role_op_add_second;
    }

    void add_other() {
        ASSERT_EQ(m_state, enum_role_op_add_second);
        checker(*m_mgr, m_archive_table_ids, m_state);
        m_state = enum_role_op_remove_second;
    }

    void remove_other() {
        ASSERT_EQ(m_state, enum_role_op_remove_second);
        checker(*m_mgr, m_archive_table_ids, m_state);
        m_state = enum_role_op_remove_first;
    }

    void remove_archive() {
        ASSERT_EQ(m_state, enum_role_op_remove_first);
        checker(*m_mgr, m_archive_table_ids, m_state);
    }

private:
    void checker(xrole_chains_mgr_t & mgr, const std::vector<std::uint16_t> & archive_table_ids, enum_role_state state) {
        bool ret = false;

        map_chain_info_t tmp_chains = mgr.get_all_chains();
        map_chain_info_t chains = tmp_chains;

        // beacon table
        {
            std::string address = xdatautil::serialize_owner_str(sys_contract_beacon_table_block_addr, 0);

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);
                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        // zec table
        {
            std::string address = xdatautil::serialize_owner_str(sys_contract_zec_table_block_addr, 0);

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        // shard table
        for (auto & table_id : archive_table_ids) {
            std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, table_id);

            if (state == enum_role_op_add_first || state == enum_role_op_add_second || state == enum_role_op_remove_second) {
                xchain_info_t info;
                ret = mgr.get_chain(address, info);
                ASSERT_EQ(ret, true);

                auto it = chains.find(address);
                ASSERT_NE(it, chains.end());
                ASSERT_EQ(it->second, info);

                ASSERT_EQ(it->second.address, address);

                chains.erase(address);

            } else if (state == enum_role_op_remove_first) {
                ret = mgr.exists(address);
                ASSERT_EQ(ret, false);

                auto it = chains.find(address);
                ASSERT_EQ(it, chains.end());
            }
        }

        ASSERT_EQ(chains.size(), 0);
    }
};

TEST(xrole_chains_mgr, archive_add_remove_beacon) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t archive_addr = create_archive_addr(1, "aa", 1);
    std::vector<std::uint16_t> archive_table_ids;
    for (int i = 0; i < 1024; i++)
        archive_table_ids.push_back(i);

    vnetwork::xvnode_address_t beacon_addr = create_beacon_addr(1, "aa", 1);
    std::vector<std::uint16_t> beacon_table_ids = {0};

    std::shared_ptr<xrole_chains_t> beacon_chains = std::make_shared<xrole_chains_t>(archive_addr, archive_table_ids);
    const map_chain_info_t & chains1 = beacon_chains->get_chains_wrapper().get_chains();

    archive_add_remove_other_checker checker(&mgr, archive_table_ids);

    {
        mgr.add_role(archive_addr, archive_table_ids);
        checker.add_archive();
    }

    {
        mgr.add_role(beacon_addr, beacon_table_ids);
        checker.add_other();
    }

    {
        mgr.remove_role(beacon_addr);
        checker.remove_other();
    }

    {
        mgr.remove_role(archive_addr);
        checker.remove_archive();
    }
}

TEST(xrole_chains_mgr, archive_add_remove_zec) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t archive_addr = create_archive_addr(1, "aa", 1);
    std::vector<std::uint16_t> archive_table_ids;
    for (int i = 0; i < 1024; i++)
        archive_table_ids.push_back(i);

    vnetwork::xvnode_address_t zec_addr = create_zec_addr(1, "aa", 1);
    std::vector<std::uint16_t> zec_table_ids = {0};

    std::shared_ptr<xrole_chains_t> beacon_chains = std::make_shared<xrole_chains_t>(archive_addr, archive_table_ids);
    const map_chain_info_t & chains1 = beacon_chains->get_chains_wrapper().get_chains();

    archive_add_remove_other_checker checker(&mgr, archive_table_ids);

    {
        mgr.add_role(archive_addr, archive_table_ids);
        checker.add_archive();
    }

    {
        mgr.add_role(zec_addr, zec_table_ids);
        checker.add_other();
    }

    {
        mgr.remove_role(zec_addr);
        checker.remove_other();
    }

    {
        mgr.remove_role(archive_addr);
        checker.remove_archive();
    }
}

TEST(xrole_chains_mgr, archive_add_remove_auditor) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t archive_addr = create_archive_addr(1, "aa", 1);
    std::vector<std::uint16_t> archive_table_ids;
    for (int i = 0; i < 1024; i++)
        archive_table_ids.push_back(i);

    vnetwork::xvnode_address_t auditor_addr = create_auditor_addr(1, "aa", 1);
    std::vector<std::uint16_t> auditor_table_ids;
    for (int i = 0; i < 256; i++)
        auditor_table_ids.push_back(i);

    std::shared_ptr<xrole_chains_t> beacon_chains = std::make_shared<xrole_chains_t>(archive_addr, archive_table_ids);
    const map_chain_info_t & chains1 = beacon_chains->get_chains_wrapper().get_chains();

    archive_add_remove_other_checker checker(&mgr, archive_table_ids);

    {
        mgr.add_role(archive_addr, archive_table_ids);
        checker.add_archive();
    }

    {
        mgr.add_role(auditor_addr, auditor_table_ids);
        checker.add_other();
    }

    {
        mgr.remove_role(auditor_addr);
        checker.remove_other();
    }

    {
        mgr.remove_role(archive_addr);
        checker.remove_archive();
    }
}

TEST(xrole_chains_mgr, archive_add_remove_validator) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t archive_addr = create_archive_addr(1, "aa", 1);
    std::vector<std::uint16_t> archive_table_ids;
    for (int i = 0; i < 1024; i++)
        archive_table_ids.push_back(i);

    vnetwork::xvnode_address_t validator_addr = create_validator_addr(1, "aa", 1);
    std::vector<std::uint16_t> validator_table_ids;
    for (int i = 0; i < 256; i++)
        validator_table_ids.push_back(i);

    archive_add_remove_other_checker checker(&mgr, archive_table_ids);

    {
        mgr.add_role(archive_addr, archive_table_ids);
        checker.add_archive();
    }

    {
        mgr.add_role(validator_addr, validator_table_ids);
        checker.add_other();
    }

    {
        mgr.remove_role(validator_addr);
        checker.remove_other();
    }

    {
        mgr.remove_role(archive_addr);
        checker.remove_archive();
    }
}

TEST(xrole_chains_mgr, archive_add_remove_multi_role) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t archive_addr = create_archive_addr(1, "aa", 1);
    std::vector<std::uint16_t> archive_table_ids;
    for (int i = 0; i < 1024; i++)
        archive_table_ids.push_back(i);

    vnetwork::xvnode_address_t auditor_addr1 = create_auditor_addr(1, "aa", 1);
    std::vector<std::uint16_t> auditor_table_ids;
    for (int i = 0; i < 256; i++)
        auditor_table_ids.push_back(i);

    vnetwork::xvnode_address_t auditor_addr2 = create_auditor_addr(1, "aa", 2);
    vnetwork::xvnode_address_t auditor_addr3 = create_auditor_addr(1, "aa", 3);
    vnetwork::xvnode_address_t auditor_addr4 = create_auditor_addr(1, "aa", 4);
    vnetwork::xvnode_address_t auditor_addr7 = create_auditor_addr(1, "aa", 7);

    mgr.add_role(archive_addr, archive_table_ids);
    mgr.add_role(auditor_addr1, auditor_table_ids);
    mgr.add_role(auditor_addr2, auditor_table_ids);
    mgr.add_role(auditor_addr3, auditor_table_ids);
    mgr.remove_role(auditor_addr1);
    mgr.add_role(auditor_addr4, auditor_table_ids);
    mgr.remove_role(auditor_addr2);
    mgr.add_role(auditor_addr7, auditor_table_ids);

    std::shared_ptr<xrole_chains_t> archive_chains = std::make_shared<xrole_chains_t>(archive_addr, archive_table_ids);
    const map_chain_info_t & chains1 = archive_chains->get_chains_wrapper().get_chains();

    std::string address = sys_contract_rec_elect_rec_addr;

    xchain_info_t info;
    bool ret = mgr.get_chain(address, info);
    ASSERT_EQ(ret, true);

    ASSERT_EQ(info.address, address);
}

class validator_update_checker {
private:
    xrole_chains_mgr_t * m_mgr;
    std::vector<std::uint16_t> m_table_ids;

public:
    validator_update_checker(xrole_chains_mgr_t * mgr, std::vector<std::uint16_t> & table_ids) : m_mgr(mgr), m_table_ids(table_ids) {}

    void add() { checker(*m_mgr, m_table_ids); }

private:
    void checker(xrole_chains_mgr_t & mgr, const std::vector<std::uint16_t> & table_ids) {
        bool ret = false;

        map_chain_info_t tmp_chains = mgr.get_all_chains();
        map_chain_info_t chains = tmp_chains;

        // shard table
        for (auto table_id : table_ids) {
            std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, table_id);

            auto it = chains.find(address);
            ASSERT_NE(it, chains.end());

            ASSERT_EQ(it->second.address, address);
            ASSERT_EQ(it->second.latest_policy, latest_policy_24h);
            ASSERT_EQ(it->second.active_policy, active_policy_none);
            ASSERT_EQ(it->second.anchor_policy, latest_policy_none);
            ASSERT_EQ(it->second.history_policy, history_policy_none);
            chains.erase(it);
        }

        ASSERT_EQ(chains.size(), 0);
    }
};

TEST(xrole_chains_mgr, add_higher_version) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t validator_addr1 = create_validator_addr(1, "aa", 1);
    vnetwork::xvnode_address_t validator_addr2 = create_validator_addr(1, "aa", 2);
    std::vector<std::uint16_t> validator_table_ids;
    for (uint32_t i = 0; i < 256; i++) {
        validator_table_ids.push_back(i);
    }

    validator_update_checker checker(&mgr, validator_table_ids);

    {
        mgr.add_role(validator_addr1, validator_table_ids);
        checker.add();
    }

    const xsync_roles_t & roles1 = mgr.get_roles();
    ASSERT_EQ(roles1.size(), 1);
    auto it1 = roles1.find(validator_addr1);
    ASSERT_NE(it1, roles1.end());

    {
        mgr.add_role(validator_addr2, validator_table_ids);
        checker.add();
    }

    const xsync_roles_t & roles2 = mgr.get_roles();
    ASSERT_EQ(roles2.size(), 1);
    auto it2 = roles2.find(validator_addr2);
    ASSERT_NE(it2, roles2.end());
}

TEST(xrole_chains_mgr, remove_lower_version) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t validator_addr2 = create_validator_addr(1, "aa", 2);
    std::vector<std::uint16_t> validator_table_ids;
    for (uint32_t i = 0; i < 256; i++) {
        validator_table_ids.push_back(i);
    }

    vnetwork::xvnode_address_t validator_addr1 = create_validator_addr(1, "aa", 1);

    validator_update_checker checker(&mgr, validator_table_ids);

    {
        mgr.add_role(validator_addr2, validator_table_ids);
        checker.add();
    }

    const xsync_roles_t & roles1 = mgr.get_roles();
    ASSERT_EQ(roles1.size(), 1);
    auto it1 = roles1.find(validator_addr2);
    ASSERT_NE(it1, roles1.end());

    {
        mgr.remove_role(validator_addr1);
        checker.add();
    }

    const xsync_roles_t & roles2 = mgr.get_roles();
    ASSERT_EQ(roles2.size(), 1);
    auto it2 = roles2.find(validator_addr2);
    ASSERT_NE(it2, roles2.end());
}

TEST(xrole_chains_mgr, remove_empty_role) {
    xsync_store_face_mock_t sync_store;
    xrole_chains_mgr_t mgr("", &sync_store);

    vnetwork::xvnode_address_t validator_addr = create_validator_addr(1, "aa", 1);
    std::vector<std::uint16_t> validator_table_ids;
    for (uint32_t i = 0; i < 256; i++) {
        validator_table_ids.push_back(i);
    }

    vnetwork::xvnode_address_t archive_addr = create_archive_addr(1, "aa", 1);

    validator_update_checker checker(&mgr, validator_table_ids);

    {
        mgr.add_role(validator_addr, validator_table_ids);
        checker.add();
    }

    const xsync_roles_t & roles1 = mgr.get_roles();
    ASSERT_EQ(roles1.size(), 1);
    auto it1 = roles1.find(validator_addr);
    ASSERT_NE(it1, roles1.end());

    {
        mgr.remove_role(archive_addr);
        checker.add();
    }

    const xsync_roles_t & roles2 = mgr.get_roles();
    ASSERT_EQ(roles2.size(), 1);
    auto it2 = roles2.find(validator_addr);
    ASSERT_NE(it2, roles2.end());
}







class xtest_block_t : public xblock_t {
public:
    xtest_block_t() : xblock_t(enum_xdata_type_max) {}

public:
    std::string account;
    uint64_t height;
    uint64_t timer_height;
};

class xtest_sync_store_mock_t : public xsync_store_face_mock_t {
public:
    xauto_ptr<xvblock_t> get_current_block(const std::string & account) override {
        auto it = map_height.find(account);
        if (it == map_height.end())
            assert(0);

        uint64_t h = it->second;

        xblock_para_t block_para;
        block_para.account = account;
        block_para.height = h;
        xblockheader_t * _blockheader = xblockheader_t::create_blockheader(block_para);
        xblockcert_t * _blockcert = new xblockcert_t;
        _blockcert->set_clock(0);
        xblock_t * blk = new xblock_t(*_blockheader, *_blockcert, base::xdataunit_t::enum_xdata_type_max);

        xauto_ptr<xvblock_t> vblock = blk;

        return vblock;
    }

public:
    std::map<std::string, uint64_t> map_height;
};

TEST(xrole_chains_mgr, archive_check_height) {
    xtest_sync_store_mock_t * mock_sync_store = new xtest_sync_store_mock_t();
    xrole_chains_mgr_t mgr("", mock_sync_store);

    std::string owner = xblocktool_t::make_address_user_account("11111111111111111111");

    vnetwork::xvnode_address_t archive_addr = create_archive_addr(1, owner, 1);
    std::vector<std::uint16_t> archive_table_ids;
    for (uint32_t i = 0; i < 1024; i++) {
        archive_table_ids.push_back(i);
    }

    std::shared_ptr<xrole_chains_t> archive_chains = std::make_shared<xrole_chains_t>(archive_addr, archive_table_ids);
    const map_chain_info_t & chains = archive_chains->get_chains_wrapper().get_chains();

    for (const auto & it : chains) {
        mock_sync_store->map_height[it.first] = 1;
    }

    mgr.add_role(archive_chains);
    map_chain_info_t all_chains = mgr.get_all_chains();
    for (const auto & it : all_chains) {
        const xchain_info_t & info = it.second;
        uint64_t h;
        uint64_t v;
        mgr.get_height_and_view(info.address, h, v);
        ASSERT_EQ(h, 1);
    }
}

TEST(xrole_chains_mgr, archive_add_remove_validator_check_height) {
    xtest_sync_store_mock_t * mock_sync_store = new xtest_sync_store_mock_t();
    xrole_chains_mgr_t mgr("", mock_sync_store);

    std::string owner = xblocktool_t::make_address_user_account("11111111111111111111");

    vnetwork::xvnode_address_t archive_addr = create_archive_addr(1, "aa", 1);
    std::vector<std::uint16_t> archive_table_ids;
    for (uint32_t i = 0; i < 1024; i++) {
        archive_table_ids.push_back(i);
    }

    vnetwork::xvnode_address_t validator_addr = create_validator_addr(1, "aa", 1);
    std::vector<std::uint16_t> validator_table_ids;
    for (uint32_t i = 0; i < 256; i++) {
        validator_table_ids.push_back(i);
    }

    // set all chains!!!
    std::shared_ptr<xrole_chains_t> archive_chains = std::make_shared<xrole_chains_t>(archive_addr, archive_table_ids);
    {
        const map_chain_info_t & chains = archive_chains->get_chains_wrapper().get_chains();
        for (const auto & it : chains) {
            mock_sync_store->map_height[it.first] = 1;
        }
    }

    // add archive
    mgr.add_role(archive_chains);
    {
        map_chain_info_t all_chains = mgr.get_all_chains();
        for (const auto & it : all_chains) {
            const xchain_info_t & info = it.second;
            uint64_t h;
            uint64_t v;
            mgr.get_height_and_view(info.address, h, v);
            ASSERT_EQ(h, 1);
        }
    }

    // add validator
    std::shared_ptr<xrole_chains_t> validator_chains = std::make_shared<xrole_chains_t>(validator_addr, validator_table_ids);
    mgr.add_role(validator_chains);
    {
        map_chain_info_t all_chains = mgr.get_all_chains();
        for (const auto & it : all_chains) {
            const xchain_info_t & info = it.second;
            uint64_t h;
            uint64_t v;
            mgr.get_height_and_view(info.address, h, v);
            ASSERT_EQ(h, 1);
        }
    }

    // remove validator
    mgr.remove_role(validator_chains);
    {
        map_chain_info_t all_chains = mgr.get_all_chains();
        for (const auto & it : all_chains) {
            const xchain_info_t & info = it.second;
            uint64_t h;
            uint64_t v;
            mgr.get_height_and_view(info.address, h, v);
            ASSERT_EQ(h, 1);
        }
    }
}

TEST(xrole_chains_mgr, validator_add_remove_archive_check_height) {
    xtest_sync_store_mock_t * mock_sync_store = new xtest_sync_store_mock_t();
    xrole_chains_mgr_t mgr("", mock_sync_store);

    vnetwork::xvnode_address_t archive_addr = create_archive_addr(1, "aa", 1);
    std::vector<std::uint16_t> archive_table_ids;
    for (uint32_t i = 0; i < 1024; i++) {
        archive_table_ids.push_back(i);
    }

    vnetwork::xvnode_address_t validator_addr = create_validator_addr(1, "aa", 1);
    std::vector<std::uint16_t> validator_table_ids;
    for (uint32_t i = 0; i < 256; i++) {
        validator_table_ids.push_back(i);
    }

    // set all chains!!!
    std::shared_ptr<xrole_chains_t> archive_chains = std::make_shared<xrole_chains_t>(archive_addr, archive_table_ids);
    {
        const map_chain_info_t & chains = archive_chains->get_chains_wrapper().get_chains();
        for (const auto & it : chains) {
            mock_sync_store->map_height[it.first] = 1;
        }
    }

    // add validator
    std::shared_ptr<xrole_chains_t> validator_chains = std::make_shared<xrole_chains_t>(validator_addr, validator_table_ids);
    mgr.add_role(validator_chains);
    {
        map_chain_info_t all_chains = mgr.get_all_chains();
        for (const auto & it : all_chains) {
            const xchain_info_t & info = it.second;
            uint64_t h;
            uint64_t v;
            mgr.get_height_and_view(info.address, h, v);
            ASSERT_EQ(h, 1);
        }
    }

    // add archive
    mgr.add_role(archive_chains);
    {
        map_chain_info_t all_chains = mgr.get_all_chains();
        for (const auto & it : all_chains) {
            const xchain_info_t & info = it.second;
            uint64_t h;
            uint64_t v;
            mgr.get_height_and_view(info.address, h, v);
            ASSERT_EQ(h, 1);
        }
    }

    // remove archive
    mgr.remove_role(validator_chains);
    {
        map_chain_info_t all_chains = mgr.get_all_chains();
        for (const auto & it : all_chains) {
            const xchain_info_t & info = it.second;
            uint64_t h;
            uint64_t v;
            mgr.get_height_and_view(info.address, h, v);
            ASSERT_EQ(h, 1);
        }
    }
}

#endif
