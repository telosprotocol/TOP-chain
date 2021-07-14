#include "../services/xgrpcservice/xgrpc_service.h"
#include "nlohmann/fifo_map.hpp"
#include "nlohmann/json.hpp"
#include "xblockstore/xblockstore_face.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xelection/xelection_info_bundle.h"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xproposal_data.h"
#include "xdata/xrootblock.h"
#include "xdata/xtable_bstate.h"
#include "xdb/xdb_factory.h"
#include "xelection/xvnode_house.h"
#include "xrpc/xgetblock/get_block.h"
#include "xstake/xstake_algorithm.h"
#include "xstore/xstore_face.h"
#include "xvledger/xvblock.h"
#include "xvledger/xvledger.h"
#include "xvm/manager/xcontract_manager.h"

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
using namespace top::data;
using namespace top::election;
using top::base::xcontext_t;
using top::base::xstream_t;
using top::base::xstring_utl;
using top::base::xtime_utl;

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
    std::cout << "        - checkout_all_account" << std::endl;
    std::cout << "        - check_fast_sync [table|unit|account]" << std::endl;
    std::cout << "        - check_block_exist <account> <height>" << std::endl;
    std::cout << "        - check_block_info <account> <height|last>" << std::endl;
    std::cout << "        - check_tx_info [account]" << std::endl;
    std::cout << "        - check_latest_fullblock" << std::endl;
    std::cout << "        - check_contract_property <account> <prop> <last|all>" << std::endl;
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
        data::xrootblock_para_t para;
        data::xrootblock_t::init(para);
        top::config::config_register.get_instance().set(config::xmin_free_gas_balance_onchain_goverance_parameter_t::name, std::to_string(ASSET_TOP(100)));
        top::config::config_register.get_instance().set(config::xfree_gas_onchain_goverance_parameter_t::name, std::to_string(25000));
        top::config::config_register.get_instance().set(config::xmax_validator_stake_onchain_goverance_parameter_t::name, std::to_string(5000));
        top::config::config_register.get_instance().set(config::xchain_name_configuration_t::name, std::string{top::config::chain_name_testnet});

        m_node_id = common::xnode_id_t{"T00000LgGPqEpiK6XLCKRj9gVPN8Ej1aMbyAb3Hu"};
        m_sign_key = "ONhWC2LJtgi9vLUyoa48MF3tiXxqWf7jmT9KtOg/Lwo=";
        m_bus = top::make_object_ptr<mbus::xmessage_bus_t>(true, 1000);
        m_store = top::store::xstore_factory::create_store_with_kvdb(db_path);
        base::xvchain_t::instance().set_xdbstore(m_store.get());
        m_blockstore.attach(store::get_vblockstore());
        m_nodesvr_ptr = make_object_ptr<xvnode_house_t>(m_node_id, m_sign_key, m_blockstore, make_observer(m_bus.get()));
        m_getblock = std::make_shared<chain_info::get_block_handle>(m_store.get(), m_blockstore.get(), nullptr);
        contract::xcontract_manager_t::instance().init(make_observer(m_store), xobject_ptr_t<store::xsyncvstore_t>{});
        contract::xcontract_manager_t::set_nodesrv_ptr(m_nodesvr_ptr);
    }

    void get_all_unit_account(std::set<std::string> & accounts_set) {
        if (!ALWAYS_OVERWRITE) {
            try {
                std::ifstream all_account_file("all_account.json");
                json j;
                all_account_file >> j;
                if (!j.empty()) {
                    for (auto _table : j) {
                        for (auto _acc : _table) {
                            accounts_set.insert(_acc.get<std::string>());
                        }
                    }
                    return;
                }
            } catch (...) {
            }
        }

        gen_all_unit_account_file2(accounts_set);
    }

    std::vector<std::string> get_all_unit_account() {
        std::vector<std::string> accounts_vec;
        std::set<std::string> accounts_set;
        get_all_unit_account(accounts_set);
        for (auto const & item : accounts_set) {
            accounts_vec.emplace_back(item);
        }
        return accounts_vec;
    }

    static std::vector<std::string> get_all_table_account() {
        std::vector<std::string> account_vec;
        const std::vector<std::pair<std::string, int>> addr2name = {
            std::make_pair(std::string{sys_contract_sharding_table_block_addr}, 256),
            std::make_pair(std::string{sys_contract_zec_table_block_addr}, MAIN_CHAIN_ZEC_TABLE_USED_NUM),
            std::make_pair(std::string{sys_contract_beacon_table_block_addr}, MAIN_CHAIN_REC_TABLE_USED_NUM),
        };
        for (auto const & _p : addr2name) {
            for (auto index = 0; index < _p.second; ++index) {
                std::string address = _p.first + "@" + std::to_string(index);
                account_vec.emplace_back(address);
            }
        }
        return account_vec;
    }

    void query_all_sync_result(std::vector<std::string> const & accounts_vec, bool is_table) {
        json result_json;
        uint32_t thread_num = 8;
        if (accounts_vec.size() < thread_num) {
            for(auto const & account : accounts_vec) {
                json j;
                query_sync_result(account, j);
                result_json[account] = j;
            }
        } else {
            uint32_t address_per_thread = accounts_vec.size() / thread_num;
            std::vector<std::vector<std::string>> accounts_vec_split;
            std::vector<json> j_vec(thread_num);
            for (size_t i = 0; i < thread_num; i++) {
                uint32_t start_index = i * address_per_thread;
                uint32_t end_index = (i == (thread_num - 1)) ? accounts_vec.size() : ((i + 1) * address_per_thread);
                std::vector<std::string> thread_address;
                for (auto j = start_index; j < end_index; j++) {
                    thread_address.emplace_back(accounts_vec[j]);
                }
                accounts_vec_split.emplace_back(thread_address);
            }
            auto thread_helper = [&accounts_vec_split, &j_vec](db_export_tools *arg, int index) {
                for (auto const & account : accounts_vec_split[index]) {
                    json j;
                    arg->query_sync_result(account, j);
                    j_vec[index][account] = j;
                    std::cout << account << " block sync check finish: " << j.get<std::string>() << std::endl;
                }
            };
            std::vector<std::thread> all_thread;
            for (auto i = 0U; i < thread_num; i++) {
                std::thread th(thread_helper, this, i);
                all_thread.emplace_back(std::move(th));
            }
            for (auto i = 0U; i < thread_num; i++) {
                all_thread[i].join();
            }
            for (auto const & j : j_vec) {            
                for (auto acc = j.begin(); acc != j.end(); acc++) {
                    result_json[acc.key()] = acc.value();
                }
            }
        }

        std::string filename;
        if (accounts_vec.size() == 1) {
            filename = accounts_vec[0] + "_sync_result.json";
        } else {
            if (is_table) {
                filename = "all_table_sync_result.json";
            } else {
                filename = "all_unit_sync_result.json";
            }
        }
        std::ofstream out_json(filename);
        out_json << std::setw(4) << result_json;
        std::cout << "===> " << filename << " generated success!" << std::endl;
    }

    void query_table_latest_fullblock() {
        json result_json;
        auto const & account_vec = get_all_table_account();
        for (auto const & _p : account_vec) {
            query_table_latest_fullblock(_p, result_json[_p]);
        }
        std::string filename = "all_latest_fullblock_info.json";
        std::ofstream out_json(filename);
        out_json << std::setw(4) << result_json;
        std::cout << "===> " << filename << " generated success!" << std::endl;
    }

    void query_table_tx_info(std::vector<std::string> const & address_vec) {
        auto query_and_make_file = [](db_export_tools *arg, std::string account) {
            json result_json;
            arg->query_table_tx_info(account, result_json);
            std::string filename = "./all_table_tx_info/" + account + "_tx_info.json";
            std::ofstream out_json(filename);
            out_json << std::setw(4) << result_json[account];
            std::cout << "===> " << filename << " generated success!" << std::endl; 
        };
        mkdir("all_table_tx_info", 0750);
        uint32_t thread_num = 8;
        if (address_vec.size() < thread_num) {
            query_and_make_file(this, address_vec[0]);
        } else {
            std::vector<std::vector<std::string>> address_vec_split;
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
            auto thread_helper = [&query_and_make_file, &address_vec_split](db_export_tools *arg, int index) {
                for (auto const & _p : address_vec_split[index]) {
                    query_and_make_file(arg, _p);
                }
            };
            std::vector<std::thread> all_thread;
            int finish_num = 0;
            for (auto i = 0U; i < thread_num; i++) {
                std::thread th(thread_helper, this, i);
                all_thread.emplace_back(std::move(th));
            }
            for (auto i = 0U; i < thread_num; i++) {
                all_thread[i].join();
            }
        }
    }

    void query_block_num() {
        json j;
        uint64_t total_table_block_num{0};
        uint64_t total_unit_block_num{0};
        j["total_table_block_num"] = 0;
        j["total_unit_block_num"] = 0;
        auto const & account_vec = get_all_table_account();
        for (auto const & account : account_vec) {
            auto latest_block = m_blockstore->get_latest_committed_block(account);
            if (latest_block == nullptr) {
                std::cout << account << " get_latest_committed_block null!" << std::endl;
                return;
            }
            j[account]["table_block_num"] = latest_block->get_height() + 1;
            total_table_block_num += latest_block->get_height() + 1;

            int unit_block_num{0};
            base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(latest_block.get());
            if (bstate == nullptr) {
                std::cout << account << " get_block_state null!" << std::endl;
                std::set<std::string> account_set;
                query_unit_account(account, account_set);
                for (auto const & unit : account_set) {
                    unit_block_num += (m_blockstore->get_latest_committed_block_height(unit) + 1);
                }
                j[account]["unit_block_num"] = 0;
                j[account]["bstate"] = "null";
            } else {
                auto state = std::make_shared<xtable_bstate_t>(bstate.get());
                std::set<std::string> account_set = state->get_all_accounts();
                for (auto const & unit : account_set) {
                    base::xaccount_index_t index;
                    state->get_account_index(unit, index);
                    unit_block_num += (index.get_latest_unit_height() + 1);
                }
                j[account]["unit_block_num"] = unit_block_num;
                j[account]["bstate"] = "ok";
            }
            total_unit_block_num += unit_block_num;
        }
        j["total_table_block_num"] = total_table_block_num;
        j["total_unit_block_num"] = total_unit_block_num;
        std::string filename = "all_block_info.json";
        std::ofstream out_json(filename);
        out_json << std::setw(4) << j;
        std::cout << "===> " << filename << " generated success!" << std::endl;
    }

    void query_block_exist(std::string const & address, const uint64_t height) {
        auto const & vblock = m_blockstore->load_block_object(address, height);
        auto const & block_vec = vblock.get_vector();
        bool exist = false;
        for (auto const & item : block_vec) {
            if (item != nullptr) {
                exist = true;
                break;
            }
        }
        if (!exist) {
            std::cout << "account: " << address << " , height: " << height << " , block not exist" << std::endl;
        } else {
            std::cout << "account: " << address << " , height: " << height << " , block exist, total num: " << block_vec.size() << std::endl;
            for (auto const item : block_vec) {
                if (item != nullptr) {
                    std::cout << item->dump2() << std::endl;
                } else {
                    std::cout << "account: " << address << " , height: " << height << " , found one null block!!!" << std::endl;
                }
            }
        }
    }

    void query_block_info(std::string const & account, const int64_t height) {
        xJson::Value root;
        uint64_t h;
        // data::xblock_t * bp = nullptr;
        if (height  < 0) {
            h = m_blockstore->get_latest_committed_block_height(base::xvaccount_t{account});
            std::cout << "account: " << account << ", latest committed height: " << h << ", block info:" << std::endl;
        } else {
            h = height;
            std::cout << "account: " << account << ", height: " << h << ", block info:" << std::endl;
        }
        auto vblock = m_blockstore->load_block_object(account, h, 0, true);
        data::xblock_t * bp = dynamic_cast<data::xblock_t *>(vblock.get());
        if (bp == nullptr) {
            std::cout << "account: " << account << ", height: " << h << " block null" << std::endl;
            return;
        }
        if (bp->is_genesis_block() && bp->get_block_class() == base::enum_xvblock_class_nil && false == bp->check_block_flag(base::enum_xvblock_flag_stored)) {
            std::cout << "account: " << account << ", height: " << h << " block genesis && nil && non-stored" << std::endl;
            return;
        }
        if (false == base::xvchain_t::instance().get_xblockstore()->load_block_input(base::xvaccount_t(bp->get_account()), bp)) {
            std::cout << "account: " << account << ", height: " << h << " load_block_input failed" << std::endl;
            return;
        }
        root = dynamic_cast<chain_info::get_block_handle*>(m_getblock.get())->get_block_json(bp);
        std::string str = xJson::FastWriter().write(root);
        std::cout << str << std::endl;
    }

    void query_contract_property(std::string const & account, std::string const & prop_name, std::string const & param) {
        auto const latest_height = m_blockstore->get_latest_committed_block_height(account);

        xJson::Value jph;
        if (param == "last") {
            query_contract_property(account, prop_name, latest_height, jph);
        } else if (param == "all") {
            for (uint64_t i = 0; i < latest_height; i++) {
                query_contract_property(account, prop_name, latest_height, jph["height " + xstring_utl::tostring(i)]);
            }
        } else {
            return;
        }
        std::string filename = account + "_" + prop_name + "_" + param + "_property.json";
        std::ofstream out_json(filename);
        out_json << std::setw(4) << jph;
        std::cout << "===> " << filename << " generated success!" << std::endl;
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

private:
    void gen_all_unit_account_file(std::set<std::string> & accounts_set) {
        json accounts_json;
        std::cout << "all_account.json generating..." << std::endl;
        auto const & table_vec = get_all_table_account();
        for (auto const & account : table_vec) {
            std::set<std::string> tmp_set;
            this->query_unit_account(account, tmp_set);
            for (auto & s : tmp_set) {
                accounts_set.insert(s);
                accounts_json[account].push_back(s);
            }
        }

        std::ofstream out_json("all_account.json");
        out_json << std::setw(4) << accounts_json;
        std::cout << "===> all_account.json generated success!" << std::endl;
    }

    void gen_all_unit_account_file2(std::set<std::string> & accounts_set) {
        std::cout << "all_account.json generating..." << std::endl;
        
        auto const & table_vec = get_all_table_account();
        uint32_t thread_num = 8;
        uint32_t address_per_thread = table_vec.size() / thread_num;
        std::vector<std::vector<std::string>> table_vec_split;
        std::vector<json> j_vec(thread_num);
        for (size_t i = 0; i < thread_num; i++) {
            uint32_t start_index = i * address_per_thread;
            uint32_t end_index = (i == (thread_num - 1)) ? table_vec.size() : ((i + 1) * address_per_thread);
            std::vector<std::string> thread_address;
            for (auto j = start_index; j < end_index; j++) {
                thread_address.emplace_back(table_vec[j]);
            }
            table_vec_split.emplace_back(thread_address);
        }
        auto thread_helper = [&table_vec_split, &j_vec](db_export_tools *arg, int index) {
            for (auto const & account : table_vec_split[index]) {
                std::set<std::string> tmp_set;
                arg->query_unit_account2(account, tmp_set);
                for (auto & s : tmp_set) {
                    j_vec[index][account].push_back(s);
                }
            }
        };
        std::vector<std::thread> all_thread;
        for (auto i = 0U; i < thread_num; i++) {
            std::thread th(thread_helper, this, i);
            all_thread.emplace_back(std::move(th));
        }
        for (auto i = 0U; i < thread_num; i++) {
            all_thread[i].join();
        }
        json accounts_json;
        for (auto const & j : j_vec) {            
            for (auto table = j.begin(); table != j.end(); table++) {
                for (auto const & acc : table.value()) {
                    accounts_set.insert(acc.get<std::string>());
                }
                accounts_json[table.key()] = table.value();
            }
        }
        std::ofstream out_json("all_account.json");
        out_json << std::setw(4) << accounts_json;
        std::cout << "===> all_account.json generated success!" << std::endl;
    }

    void query_unit_account(std::string const & account, std::set<std::string> & accounts_set) {
        auto const block_height = m_blockstore->get_latest_committed_block_height(account);
        for (uint64_t h = 0; h <= block_height; ++h) {
            auto vblock = m_blockstore->load_block_object(account,h,0,true);
            data::xblock_t * block = dynamic_cast<data::xblock_t *>(vblock.get());
            if (block != nullptr) {
                auto tt3 = xtime_utl::time_now_ms();
                assert(block->get_block_level() == base::enum_xvblock_level_table);
                auto const & units = block->get_tableblock_units(false);
                if (units.empty()) {
                    continue;
                }
                for (auto & unit : units) {
                    auto unit_address = unit->get_block_owner();
                    accounts_set.insert(unit_address);
                }
            }
        }
    }

    void query_unit_account2(std::string const & account, std::set<std::string> & accounts_set) {
        auto latest_block = m_blockstore->get_latest_committed_block(account);
        if (latest_block == nullptr) {
            std::cout << account << " get_latest_committed_block null!" << std::endl;
            return;
        }
        base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(latest_block.get());
        if (bstate == nullptr) {
            std::cout << account << " get_block_state null!" << std::endl;
            query_unit_account(account, accounts_set);
        } else {
            auto state = std::make_shared<xtable_bstate_t>(bstate.get());
            accounts_set = state->get_all_accounts();
        }
    }

    void query_sync_result(std::string const & account, const uint64_t h_s, const uint64_t h_e, std::string & result, int init_s = -1, int init_e = -1) {
        int start = h_s;
        int end = -1;
        if (init_s != -1) {
            start = init_s;
        }
        if (init_e != -1) {
            end = init_e;
        }
        for (uint64_t h = h_s; h <= h_e; h++) {
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
            if (h == h_e) {
                if (end != -1) {
                    if (start == end) {
                        result += std::to_string(start) + ',';
                    } else {
                        result += std::to_string(start) + '-' + std::to_string(end) + ',';
                    }
                }
            }
        }
    }

    void query_sync_result(std::string const & account, json & result_json) {
        auto const block_height = m_blockstore->get_latest_committed_block_height(account);
        auto const connect_height = m_blockstore->get_latest_genesis_connected_block_height(account);
        if (connect_height > block_height) {
            std::cout << account << " connect_height: " << connect_height << " > block_height: " << block_height << ", error" << std::endl;
            result_json = "error";
            return;
        }
        if (block_height == connect_height) {
            result_json = "0-" + xstring_utl::tostring(block_height) + ",";
        } else {
            std::string result;
            query_sync_result(account, connect_height + 1, block_height, result, 0, connect_height);
            result_json = result;
        }
    }

    void query_table_latest_fullblock(std::string const & account, json & j) {
        auto vblock = m_blockstore->get_latest_committed_full_block(account);
        data::xblock_t * block = dynamic_cast<data::xblock_t *>(vblock.get());
        if (block == nullptr) {
            std::cout << " table " << account << " get_latest_committed_full_block null" << std::endl;
            return;
        }

        auto height_full = block->get_height();
        auto height_commit = m_blockstore->get_latest_committed_block_height(account);
        if (!block->is_fulltable() && height_full != 0) {
            std::cout << " table " << account << " latest_committed_full_block is not full table" << std::endl;
            return;
        }
        j["last_committed_block"]["height"] = height_commit;
        j["last_full_block"]["height"] = height_full;
        if (height_full != 0) {
            j["last_full_block"]["hash"] = to_hex_str(block->get_fullstate_hash());
            base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(block);
            if (bstate == nullptr) {
                j["last_full_block"]["bstate"] = "null";
            } else {
                data::xtablestate_ptr_t tablestate = std::make_shared<data::xtable_bstate_t>(bstate.get());
                j["last_full_block"]["bstate"] = "ok";
                j["last_full_block"]["bstate_account_size"] = tablestate->get_account_size();
            }
        }
        std::string s;
        query_sync_result(account, height_full, height_commit, s);
        j["exist_block"] = s;
    }
    
    void query_table_tx_info(std::string const & account, json & result_json) {
        auto const block_height = m_blockstore->get_latest_committed_block_height(account);
        std::map<std::string, tx_ext_t> send;
        std::map<std::string, tx_ext_t> confirm;
        std::map<std::string, tx_ext_t> self;
        std::set<std::string> multi_tx;
        std::vector<tx_ext_t> multi;
        json j;
        int empty_table_block_num{0};
        int light_table_block_num{0};
        int full_table_block_num{0};
        int missing_table_block_num{0};
        int empty_unit_block_num{0};
        int light_unit_block_num{0};
        int full_unit_block_num{0};
        int total_unit_block_num{0};
        int recv_num{0};
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
        auto t1 = xtime_utl::time_now_ms();
        base::xvaccount_t _vaccount(account);
        for (uint64_t h = 0; h <= block_height; h++) {
            auto const & vblock = m_blockstore->load_block_object(account,h,0,false);
            const data::xblock_t * block = dynamic_cast<data::xblock_t *>(vblock.get());
            if (block == nullptr) {
                missing_table_block_num++;
                std::cout << account << " missing block at height " << h << std::endl;
                continue;
            }

            if (block->get_block_class() == base::enum_xvblock_class_nil) {
                empty_table_block_num++;
                continue;
            } else if (block->get_block_class() == base::enum_xvblock_class_full) {
                full_table_block_num++;
                continue;
            } else {
                light_table_block_num++;
            }
            m_blockstore->load_block_input(_vaccount, vblock.get());
            assert(block->get_block_level() == base::enum_xvblock_level_table);
            const uint64_t timestamp = block->get_timestamp();
            const std::vector<base::xventity_t*> & _table_inentitys = block->get_input()->get_entitys();
            uint32_t entitys_count = _table_inentitys.size();
            for (uint32_t index = 1; index < entitys_count; index++) {  // unit entity from index#1
                total_unit_block_num++;
                base::xvinentity_t* _table_unit_inentity = dynamic_cast<base::xvinentity_t*>(_table_inentitys[index]);
                base::xtable_inentity_extend_t extend;
                extend.serialize_from_string(_table_unit_inentity->get_extend_data());
                const xobject_ptr_t<base::xvheader_t> & _unit_header = extend.get_unit_header();

                if (_unit_header->get_block_class() == base::enum_xvblock_class_nil) {
                    empty_unit_block_num++;
                    continue;
                } else if (_unit_header->get_block_class() == base::enum_xvblock_class_full) {
                    full_unit_block_num++;
                    continue;
                } else {
                    light_unit_block_num++;
                }                
                
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
                    auto type = txaction.get_tx_subtype();
                    if (type == enum_transaction_subtype_self) {
                        if (self.count(tx_ext.hash)) {
                            multi_tx.insert(tx_ext.hash);
                            multi.push_back(self[tx_ext.hash]);
                            multi.push_back(tx_ext);
                            self.erase(tx_ext.hash);
                        } else if (multi_tx.count(tx_ext.hash)) {
                            multi.push_back(tx_ext);
                        } else {
                            self[tx_ext.hash] = tx_ext;
                        }
                    } else if (type == enum_transaction_subtype_send) {
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
                    } else if (type == enum_transaction_subtype_confirm) {
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
                    } else if (type == enum_transaction_subtype_recv) {
                        recv_num++;
                    } 
                    else {
                        continue;
                    }
                }
            }
        }
        j["table info"]["table height"] = block_height;
        j["table info"]["total table block num"] = block_height + 1;
        j["table info"]["miss table block num"] = missing_table_block_num;
        j["table info"]["empty table block num"] = empty_table_block_num;
        j["table info"]["light table block num"] = light_table_block_num;
        j["table info"]["full table block num"] = full_table_block_num;
        j["table info"]["total unit block num"] = total_unit_block_num;
        j["table info"]["empty unit block num"] = empty_unit_block_num;
        j["table info"]["light unit block num"] = light_unit_block_num;
        j["table info"]["full unit block num"] = full_unit_block_num;
        j["table info"]["total self num"] = self.size();
        j["table info"]["total send num"] = send.size();
        j["table info"]["total recv num"] = recv_num;
        j["table info"]["total confirm num"] = confirm.size();
        // process tx map
        auto t2 = xtime_utl::time_now_ms();
        std::vector<tx_ext_t> sendv;
        std::vector<tx_ext_t> confirmv;
        std::vector<tx_ext_t> selfv;
        std::vector<std::pair<tx_ext_t, tx_ext_t>> confirmedv;
        auto cmp = [](tx_ext_t lhs, tx_ext_t rhs) {
            return lhs.height < rhs.height;
        };
        auto map_value_to_vec = [](std::map<std::string, tx_ext_t> & map, std::vector<tx_ext_t> & vec) {
            for (auto const & item : map) {
                vec.push_back(item.second);
            }
        };
        auto setj = [](std::vector<tx_ext_t> & vec) -> json{
            json j;
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
            return j;
        };
        auto set_confirmedj = [](std::vector<std::pair<tx_ext_t, tx_ext_t>> & vec,
                                 json & j,
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
        auto t3 = xtime_utl::time_now_ms();
        map_value_to_vec(self, selfv);
        map_value_to_vec(send, sendv);
        map_value_to_vec(confirm, confirmv);
        std::sort(selfv.begin(), selfv.end(), cmp);
        std::sort(sendv.begin(), sendv.end(), cmp);
        std::sort(confirmv.begin(), confirmv.end(), cmp);

        uint32_t total_confirm_time = 0;
        uint32_t max_confirm_time = 0;
        json tx_confirmed;
        set_confirmedj(confirmedv, tx_confirmed, total_confirm_time, max_confirm_time);
        
        j["confirmed conut"] = confirmedv.size();
        j["send only count"] = sendv.size();
        j["confirmed only count"] = confirmv.size();
        j["confirmed total time"] = total_confirm_time;
        j["confirmed max time"] = max_confirm_time;
        j["confirmed avg time"] = float(total_confirm_time) / confirmedv.size();
        j["confirmed detail"] = tx_confirmed;
        j["self detail"] = setj(selfv);
        j["send only detail"] = setj(sendv);
        j["confirmed only detail"] = setj(confirmv);
        j["multi detail"] = setj(multi);
        result_json[account] = j;
        auto t4 = xtime_utl::time_now_ms();
        // std::cout << t2-t1 << " " << t3-t2 << " " << t4-t3 << std::endl;
    }

    void query_contract_property(std::string const & account, std::string const & prop_name, uint64_t height, xJson::Value & jph) {
        static std::set<std::string> property_names = {
            XPROPERTY_CONTRACT_ELECTION_EXECUTED_KEY,
            XPROPERTY_CONTRACT_STANDBYS_KEY,
            XPROPERTY_CONTRACT_GROUP_ASSOC_KEY,
            xstake::XPORPERTY_CONTRACT_GENESIS_STAGE_KEY,
            xstake::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_YEARLY,
            xstake::XPORPERTY_CONTRACT_REG_KEY,
            xstake::XPORPERTY_CONTRACT_TICKETS_KEY,
            xstake::XPORPERTY_CONTRACT_WORKLOAD_KEY,
            xstake::XPORPERTY_CONTRACT_VALIDATOR_WORKLOAD_KEY,
            xstake::XPORPERTY_CONTRACT_TASK_KEY,
            xstake::XPORPERTY_CONTRACT_VOTES_KEY1,
            xstake::XPORPERTY_CONTRACT_VOTES_KEY2,
            xstake::XPORPERTY_CONTRACT_VOTES_KEY3,
            xstake::XPORPERTY_CONTRACT_VOTES_KEY4,
            xstake::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY1,
            xstake::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY2,
            xstake::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY3,
            xstake::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY4,
            xstake::XPORPERTY_CONTRACT_NODE_REWARD_KEY,
            xstake::XPORPERTY_CONTRACT_REFUND_KEY,
            xstake::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE,
            xstake::XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY,
            xstake::XPROPERTY_CONTRACT_SLASH_INFO_KEY,
            PROPOSAL_MAP_ID,
            VOTE_MAP_ID
        };

        auto is_special_contract_property = [](std::string const & prop_name) -> bool {
            auto iter = property_names.find(prop_name);
            if (iter != property_names.end()) {
                return true;
            }
            if (prop_name.size() > 3 && XPROPERTY_CONTRACT_ELECTION_RESULT_KEY == prop_name.substr(0, 3)) {
                return true;
            }
            return false;
        };

        base::xvaccount_t _vaddr(account);
        auto _block = m_blockstore->load_block_object(_vaddr, height, 0, false);
        if (_block == nullptr) {
            std::cout << account << " height " << height << " block null!" << std::endl;
            return;
        }
        if (_block->is_genesis_block() && _block->get_block_class() == base::enum_xvblock_class_nil) {
            std::cout << account << " height " << height << " block genesis or nil!" << std::endl;
            return;
        }
        base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(_block.get());
        if (bstate == nullptr) {
            std::cout << account << " height " << height << " bstate null!" << std::endl;
            return;
        }
        xaccount_ptr_t unitstate = std::make_shared<xunit_bstate_t>(bstate.get());
        if (unitstate == nullptr) {
            std::cout << account << " height " << height << " unitstate null!" << std::endl;
            return;
        }
        if (false == unitstate->get_bstate()->find_property(prop_name)) {
            std::cout << account << " height " << height << " fail-find property " << prop_name  << "!" << std::endl;
            return;
        }
        if (is_special_contract_property(prop_name)) {
            top::contract::xcontract_manager_t::instance().get_contract_data(top::common::xaccount_address_t{account}, unitstate, prop_name, top::contract::xjson_format_t::detail, jph);
        } else {
            base::xvproperty_t* propobj = unitstate->get_bstate()->get_property_object(prop_name);
            if (propobj->get_obj_type() == base::enum_xobject_type_vprop_string_map) {
                auto propobj_map = unitstate->get_bstate()->load_string_map_var(prop_name);
                auto values = propobj_map->query();
                xJson::Value j;
                for (auto & v : values) {
                    j[v.first] = v.second;
                }
                jph[prop_name] = j;
            } else if (propobj->get_obj_type() == base::enum_xobject_type_vprop_string) {
                auto propobj_str = unitstate->get_bstate()->load_string_var(prop_name);
                jph[prop_name] = propobj_str->query();
            } else if (propobj->get_obj_type() == base::enum_xobject_type_vprop_string_deque) {
                auto propobj_deque = unitstate->get_bstate()->load_string_deque_var(prop_name);
                auto values = propobj_deque->query();
                for (auto & v : values) {
                    jph[prop_name].append(v);
                }
            } else if (propobj->get_obj_type() == base::enum_xobject_type_vprop_token) {
                auto propobj = unitstate->get_bstate()->load_token_var(prop_name);
                base::vtoken_t balance = propobj->get_balance();
                jph[prop_name] = std::to_string(balance);
            } else if (propobj->get_obj_type() == base::enum_xobject_type_vprop_uint64) {
                auto propobj = unitstate->get_bstate()->load_uint64_var(prop_name);
                uint64_t value = propobj->get();
                jph[prop_name] = std::to_string(value);
            }
        }
    }

    common::xnode_id_t m_node_id;
    std::string m_sign_key;
    xobject_ptr_t<mbus::xmessage_bus_face_t> m_bus;
    xobject_ptr_t<store::xstore_face_t> m_store;
    xobject_ptr_t<base::xvblockstore_t> m_blockstore;
    xobject_ptr_t<base::xvnodesrv_t> m_nodesvr_ptr;
    std::shared_ptr<rpc::xrpc_handle_face_t> m_getblock;
};

