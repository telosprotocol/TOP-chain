#include "nlohmann/fifo_map.hpp"
#include "nlohmann/json.hpp"
#include "xbase/xvledger.h"
#include "xblockstore/xblockstore_face.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xnative_contract_address.h"
#include "xdb/xdb_factory.h"
#include "xstore/xstore_face.h"
#include "xstake/xstake_algorithm.h"
#include <fstream>
#include <iostream>

#define ALWAYS_OVERWRITE false

// A workaround to give to use fifo_map as map, we are just ignoring the 'less' compare
template <class K, class V, class dummy_compare, class A>
using my_workaround_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;

using unordered_json = nlohmann::basic_json<my_workaround_fifo_map>;
using json = unordered_json;

using namespace top;
using namespace top::xstake;
using top::base::xcontext_t;
using top::base::xstream_t;

class xtop_hash_t : public top::base::xhashplugin_t {
public:
    xtop_hash_t()
      : base::xhashplugin_t(-1)  //-1 = support every hash types
    {
    }

private:
    xtop_hash_t(const xtop_hash_t &) = delete;
    xtop_hash_t & operator=(const xtop_hash_t &) = delete;

public:
    virtual ~xtop_hash_t(){};
    virtual const std::string hash(const std::string & input, enum_xhash_type type) override {
        xassert(type == enum_xhash_type_sha2_256);
        auto hash = utl::xsha2_256_t::digest(input);
        return std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    }
};

void usage() {
    std::cout << "------- usage -------" << std::endl;
    std::cout << "- ./xdb_export [config_json_file] [function_name]" << std::endl;
    std::cout << "    - [function_name]:" << std::endl;
    std::cout << "        - all_account" << std::endl;
    std::cout << "        - all_property" << std::endl;
    std::cout << "        - stake_property" << std::endl;
    std::cout << "        - query [account address]" << std::endl;
    std::cout << "-------  end  -------" << std::endl;
}

static std::vector<std::pair<std::string, std::string>> stake_map_string_string_pair_list = {
    std::make_pair(sys_contract_rec_registration_addr, XPORPERTY_CONTRACT_TICKETS_KEY),
    std::make_pair(sys_contract_rec_registration_addr, XPORPERTY_CONTRACT_REG_KEY),
    std::make_pair(sys_contract_rec_registration_addr, XPORPERTY_CONTRACT_VOTE_REPORT_TIME_KEY),
    std::make_pair(sys_contract_rec_registration_addr, XPROPERTY_CONTRACT_SLASH_INFO_KEY),
    std::make_pair(sys_contract_rec_registration_addr, XPORPERTY_CONTRACT_REFUND_KEY),
    std::make_pair(sys_contract_zec_reward_addr, XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE),
    std::make_pair(sys_contract_zec_reward_addr, XPORPERTY_CONTRACT_TASK_KEY),
    // std::make_pair(sys_contract_zec_workload_addr, XPORPERTY_CONTRACT_TGAS_KEY),
    std::make_pair(sys_contract_zec_workload_addr, XPORPERTY_CONTRACT_WORKLOAD_KEY),
    std::make_pair(sys_contract_zec_workload_addr, XPORPERTY_CONTRACT_VALIDATOR_WORKLOAD_KEY),
    std::make_pair(sys_contract_zec_slash_info_addr, XPROPERTY_CONTRACT_TABLEBLOCK_NUM_KEY),
    std::make_pair(sys_contract_zec_slash_info_addr, XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY),
    std::make_pair(sys_contract_zec_vote_addr, XPORPERTY_CONTRACT_VOTE_REPORT_TIME_KEY),
    std::make_pair(sys_contract_zec_vote_addr, XPORPERTY_CONTRACT_TICKETS_KEY),
};

static std::vector<std::pair<std::string, std::string>> stake_string_pair_list = {
    std::make_pair(sys_contract_rec_registration_addr, XPORPERTY_CONTRACT_GENESIS_STAGE_KEY),
    std::make_pair(sys_contract_zec_reward_addr, XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_YEARLY),
    std::make_pair(sys_contract_zec_reward_addr, XPROPERTY_LAST_READ_REC_REG_CONTRACT_BLOCK_HEIGHT),
    std::make_pair(sys_contract_zec_reward_addr, XPROPERTY_LAST_READ_REC_REG_CONTRACT_LOGIC_TIME),
    std::make_pair(sys_contract_zec_reward_addr, XPROPERTY_REWARD_DETAIL),
};

