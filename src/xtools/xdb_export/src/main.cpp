#include "nlohmann/fifo_map.hpp"
#include "nlohmann/json.hpp"
#include "xblockstore/xblockstore_face.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xrootblock.h"
#include "xdata/xtable_bstate.h"
#include "xdb/xdb_factory.h"
#include "xstake/xstake_algorithm.h"
#include "xstore/xstore_face.h"
#include "xvledger/xvblock.h"
#include "xvledger/xvledger.h"

#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fstream>
#include <iostream>
#include <thread>

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
using top::base::xstring_utl;

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
    std::cout << "- ./xdb_export <config_json_file> <function_name>" << std::endl;
    std::cout << "    - <function_name>:" << std::endl;
    std::cout << "        - all_account" << std::endl;
    std::cout << "        - all_property" << std::endl;
    std::cout << "        - stake_property" << std::endl;
    std::cout << "        - check_fast_sync [table|unit] [account address]" << std::endl;
    std::cout << "        - check_block_exist <account address> <height>" << std::endl;
    std::cout << "        - check_tx_info [table]" << std::endl;
    std::cout << "        - check_latest_fullblock [table]" << std::endl;
    std::cout << "        - query <account address>" << std::endl;
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
    // std::make_pair(sys_contract_sharding_slash_info_addr, XPORPERTY_CONTRACT_TABLEBLOCK_HEIGHT_KEY),
    // std::make_pair(sys_contract_sharding_workload_addr, XPORPERTY_CONTRACT_TABLEBLOCK_HEIGHT_KEY),
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