int main(int argc, char ** argv) {
    auto hash_plugin = new xtop_hash_t();

    if (argc < 3) {
        usage();
        return -1;
    }

    std::string db_path;
    {
        std::string config_file{argv[1]};
        if (access(config_file.c_str(), 0) != 0) {
            std::cout << "config file: " << config_file << " not found" << std::endl;
            return -1;
        }
        json confg_json;
        std::ifstream config_stream(config_file);
        config_stream >> confg_json;
        db_path = confg_json.at("db_path");
        if (access(db_path.c_str(), 0) != 0) {
            std::cout << "db: " << db_path << "not found!" << std::endl;
            return -1;
        }
    }
    // xinit_log("./xdb_export.log", true, true);
    // xset_log_level(enum_xlog_level_debug);
    // xdbg("------------------------------------------------------------------");
    // xinfo("new log start here");
    db_export_tools tools{db_path};
    std::string function_name{argv[2]};
    if (function_name == "checkout_all_account") {
        std::set<std::string> accounts_set;
        tools.get_all_unit_account(accounts_set);
        for (auto const & s : accounts_set) {
            std::cout << s << std::endl;
        }
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
        if (argc == 3) {
            auto const & table_account_vec = db_export_tools::get_all_table_account();
            auto const & unit_account_vec = tools.get_all_unit_account();
            tools.query_all_sync_result(table_account_vec, true);
            tools.query_all_sync_result(unit_account_vec, false);
        } else if (argc == 4) {
            std::string method_name{argv[3]};
            if (method_name == "table") {
                auto const & table_account_vec = db_export_tools::get_all_table_account();
                tools.query_all_sync_result(table_account_vec, true);
            } else if (method_name == "unit") {
                auto const & unit_account_vec = tools.get_all_unit_account();
                tools.query_all_sync_result(unit_account_vec, false);
            } else if (method_name == "account") {
                std::vector<std::string> account = {argv[4]};
                tools.query_all_sync_result(account, false);
            }else {
                usage();
                return -1;
            }
        }
    } else if (function_name == "check_tx_info") {
        if (argc == 3) {
            auto const & account_vec = db_export_tools::get_all_table_account();
            tools.query_table_tx_info(account_vec);
        } else if (argc == 4) {
            std::vector<std::string> account = {argv[3]};
            tools.query_table_tx_info(account);
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
    } else if (function_name == "check_block_info") {
        if (argc < 5) {
            usage();
            return -1;
        }
        int height = -1;
        if (std::string{argv[4]} != std::string{"last"}) {
            height = std::stoi(argv[4]);
        }
        tools.query_block_info(argv[3], height);
    } else if (function_name == "check_latest_fullblock") {
        tools.query_table_latest_fullblock();
    } else if (function_name == "check_contract_property") {
        if (argc < 6) {
            usage();
            return -1;
        }
        std::string param{argv[5]};
        if (param != "last" && param != "all") {
            usage();
            return -1;
        }
        tools.query_contract_property(argv[3], argv[4], param);
    } else {
        usage();
    }
    return 0;
}