static std::vector<std::pair<std::string, std::string>> table_stake_string_pair_list = {
    std::make_pair(sys_contract_sharding_slash_info_addr, XPORPERTY_CONTRACT_TABLEBLOCK_HEIGHT_KEY),
    std::make_pair(sys_contract_sharding_workload_addr, XPORPERTY_CONTRACT_TABLEBLOCK_HEIGHT_KEY),
    // std::make_pair(sys_contract_sharding_vote_addr, XPORPERTY_CONTRACT_TIME_KEY),
};

static std::vector<std::pair<std::string, std::string>> table_stake_map_string_string_pair_list = {
    std::make_pair(sys_contract_sharding_reward_claiming_addr, XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY1),
    std::make_pair(sys_contract_sharding_reward_claiming_addr, XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY2),
    std::make_pair(sys_contract_sharding_reward_claiming_addr, XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY3),
    std::make_pair(sys_contract_sharding_reward_claiming_addr, XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY4),
    std::make_pair(sys_contract_sharding_reward_claiming_addr, XPORPERTY_CONTRACT_NODE_REWARD_KEY),
    std::make_pair(sys_contract_sharding_vote_addr, XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY1),
    std::make_pair(sys_contract_sharding_vote_addr, XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY2),
    std::make_pair(sys_contract_sharding_vote_addr, XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY3),
    std::make_pair(sys_contract_sharding_vote_addr, XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY4),
    std::make_pair(sys_contract_sharding_vote_addr, XPORPERTY_CONTRACT_POLLABLE_KEY),
};

class db_export_tools {
public:
    db_export_tools(std::string const & db_path) {
        // std::shared_ptr<db::xdb_face_t> m_db = top::db::xdb_factory_t::instance(db_path);
        // m_bus = top::make_unique<mbus::xmessage_bus_t>(true, 1000);

        m_store = top::store::xstore_factory::create_store_with_kvdb(db_path);

        // m_store = top::store::xstore_factory::create_store_with_static_kvdb(m_db, make_observer(m_bus));
        // m_blockstore.attach(store::xblockstorehub_t::instance().get_block_store(*m_store, ""));
    }

    void get_all_unit_account(std::set<std::string> & accounts_set, json & accounts_json) {
        if (!ALWAYS_OVERWRITE) {
            try {
                std::ifstream all_account_file("all_account.json");
                json j;
                all_account_file >> j;
                if ((j.find("sharding@0") != j.end())&&(j.find("zec@0") != j.end())&&(j.find("beacon@0") != j.end())) {
                    std::cout << "all_account.json file already exist" << std::endl;
                    accounts_json = j;
                    for (auto _table : j) {
                        for (auto _acc : _table) {
                            accounts_set.insert(_acc.get<std::string>());
                            // std::cout << _acc << std::endl;
                        }
                    }
                    return;
                }
            } catch (...) {
            }
        }

        const std::vector<std::pair<std::string, std::string>> addr2name = {
            std::make_pair(std::string{sys_contract_sharding_table_block_addr}, "sharding"),
            std::make_pair(std::string{sys_contract_zec_table_block_addr}, "zec"),
            std::make_pair(std::string{sys_contract_beacon_table_block_addr}, "beacon"),
        };
        for (auto const & _p : addr2name) {
            for (auto index = 0; index < 256; ++index) {
                std::string address = _p.first + "@" + std::to_string(index);
                auto block_height = m_store->get_blockchain_height(address);
                if (block_height)
                    std::cout << index << ": " << block_height << " " << std::endl;

                std::set<std::string> tmp_set;
                for (uint64_t h = 0; h <= block_height; ++h) {
                    base::xauto_ptr<xblock_t> block = m_store->get_block_by_height(address, h);

                    if (block != nullptr) {
                        assert(block->get_block_level() == base::enum_xvblock_level_table);
                        auto const & units = block->get_tableblock_units(false);
                        if (!units.empty()) {
                            for (auto & unit : units) {
                                auto unit_address = unit->get_block_owner();
                                tmp_set.insert(unit_address);
                            }
                        }
                    }
                }
                for (auto & s : tmp_set) {
                    accounts_set.insert(s);
                    accounts_json[std::string{_p.second + "@" + std::to_string(index)}].push_back(s);
                }
            }
        }
    }