struct tx_ext_t {
    std::string hash{""};
    int32_t tableid{-1};
    uint64_t height{0};
    uint64_t timestamp{0};
    std::string src{""};
    std::string target{""};
    uint64_t unit_height{0};
    uint8_t phase{0};
};
class db_export_tools {
public:
    db_export_tools(std::string const & db_path) {
        // std::shared_ptr<db::xdb_face_t> m_db = top::db::xdb_factory_t::instance(db_path);
        // m_bus = top::make_unique<mbus::xmessage_bus_t>(true, 1000);

        m_store = top::store::xstore_factory::create_store_with_kvdb(db_path);
        base::xvchain_t::instance().set_xdbstore(m_store.get());
        m_blockstore.attach(store::get_vblockstore());
        // m_indexstore = store::xindexstore_factory_t::create_indexstorehub(make_observer(m_store), make_observer(m_blockstore));

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
                auto latest_block = m_blockstore->get_latest_committed_block(address);
                auto block_height = latest_block->get_height();
                if (block_height)
                    std::cout << index << ": " << block_height << " " << std::endl;

                std::set<std::string> tmp_set;
                for (uint64_t h = 0; h <= block_height; ++h) {
                    auto vblock = m_blockstore->load_block_object(address,h,0,true);
                    data::xblock_t * block = dynamic_cast<data::xblock_t *>(vblock.get());
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

    static std::vector<std::string> get_all_table_address() {
        std::vector<std::string> all_address;
        const std::map<std::pair<std::string, std::string>, int> addr2name = {
            std::make_pair(std::make_pair(std::string{sys_contract_sharding_table_block_addr}, "sharding"), 256),
            std::make_pair(std::make_pair(std::string{sys_contract_zec_table_block_addr}, "zec"), MAIN_CHAIN_ZEC_TABLE_USED_NUM),
            std::make_pair(std::make_pair(std::string{sys_contract_beacon_table_block_addr}, "beacon"), MAIN_CHAIN_REC_TABLE_USED_NUM),
        };
        for (auto const & _p : addr2name) {
            for (auto index = 0; index < _p.second; ++index) {
                std::string address = _p.first.first + "@" + std::to_string(index);
                all_address.emplace_back(address);
            }
        }
        return all_address;
    }

    void query_all_table_sync_result(json & result_json) {
        const std::map<std::pair<std::string, std::string>, int> addr2name = {
            std::make_pair(std::make_pair(std::string{sys_contract_sharding_table_block_addr}, "sharding"), 256),
            std::make_pair(std::make_pair(std::string{sys_contract_zec_table_block_addr}, "zec"), 3),
            std::make_pair(std::make_pair(std::string{sys_contract_beacon_table_block_addr}, "beacon"), 1),
        };
        for (auto const & _p : addr2name) {
            for (auto index = 0; index < _p.second; ++index) {
                std::string address = _p.first.first + "@" + std::to_string(index);
                query_sync_result(address, result_json);
            }
        }
    }

    void query_all_unit_sync_result(std::set<std::string> & accounts_set, json & result_json) {
        for(auto const & account : accounts_set) {
            query_sync_result(account, result_json);
        }
    }

    void query_sync_result(std::string const & account, json & result_json) {
        std::string result;
        int start = 0;
        int end = -1;
        auto latest_block = m_blockstore->get_latest_committed_block(account);
        data::xblock_t * block = dynamic_cast<data::xblock_t *>(latest_block.get());
        if (latest_block == nullptr) {
            std::cout << "account " << account << "latest committed block is null" << std::endl;
            return;
        }
        auto block_height = latest_block->get_height();
        for (uint64_t h = 0; h <= block_height; h++) {
            auto vblock = m_blockstore->load_block_object(account,h,0,false);
            data::xblock_t * block = dynamic_cast<data::xblock_t *>(vblock.get());
            if (block == nullptr) {
                if (end != -1) {
                    if (start == end) {
                        result += std::to_string(start) + ',';
                    } else {
                        result += std::to_string(start) + '-' + std::to_string(end) + ',';
                    }
                }
                start = h+1;
                end = -1;
            } else {
                end = h;
            }
            if (h == block_height) {
                if (end != -1) {
                    if (start == end) {
                        result += std::to_string(start) + ',';
                    } else {
                        result += std::to_string(start) + '-' + std::to_string(end) + ',';
                    }
                }
            }
        }
        result_json[account] = result;
    }

    void query_vec_table_latest_fullblock(std::vector<std::string> const & address_vec) {
        if (address_vec.size() == 0) {
            return;
        }
        json result_json;
        for (auto const & _p : address_vec) {
            query_table_latest_fullblock(_p, result_json[_p]);
        }
        std::string filename;
        if (address_vec.size() == 1) {
            filename = address_vec[0] + "_latest_fullblock_info.json";
        } else {
            filename = "all_latest_fullblock_info.json";
        }

        std::ofstream out_json(filename);
        out_json << std::setw(4) << result_json;
        std::cout << "===> " << filename << " generated success!" << std::endl;
    }

    void query_vec_table_tx_info(std::vector<std::string> const & address_vec) {
        for (auto const & _p : address_vec) {
            json result_json;
            query_table_tx_info(_p, result_json);
            std::string filename = "./all_table_tx_info/" + _p + "_tx_info.json";
            std::ofstream out_json(filename);
            out_json << std::setw(4) << result_json[_p];
            std::cout << "===> " << filename << " generated success!" << std::endl;
        }
    }

    void query_vec_table_tx_info_multi_thread(std::vector<std::string> const & address_vec) {
        auto thread_helper = [](db_export_tools *arg, std::vector<std::string> const & address_vec) {
            arg->query_vec_table_tx_info(address_vec);
        };
        mkdir("all_table_tx_info", 0750);
        std::vector<std::vector<std::string>> address_vec_split;
        uint32_t thread_num = 8;
        uint32_t address_per_thread = address_vec.size() / thread_num;
        for (size_t i = 0; i < thread_num; i++) {
            uint32_t start_index = i * address_per_thread;
            uint32_t end_index = (i == (thread_num - 1)) ? address_vec.size() : ((i + 1) * address_per_thread);
            std::vector<std::string> thread_address;
            for (auto j = start_index; j < end_index; j++) {
                thread_address.emplace_back(address_vec[j]);
            }
            address_vec_split.emplace_back(thread_address);
        }
        std::vector<std::thread> all_thread;
        int finish_num = 0;
        for (auto i = 0U; i < thread_num; i++) {
            std::thread th(thread_helper, this, address_vec_split[i]);
            all_thread.emplace_back(std::move(th));
        }
        for (auto i = 0U; i < thread_num; i++) {
            all_thread[i].join();
        }
    }

    void query_block_exist(std::string const & address, const uint64_t height) {
        auto const & vblock = m_blockstore->load_block_object(address,height,0,false);
        const data::xblock_t * block = dynamic_cast<data::xblock_t *>(vblock.get());
        if (block == nullptr) {
            std::cout << "account: " << address << " , height: " << height << " , block not exist" << std::endl;
        } else {
            std::cout << "account: " << address << " , height: " << height << " , block exist" << std::endl;
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
#if 0 // TODO(jimmy)
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
#endif
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
    void query_table_latest_fullblock(std::string const & account, json & result_json) {
        auto vblock = m_blockstore->get_latest_committed_full_block(account);
        data::xblock_t * block = dynamic_cast<data::xblock_t *>(vblock.get());
        if (block == nullptr) {
            std::cout << " table " << account << " get_latest_committed_full_block null" << std::endl;
            return;
        }
        if (block->get_height() == 0) {
            return;
        }
        if (!block->is_fulltable()) {
            std::cout << " table " << account << " latest_committed_full_block is not full table" << std::endl;
            return;
        }
        result_json["height"] = block->get_height();
        auto root_hash = block->get_fullstate_hash();
        result_json["hash"] = to_hex_str(root_hash);
        base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(block);
        data::xtablestate_ptr_t tablestate = bstate != nullptr ? std::make_shared<data::xtable_bstate_t>(bstate.get()) : nullptr;
        if (bstate == nullptr) {
            result_json["bstate"] = "null";
            return;
        } else {
            data::xtablestate_ptr_t tablestate = std::make_shared<data::xtable_bstate_t>(bstate.get());
            result_json["account_size"] = tablestate->get_account_size();
        }
    }
    
    void query_table_tx_info(std::string const & account, json & result_json) {
        auto const & latest_block = m_blockstore->get_latest_committed_block(account);
        auto const block_height = latest_block->get_height();
        std::map<std::string, tx_ext_t> send;
        std::map<std::string, tx_ext_t> confirm;
        std::map<std::string, tx_ext_t> self;
        std::set<std::string> multi_tx;
        std::vector<tx_ext_t> multi;
        int tableid{-1};
        {
            std::vector<std::string> parts;
            if(xstring_utl::split_string(account,'@',parts) >= 2) {
                tableid = xstring_utl::toint32(parts[1]);
            } else {
                std::cout << account << " parse table id error" << tableid << std::endl;
                return;
            }
        }
        base::xvaccount_t _vaccount(account);
        for (uint64_t h = 0; h <= block_height; h++) {
            auto const & vblock = m_blockstore->load_block_object(account,h,0,false);
            const data::xblock_t * block = dynamic_cast<data::xblock_t *>(vblock.get());
            if (block == nullptr) {
                std::cout << " table " << account << " height " << h << " block null" << std::endl;
                continue;
            }
            m_blockstore->load_block_input(_vaccount, vblock.get());
            assert(block->get_block_level() == base::enum_xvblock_level_table);
            const uint64_t timestamp = block->get_timestamp();
            const std::vector<base::xventity_t*> & _table_inentitys = block->get_input()->get_entitys();
            uint32_t entitys_count = _table_inentitys.size();
            for (uint32_t index = 1; index < entitys_count; index++) {  // unit entity from index#1
                base::xvinentity_t* _table_unit_inentity = dynamic_cast<base::xvinentity_t*>(_table_inentitys[index]);
                base::xtable_inentity_extend_t extend;
                extend.serialize_from_string(_table_unit_inentity->get_extend_data());
                const xobject_ptr_t<base::xvheader_t> & _unit_header = extend.get_unit_header();
                const uint64_t unit_height = _unit_header->get_height();
                const std::vector<base::xvaction_t> &  input_actions = _table_unit_inentity->get_actions();
                for (auto & action : input_actions) {
                    if (action.get_org_tx_hash().empty()) {  // not txaction
                        continue;
                    }
                    data::xlightunit_action_t txaction(action);
                    auto tx_ptr = block->query_raw_transaction(txaction.get_tx_hash());
                    tx_ext_t tx_ext;
                    if (tx_ptr != nullptr) {
                        tx_ext.src = tx_ptr->get_source_addr();
                        tx_ext.target = tx_ptr->get_target_addr();
                    }
                    tx_ext.tableid = tableid;
                    tx_ext.height = h;
                    tx_ext.timestamp = timestamp;
                    tx_ext.hash = "0x" + txaction.get_tx_hex_hash();
                    tx_ext.unit_height = unit_height;
                    tx_ext.phase = txaction.get_tx_subtype();
                    if (txaction.get_tx_subtype() == enum_transaction_subtype_self) {
                        if (self.count(tx_ext.hash)) {
                            multi_tx.insert(tx_ext.hash);
                            multi.push_back(self[tx_ext.hash]);
                            multi.push_back(tx_ext);
                            self.erase(tx_ext.hash);
                        } else if (multi_tx.count(tx_ext.hash)) {
                            multi.push_back(tx_ext);
                        }
                    } else if (txaction.get_tx_subtype() == enum_transaction_subtype_send) {
                        if (send.count(tx_ext.hash)) {
                            multi_tx.insert(tx_ext.hash);
                            multi.push_back(send[tx_ext.hash]);
                            multi.push_back(tx_ext);
                            send.erase(tx_ext.hash);
                        } else if (multi_tx.count(tx_ext.hash)) {
                            multi.push_back(tx_ext);
                        } else {
                            send[tx_ext.hash] = tx_ext;
                        }
                    } else if (txaction.get_tx_subtype() == enum_transaction_subtype_confirm) {
                        if (confirm.count(tx_ext.hash)) {
                            multi_tx.insert(tx_ext.hash);
                            multi.push_back(confirm[tx_ext.hash]);
                            multi.push_back(tx_ext);
                            confirm.erase(tx_ext.hash);
                        } else if (multi_tx.count(tx_ext.hash)) {
                            multi.push_back(tx_ext);
                        } else {
                            confirm[tx_ext.hash] = tx_ext;
                        }
                    } else {
                        continue;
                    }
                }
            }
        }
        // process tx map
        std::vector<tx_ext_t> sendv;
        std::vector<tx_ext_t> confirmv;
        std::vector<tx_ext_t> selfv;
        std::vector<std::pair<tx_ext_t, tx_ext_t>> confirmedv;
        auto cmp1 = [](tx_ext_t lhs, tx_ext_t rhs) {
            return lhs.height < rhs.height;
        };
        auto cmp2 = [](std::pair<tx_ext_t, tx_ext_t> lhs, std::pair<tx_ext_t, tx_ext_t> rhs) {
            return lhs.first.height < rhs.first.height;
        };
        auto map_value_to_vec = [](std::map<std::string, tx_ext_t> & map, std::vector<tx_ext_t> & vec) {
            for (auto const & item : map) {
                vec.push_back(item.second);
            }
        };
        auto setj = [](std::vector<tx_ext_t> & vec, json & j) {
            for (auto const & item : vec) {
                json tx;
                tx["table height"] = item.height;
                tx["timestamp"] = item.timestamp;
                tx["source address"] = item.src;
                tx["target address"] = item.target;
                tx["unit height"] = item.unit_height;
                tx["phase"] = item.phase;
                tx["table id"] = item.tableid;
                j[item.hash] = tx;
            }
        };
        auto set_confirmedj = [](std::vector<std::pair<tx_ext_t, tx_ext_t>> & vec,
                                 json & j,
                                 uint32_t & count,
                                 uint32_t & total_confirm_time,
                                 uint32_t & max_confirm_time) {
            for (auto const & item : vec) {
                json tx;
                tx["time cost"] = item.second.timestamp-item.first.timestamp;
                tx["send time"] =  item.first.timestamp;
                tx["confirm time"] = item.second.timestamp;
                tx["send table height"] = item.first.height;
                tx["confirm table height"] = item.second.height;
                tx["send unit height"] = item.first.unit_height;
                tx["confirm unit height"] =item.second.unit_height;
                tx["source address"] = item.first.src;
                tx["target address"] = item.first.target;
                j[item.first.hash] = tx;
                count++;
                total_confirm_time += item.second.timestamp-item.first.timestamp;
                if (item.second.timestamp-item.first.timestamp > max_confirm_time) {
                    max_confirm_time = item.second.timestamp-item.first.timestamp;
                }
            }
        };
        for(auto it = send.begin(); it != send.end(); ) {
            if (confirm.count(it->first)) {
                confirmedv.push_back(std::make_pair(it->second, confirm[it->first]));
                confirm.erase(it->first);
                send.erase(it++);
            }
            else{
                ++it;
            }
        }
        map_value_to_vec(send, sendv);
        map_value_to_vec(confirm, confirmv);
        map_value_to_vec(self, selfv);
        std::sort(sendv.begin(), sendv.end(), cmp1);
        std::sort(confirmv.begin(), confirmv.end(), cmp1);
        std::sort(selfv.begin(), selfv.end(), cmp1);
        std::sort(confirmedv.begin(), confirmedv.end(), cmp2);

        uint32_t count = 0;
        uint32_t total_confirm_time = 0;
        uint32_t max_confirm_time = 0;
        uint32_t average_confirm_time = 0;
        json tx_send;
        json tx_confirm;
        json tx_self;
        json tx_multi;
        json tx_confirmed;
        set_confirmedj(confirmedv, tx_confirmed, count, total_confirm_time, max_confirm_time);
        setj(sendv, tx_send);
        setj(confirmv, tx_confirm);
        setj(multi, tx_multi);
        json j;
        j["confirmed conut"] = count;
        j["send only count"] = sendv.size();
        j["confirmed only count"] = confirmv.size();
        j["confirmed total time"] = total_confirm_time;
        j["confirmed max time"] = max_confirm_time;
        j["confirmed avg time"] = float(total_confirm_time) / count;
        j["confirmed detail"] = tx_confirmed;
        j["send only detail"] = tx_send;
        j["confirmed only detail"] = tx_confirm;
        j["multi detail"] = tx_multi;
        result_json[account] = j;
    }
    // std::unique_ptr<mbus::xmessage_bus_face_t> m_bus;
    // std::shared_ptr<top::db::xdb_face_t> m_db;
    top::xobject_ptr_t<top::store::xstore_face_t> m_store;
    top::xobject_ptr_t<top::base::xvblockstore_t> m_blockstore;
    // top::xobject_ptr_t<top::store::xindexstorehub_t> m_indexstore;
};

int main(int argc, char ** argv) {
    auto hash_plugin = new xtop_hash_t();
    top::config::config_register.get_instance().set(config::xmin_free_gas_balance_onchain_goverance_parameter_t::name, std::to_string(ASSET_TOP(100)));
    top::config::config_register.get_instance().set(config::xfree_gas_onchain_goverance_parameter_t::name, std::to_string(25000));
    top::config::config_register.get_instance().set(config::xmax_validator_stake_onchain_goverance_parameter_t::name, std::to_string(5000));
    top::config::config_register.get_instance().set(config::xchain_name_configuration_t::name, std::string{top::config::chain_name_testnet});
    data::xrootblock_para_t para;
    data::xrootblock_t::init(para);

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
    } else if (function_name == "check_fast_sync") {
        std::set<std::string> accounts_set;
        json accounts_json;
        json result_json;
        std::string file_name;

        if (argc == 3) {
            tools.get_all_unit_account(accounts_set, accounts_json);
            json table_json;
            json unit_json;
            tools.query_all_table_sync_result(table_json);
            tools.query_all_unit_sync_result(accounts_set, unit_json);
            result_json["table"] = table_json;
            result_json["unit"] = unit_json;
            file_name = "all_sync_result.json";
        } else {
            std::string method_name{argv[3]};
            if (method_name == "table") {
                if (argc == 4) {
                    tools.query_all_table_sync_result(result_json);
                    file_name = "all_table_sync_result.json";
                } else if (argc == 5) {
                    tools.query_sync_result(argv[4], result_json);
                    file_name = std::string{argv[4]} + "_table_sync_result.json";
                } else {
                    usage();
                    return -1;
                }
            } else if (method_name == "unit") {
                tools.get_all_unit_account(accounts_set, accounts_json);
                if (argc == 4) {
                    tools.query_all_unit_sync_result(accounts_set, result_json);
                    file_name = "all_unit_sync_result.json";
                } else if (argc == 5) {
                    tools.query_sync_result(argv[4], result_json);
                    file_name = std::string{argv[4]} + "_unit_sync_result.json";
                }
                else {
                    usage();
                    return -1;
                }
            }else {
                usage();
                return -1;
            }
        }
        std::ofstream out_json(file_name);
        out_json << std::setw(4) << result_json;
    } else if (function_name == "check_tx_info") {
        if (argc == 3) {
            auto const & all_table_address = db_export_tools::get_all_table_address();
            tools.query_vec_table_tx_info_multi_thread(all_table_address);
        } else if (argc == 4) {
            std::vector<std::string> address = {argv[3]};
            tools.query_vec_table_tx_info(address);
        } else {
            usage();
            return -1;
        }
    } else if (function_name == "check_block_exist") {
        if (argc < 5) {
            usage();
            return -1;
        }
        tools.query_block_exist(argv[3], std::stoi(argv[4]));
    } else if (function_name == "check_latest_fullblock") {
        if (argc == 3) {
            auto const & all_table_address = db_export_tools::get_all_table_address();
            tools.query_vec_table_latest_fullblock(all_table_address);
        } else if (argc == 4) {
            std::vector<std::string> address = {argv[3]};
            tools.query_vec_table_latest_fullblock(address);
        } else {
            usage();
            return -1;
        }
    } else {
        usage();
    }
    return 0;
}