    void query_unit_property(std::string const & address, json & accounts_json) {
        std::cout << "account " << address << std::endl;
        xaccount_ptr_t account_ptr = m_store->query_account(address);
        if (account_ptr == nullptr) {
            std::cout << "nullptr";
            return;
        }
        json jaccount_native;
        get_unit_native_property(account_ptr, jaccount_native);
        accounts_json[address]["native_property"] = jaccount_native;
        json jaccount_user;
        get_unit_user_property(account_ptr, jaccount_user);
        accounts_json[address]["user_property"] = jaccount_user;
    }

    void query_unit_set_property(std::set<std::string> const & accounts_set, json & accounts_json) {
        for (auto const & account : accounts_set) {
            std::cout << "account " << account << std::endl;
            xaccount_ptr_t account_ptr = m_store->query_account(account);
            if (account_ptr == nullptr) {
                continue;
            }
            json jaccount_native;
            get_unit_native_property(account_ptr, jaccount_native);
            accounts_json[account]["native_property"] = jaccount_native;
            json jaccount_user;
            get_unit_user_property(account_ptr, jaccount_user);
            accounts_json[account]["user_property"] = jaccount_user;
        }
    }

    void get_stake_property_string(std::string const & addr, std::string const & property_key, json & stake_json) {
        std::string value;
        m_store->string_get(addr, property_key, value);
        std::string base64_str = base::xstring_utl::base64_encode((const uint8_t *)value.data(), value.size());
        stake_json[addr][property_key] = base64_str;
    }

    void get_stake_property_map_string_string(std::string const & addr, std::string const & property_key, json & stake_json) {
        json ser_res;
        std::map<std::string, std::string> value;
        m_store->map_copy_get(addr, property_key, value);
        for (auto m : value) {
            std::string first_base64_str = base::xstring_utl::base64_encode((const uint8_t *)m.first.data(), m.first.size());
            std::string second_base64_str = base::xstring_utl::base64_encode((const uint8_t *)m.second.data(), m.second.size());
            ser_res[first_base64_str] = second_base64_str;
        }
        stake_json[addr][property_key] = ser_res;
    }

    void get_table_stake_property_string(std::string const &addr,std::string const & property_key,json & stake_json){
        for (auto index = 0; index < 256; ++index) {
            std::string table_addr = addr + "@" + std::to_string(index);
            std::string value;
            m_store->string_get(table_addr, property_key, value);
            std::string base64_str = base::xstring_utl::base64_encode((const uint8_t *)value.data(), value.size());
            if (!value.empty())
                stake_json[addr]["@" + std::to_string(index)][property_key] = base64_str;
        }
    }

    void get_table_stake_property_map_string_string(std::string const & addr, std::string const & property_key, json & stake_json) {
        for (auto index = 0; index < 256; ++index) {
            std::string table_addr = addr + "@" + std::to_string(index);
            json ser_res;
            std::map<std::string, std::string> value;
            m_store->map_copy_get(table_addr, property_key, value);
            for (auto m : value) {
                std::string first_base64_str = base::xstring_utl::base64_encode((const uint8_t *)m.first.data(), m.first.size());
                std::string second_base64_str = base::xstring_utl::base64_encode((const uint8_t *)m.second.data(), m.second.size());
                ser_res[first_base64_str] = second_base64_str;
            }
            if (!ser_res.empty())
                stake_json[addr]["@" + std::to_string(index)][property_key] = ser_res;
        }
    }

    void test_get_stake(json & stake_json){
        std::map<std::string, std::string> nodes;
        m_store->map_copy_get(sys_contract_rec_registration_addr,XPORPERTY_CONTRACT_REG_KEY,nodes);
#if 0
        for (auto m : nodes) {
            xstake::xreg_node_info reg_node_info;
            xstream_t stream(xcontext_t::instance(), (uint8_t *)m.second.data(), m.second.size());
            reg_node_info.serialize_from(stream);
            json j;
            j["account_addr"] = reg_node_info.m_account;
            j["node_deposit"] = static_cast<unsigned long long>(reg_node_info.m_account_mortgage);
            if (reg_node_info.m_genesis_node) {
                j["registered_node_type"] = std::string{"advance,validator,edge"};
            } else {
                j["registered_node_type"] = common::to_string(reg_node_info.m_registered_role);
            }
            j["vote_amount"] = static_cast<unsigned long long>(reg_node_info.m_vote_amount);
            {
                auto credit = static_cast<double>(reg_node_info.m_auditor_credit_numerator) / reg_node_info.m_auditor_credit_denominator;
                std::stringstream ss;
                ss << std::fixed << std::setprecision(6) << credit;
                j["auditor_credit"] = ss.str();
            }
            {
                auto credit = static_cast<double>(reg_node_info.m_validator_credit_numerator) / reg_node_info.m_validator_credit_denominator;
                std::stringstream ss;
                ss << std::fixed << std::setprecision(6) << credit;
                j["validator_credit"] = ss.str();
            }
            j["dividend_ratio"] = reg_node_info.m_support_ratio_numerator * 100 / reg_node_info.m_support_ratio_denominator;
            // j["m_stake"] = static_cast<unsigned long long>(reg_node_info.m_stake);
            j["auditor_stake"] = static_cast<unsigned long long>(reg_node_info.get_auditor_stake());
            j["validator_stake"] = static_cast<unsigned long long>(reg_node_info.get_validator_stake());
            j["rec_stake"] = static_cast<unsigned long long>(reg_node_info.rec_stake());
            j["zec_stake"] = static_cast<unsigned long long>(reg_node_info.zec_stake());
            std::string network_ids;
            for (auto const & net_id : reg_node_info.m_network_ids) {
                network_ids += base::xstring_utl::tostring(net_id) + ' ';
            }
            j["network_id"] = network_ids;
            j["nodename"] = reg_node_info.nickname;
            j["node_sign_key"] = reg_node_info.consensus_public_key.to_string();
            stake_json[m.first] = j;
        }
#endif
    }

private:
    void get_unit_native_property(xaccount_ptr_t const & account_ptr, json & j) {
        auto property = account_ptr->get_native_property().get_properties();
        for (auto const & iter : property) {
            std::string value;
            auto bytes_num = iter.second.get()->serialize_to_string(value);
            std::string base64_str = base::xstring_utl::base64_encode((const uint8_t *)value.data(), value.size());
            j[iter.first] = base64_str;
#if 0
            std::cout << iter.first << " raw data:";
            auto raw = value.data();
            for (int i = 0; i < bytes_num; i++) {
                printf("0x%x ", raw[i]);
            }
            std::cout << std::endl;
            std::cout << iter.first << " base64 data:";
            std::cout << base64_str <<std::endl;
#endif
        }
    }

    void get_unit_user_property(xaccount_ptr_t const & account_ptr, json & j) {
        j["balance"] = base::xstring_utl::tostring(account_ptr->balance());
        j["burn_balance"] = base::xstring_utl::tostring(account_ptr->burn_balance());
        j["tgas_balance"] = base::xstring_utl::tostring(account_ptr->tgas_balance());
        j["disk_balance"] = base::xstring_utl::tostring(account_ptr->disk_balance());
        j["vote_balance"] = base::xstring_utl::tostring(account_ptr->vote_balance());
        j["lock_balance"] = base::xstring_utl::tostring(account_ptr->lock_balance());
        j["lock_tgas"] = base::xstring_utl::tostring(account_ptr->lock_tgas());
        j["unvote_num"] = base::xstring_utl::tostring(account_ptr->unvote_num());
        j["get_unconfirm_sendtx_num"] = base::xstring_utl::tostring(account_ptr->get_unconfirm_sendtx_num());
        j["get_last_full_unit_height"] = base::xstring_utl::tostring(account_ptr->get_last_full_unit_height());
        j["account_send_trans_number"] = base::xstring_utl::tostring(account_ptr->account_send_trans_number());
        j["account_recv_trans_number"] = base::xstring_utl::tostring(account_ptr->account_recv_trans_number());
        j["get_free_tgas"] = base::xstring_utl::tostring(account_ptr->get_free_tgas());
        j["get_last_tx_hour"] = base::xstring_utl::tostring(account_ptr->get_last_tx_hour());
        j["get_used_tgas"] = base::xstring_utl::tostring(account_ptr->get_used_tgas());
    }

private:
    // std::unique_ptr<mbus::xmessage_bus_face_t> m_bus;
    // std::shared_ptr<top::db::xdb_face_t> m_db;
    top::xobject_ptr_t<top::store::xstore_face_t> m_store;
};

int main(int argc, char ** argv) {
    auto hash_plugin = new xtop_hash_t();
    top::config::config_register.get_instance().set(config::xmin_free_gas_balance_onchain_goverance_parameter_t::name, std::to_string(ASSET_TOP(100)));
    top::config::config_register.get_instance().set(config::xfree_gas_onchain_goverance_parameter_t::name, std::to_string(25000));
    top::config::config_register.get_instance().set(config::xmax_validator_stake_onchain_goverance_parameter_t::name, std::to_string(5000));

    if (argc < 3) {
        usage();
        return -1;
    }
    std::string file_path{argv[1]};
    std::cout << file_path << std::endl;

    json j;
    std::ifstream config_file(file_path);
    config_file >> j;
    std::cout << "db_path: " << j.at("db_path") << std::endl;

    db_export_tools tools{j.at("db_path")};

    std::string function_name{argv[2]};
    if (function_name == "all_account") {
        std::set<std::string> accounts_set;
        json accounts_json;
        tools.get_all_unit_account(accounts_set, accounts_json);
        for (auto const & s : accounts_set) {
            std::cout << s << std::endl;
        }
        std::ofstream out_json("all_account.json");
        out_json << std::setw(4) << accounts_json;
    } else if (function_name == "all_property") {
        std::set<std::string> accounts_set;
        json accounts_json;
        json property_json;
        tools.get_all_unit_account(accounts_set, accounts_json);
        tools.query_unit_set_property(accounts_set, property_json);
        std::ofstream out_json("all_property.json");
        out_json << std::setw(4) << property_json;
    } else if (function_name == "query") {
        if (argc < 4) {
            usage();
            return -1;
        }
        std::string address{argv[3]};
        json accounts_json;
        tools.query_unit_property(address, accounts_json);
        std::ofstream out_json(address + ".json");
        out_json << std::setw(4) << accounts_json;
    } else if (function_name == "query_units") {
        std::set<std::string> accounts_set;
        json res, _j;
        tools.get_all_unit_account(accounts_set, _j);
        for (auto const & s : accounts_set) {
            json accounts_json;
            tools.query_unit_property(s, accounts_json);
            res[s] = accounts_json;
        }
        std::ofstream out_json("query_units.json");
        out_json << std::setw(4) << res;
    }else if (function_name == "stake_property"){
        json res;
        for (auto const & _pair:stake_map_string_string_pair_list) {
            tools.get_stake_property_map_string_string(_pair.first, _pair.second, res);
        }
        for(auto const & _pair:stake_string_pair_list){
            tools.get_stake_property_string(_pair.first, _pair.second, res);
        }
        for (auto const & _pair:table_stake_map_string_string_pair_list) {
            tools.get_table_stake_property_map_string_string(_pair.first, _pair.second, res);
        }
        for(auto const & _pair:table_stake_string_pair_list){
            tools.get_table_stake_property_string(_pair.first, _pair.second, res);
        }
        std::ofstream out_json("stake_property.json");
        out_json << std::setw(4) << res;
    #if 0
    } else if (function_name == "stake") {
        json res;
        tools.test_get_stake(res);
        std::ofstream out_json("stake.json");
        out_json << std::setw(4) << res;
    #endif
    } else {
        usage();
    }
    return 0;
}