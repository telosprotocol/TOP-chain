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
#include "xdata/xslash.h"
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

#define NODE_ID "T00000LgGPqEpiK6XLCKRj9gVPN8Ej1aMbyAb3Hu"
#define SIGN_KEY "ONhWC2LJtgi9vLUyoa48MF3tiXxqWf7jmT9KtOg/Lwo="

class xtop_hash_t;
class db_reset_t;
class db_export_tools;

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

class db_export_tools {
public:
    friend class db_reset_t;
    db_export_tools(std::string const & db_path) {
        XMETRICS_INIT();
        top::config::config_register.get_instance().set(config::xmin_free_gas_asset_onchain_goverance_parameter_t::name, std::to_string(ASSET_TOP(100)));
        top::config::config_register.get_instance().set(config::xfree_gas_onchain_goverance_parameter_t::name, std::to_string(25000));
        top::config::config_register.get_instance().set(config::xmax_validator_stake_onchain_goverance_parameter_t::name, std::to_string(5000));
        top::config::config_register.get_instance().set(config::xchain_name_configuration_t::name, std::string{top::config::chain_name_testnet});
        top::config::config_register.get_instance().set(config::xroot_hash_configuration_t::name, std::string{});
        data::xrootblock_para_t para;
        data::xrootblock_t::init(para);
        m_bus = top::make_object_ptr<mbus::xmessage_bus_t>(true, 1000);
        m_store = top::store::xstore_factory::create_store_with_kvdb(db_path);
        base::xvchain_t::instance().set_xdbstore(m_store.get());
        base::xvchain_t::instance().set_xevmbus(m_bus.get());
        m_blockstore.attach(store::get_vblockstore());
        m_nodesvr_ptr = make_object_ptr<xvnode_house_t>(common::xnode_id_t{NODE_ID}, SIGN_KEY, m_blockstore, make_observer(m_bus.get()));
        m_getblock = std::make_shared<chain_info::get_block_handle>(m_store.get(), m_blockstore.get(), nullptr);
        contract::xcontract_manager_t::instance().init(make_observer(m_store), xobject_ptr_t<store::xsyncvstore_t>{});
        contract::xcontract_manager_t::set_nodesrv_ptr(m_nodesvr_ptr);
    }

    static std::vector<std::string> get_unit_contract_accounts() {
        std::vector<std::string> v;
        const std::vector<std::string> unit = {
            sys_contract_rec_registration_addr,
            sys_contract_rec_elect_edge_addr,
            sys_contract_rec_elect_archive_addr,
            sys_contract_rec_elect_rec_addr,
            sys_contract_rec_elect_zec_addr,
            sys_contract_rec_tcc_addr,
            sys_contract_rec_standby_pool_addr,
            sys_contract_zec_workload_addr,
            sys_contract_zec_vote_addr,
            sys_contract_zec_reward_addr,
            sys_contract_zec_slash_info_addr,
            sys_contract_zec_elect_consensus_addr,
            sys_contract_zec_standby_pool_addr,
            sys_contract_zec_group_assoc_addr,
        };
        const std::vector<std::string> table = {
            sys_contract_sharding_vote_addr,
            sys_contract_sharding_reward_claiming_addr,
        };
        for (auto const & u : unit) {
            v.emplace_back(u);
        }
        for (auto const & t : table) {
            for (auto i = 0; i < enum_vledger_const::enum_vbucket_has_tables_count; i++) {
                std::string u{t + "@" + std::to_string(i)};
                v.emplace_back(u);
            }
        }
        return v;
    }

    static std::vector<std::string> get_table_contract_accounts() {
        std::vector<std::string> v;
        const std::vector<std::pair<std::string, int>> table = {
            std::make_pair(std::string{sys_contract_sharding_table_block_addr}, enum_vledger_const::enum_vbucket_has_tables_count),
            std::make_pair(std::string{sys_contract_zec_table_block_addr}, MAIN_CHAIN_ZEC_TABLE_USED_NUM),
            std::make_pair(std::string{sys_contract_beacon_table_block_addr}, MAIN_CHAIN_REC_TABLE_USED_NUM),
        };
        for (auto const & t : table) {
            for (auto i = 0; i < t.second; i++) {
                std::string u{t.first + "@" + std::to_string(i)};
                v.emplace_back(u);
            }
        }
        return v;
    }

    std::vector<std::string> get_db_unit_accounts() {
        auto const & s = query_db_unit_accounts();
        std::vector<std::string> v;
        v.assign(s.begin(), s.end());
        return v;
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
        auto const & account_vec = get_table_contract_accounts();
        for (auto const & _p : account_vec) {
            query_table_latest_fullblock(_p, result_json[_p]);
        }
        std::string filename = "all_latest_fullblock_info.json";
        std::ofstream out_json(filename);
        out_json << std::setw(4) << result_json;
        std::cout << "===> " << filename << " generated success!" << std::endl;
    }

    void query_table_tx_info(std::vector<std::string> const & address_vec, const uint32_t start_timestamp, const uint32_t end_timestamp) {
        auto query_and_make_file = [start_timestamp, end_timestamp](db_export_tools *arg, std::string account) {
            json result_json;
            arg->query_table_tx_info(account, start_timestamp, end_timestamp, result_json);
            std::string filename = "./all_table_tx_info/" + account + "_tx_info.json";
            std::ofstream out_json(filename);
            out_json << std::setw(4) << result_json[account];
            std::cout << "===> " << filename << " generated success!" << std::endl; 
        };
        mkdir("all_table_tx_info", 0750);
        uint32_t thread_num = 3;
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
        auto const & account_vec = get_table_contract_accounts();
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

    void query_block_info(std::string const & account, std::string const & param) {
        xJson::Value root;
        if (param == "last") {
            uint64_t h = m_blockstore->get_latest_committed_block_height(base::xvaccount_t{account});
            std::cout << "account: " << account << ", latest committed height: " << h << ", block info:" << std::endl;
            query_block_info(account, h, root);
            std::string str = xJson::FastWriter().write(root);
            std::cout << str << std::endl;
        } else if (param != "all") {
            uint64_t h = std::stoi(param);
            std::cout << "account: " << account << ", height: " << h << ", block info:" << std::endl;
            query_block_info(account, h, root);
            std::string str = xJson::FastWriter().write(root);
            std::cout << str << std::endl;
        } else {
            uint64_t h = m_blockstore->get_latest_committed_block_height(base::xvaccount_t{account});
            for (size_t i = 0; i <= h; i++) {
                xJson::Value j;
                query_block_info(account, i, j);
                root["height" + std::to_string(i)] = j;
            }
            std::string filename = account + "_all_block_info.json";
            std::ofstream out_json(filename);
            out_json << std::setw(4) << root;
            std::cout << "===> " << filename << " generated success!" << std::endl;
        }
    }

    void query_block_basic(std::string const & account, std::string const & param) {
        json root;
        if (param == "last") {
            auto vblock = m_blockstore->get_latest_cert_block(base::xvaccount_t{account});
            if (vblock == nullptr) {
                std::cout << "account: " << account << ", latest cert block nullptr!" << std::endl;
                return;
            }
            uint64_t h = vblock->get_height();
            std::cout << "account: " << account << ", latest cert height: " << h << ", block info:" << std::endl;
            query_block_basic(account, h, root);
            std::cout << root << std::endl;
        } else if (param != "all") {
            uint64_t h = std::stoi(param);
            std::cout << "account: " << account << ", height: " << h << ", block info:" << std::endl;
            query_block_basic(account, h, root);
            std::cout << root << std::endl;
        } else {
            auto vblock = m_blockstore->get_latest_cert_block(base::xvaccount_t{account});
            if (vblock == nullptr) {
                std::cout << "account: " << account << ", latest cert block nullptr!" << std::endl;
                return;
            }
            uint64_t h = vblock->get_height();
            for (size_t i = 0; i <= h; i++) {
                json j;
                query_block_basic(account, i, j);
                root["height" + std::to_string(i)] = j;
            }
            std::string filename = account + "_all_block_basic.json";
            std::ofstream out_json(filename);
            out_json << std::setw(4) << root;
            std::cout << "===> " << filename << " generated success!" << std::endl;
        }
    }

    void query_state_basic(std::string const & account, std::string const & param) {
        json root;
        if (param == "last") {
            auto vblock = m_blockstore->get_latest_cert_block(base::xvaccount_t{account});
            if (vblock == nullptr) {
                std::cout << "account: " << account << ", latest cert block nullptr!" << std::endl;
                return;
            }
            uint64_t h = vblock->get_height();
            std::cout << "account: " << account << ", latest cert height: " << h << ", state info:" << std::endl;
            query_state_basic(account, h, root);
            std::cout << root << std::endl;
        } else if (param != "all") {
            uint64_t h = std::stoi(param);
            std::cout << "account: " << account << ", height: " << h << ", state info:" << std::endl;
            query_state_basic(account, h, root);
            std::cout << root << std::endl;
        } else {
            auto vblock = m_blockstore->get_latest_cert_block(base::xvaccount_t{account});
            if (vblock == nullptr) {
                std::cout << "account: " << account << ", latest cert block nullptr!" << std::endl;
                return;
            }
            uint64_t h = vblock->get_height();
            for (size_t i = 0; i <= h; i++) {
                json j;
                query_state_basic(account, i, j);
                root["height" + std::to_string(i)] = j;
            }
            std::string filename = account + "_all_state_basic.json";
            std::ofstream out_json(filename);
            out_json << std::setw(4) << root;
            std::cout << "===> " << filename << " generated success!" << std::endl;
        }
    }

    void query_contract_property(std::string const & account, std::string const & prop_name, std::string const & param) {
        auto const latest_height = m_blockstore->get_latest_committed_block_height(account);

        xJson::Value jph;
        if (param == "last") {
            query_contract_property(account, prop_name, latest_height, jph);
        } else if (param == "all") {
            for (uint64_t i = 0; i <= latest_height; i++) {
                query_contract_property(account, prop_name, i, jph["height " + xstring_utl::tostring(i)]);
            }
        } else {
            return;
        }
        std::string filename = account + "_" + prop_name + "_" + param + "_property.json";
        std::ofstream out_json(filename);
        out_json << std::setw(4) << jph;
        std::cout << "===> " << filename << " generated success!" << std::endl;
    }

private:
    std::set<std::string> query_db_unit_accounts() {
        std::set<std::string> accounts;
        std::ifstream file_stream("all_account.json");
        json j;
        file_stream >> j;
        if (!j.empty()) {
            for (auto _table : j) {
                for (auto _acc : _table) {
                    accounts.insert(_acc.get<std::string>());
                }
            }
        } else {
            generate_db_unit_accounts_file_v2(accounts);
        }
        return accounts;
    }

    std::set<std::string> generate_db_unit_accounts_file() {
        std::set<std::string> accounts;
        json j;
        std::cout << "all_account.json generating..." << std::endl;
        auto const & tables = get_table_contract_accounts();
        for (auto const & table : tables) {
            std::set<std::string> units;
            query_unit_account(table, units);
            for (auto const & unit : units) {
                accounts.insert(unit);
                j[table].push_back(unit);
            }
        }

        std::ofstream file_stream("all_account.json");
        file_stream << std::setw(4) << j;
        std::cout << "===> all_account.json generated success!" << std::endl;
        return accounts;
    }

    void generate_db_unit_accounts_file_v2(std::set<std::string> & accounts_set) {
        std::cout << "all_account.json generating..." << std::endl;
        
        auto const & table_vec = get_table_contract_accounts();
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
    
    void query_table_tx_info(std::string const & account, const uint32_t start_timestamp, const uint32_t end_timestamp, json & result_json) {
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
        uint32_t tx_v1_num = 0;
        uint64_t tx_v1_total_size = 0;
        uint32_t tx_v2_num = 0;
        uint64_t tx_v2_total_size = 0;
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
            if (timestamp < start_timestamp || timestamp > end_timestamp) {
                continue;
            }
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
                    auto tx_size = block->query_tx_size(txaction.get_tx_hash());
                    auto tx_ptr = block->query_raw_transaction(txaction.get_tx_hash());
                    if (tx_size > 0) {
                        if (tx_ptr != nullptr) {
                            if (tx_ptr->get_tx_version() == 2) {
                                tx_v2_num++;
                                tx_v2_total_size += tx_size;
                            } else {
                                tx_v1_num++;
                                tx_v1_total_size += tx_size;
                            }
                        }
                    }
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
        j["table info"]["total tx v1 num"] = tx_v1_num;
        j["table info"]["total tx v1 size"] = tx_v1_total_size;
        if (tx_v1_num != 0) {
            j["table info"]["tx v1 avg size"] = tx_v1_total_size / tx_v1_num;
        }
        j["table info"]["total tx v2 num"] = tx_v2_num;
        j["table info"]["total tx v2 size"] = tx_v2_total_size;
        if (tx_v2_num != 0) {
            j["table info"]["tx v2 avg size"] = tx_v2_total_size / tx_v2_num;
        }
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

    void query_block_info(std::string const & account, const uint64_t h, xJson::Value & root) {
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
    }

    void query_block_basic(std::string const & account, const uint64_t h, json & result) {
        auto block_vec = m_blockstore->load_block_object(account, h).get_vector();
        if (block_vec.empty()) {
            std::cout << "account: " << account << ", height: " << h << " block null" << std::endl;
            return;
        }
        for (size_t i = 0; i < block_vec.size(); i++) {
            auto const & vblock = block_vec[i];
            if (vblock == nullptr) {
                std::cout << "account: " << account << ", height: " << h << " block[" << i << "] null" << std::endl;
                continue;
            }
            if (block_vec.size() > 1) {
                std::string block_id = std::string{"block"} + std::to_string(i);
                result[block_id]["account"] = vblock->get_account();
                result[block_id]["height"] = vblock->get_height();
                result[block_id]["class"] = vblock->get_block_class();
                result[block_id]["viewid"] = vblock->get_viewid();
                result[block_id]["viewtoken"] = vblock->get_viewtoken();
                result[block_id]["clock"] = vblock->get_clock();
                result[block_id]["hash"] = xstring_utl::to_hex(vblock->get_block_hash());
                result[block_id]["last_hash"] = xstring_utl::to_hex(vblock->get_last_block_hash());
                
            } else {
                result["account"] = vblock->get_account();
                result["height"] = vblock->get_height();
                result["class"] = vblock->get_block_class();
                result["viewid"] = vblock->get_viewid();
                result["viewtoken"] = vblock->get_viewtoken();
                result["clock"] = vblock->get_clock();
                result["hash"] = xstring_utl::to_hex(vblock->get_block_hash());
                result["last_hash"] = xstring_utl::to_hex(vblock->get_last_block_hash());
            }
        }
    }

    void query_state_basic(std::string const & account, const uint64_t h, json & result) {
        auto block_vec = m_blockstore->load_block_object(account, h).get_vector();
        if (block_vec.empty()) {
            std::cout << "account: " << account << ", height: " << h << " block null" << std::endl;
            return;
        }
        for (size_t i = 0; i < block_vec.size(); i++) {
            auto const & vblock = block_vec[i];
            if (vblock == nullptr) {
                std::cout << "account: " << account << ", height: " << h << " block[" << i << "] null" << std::endl;
                continue;
            }
            auto bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(vblock);
            if (bstate == nullptr) {
                std::cout << "account: " << account << ", height: " << h << " state null" << std::endl;
                continue;
            }
            if (block_vec.size() > 1) {
                std::string block_id = std::string{"block"} + std::to_string(i);
                result[block_id]["account"] = bstate->get_account();
                result[block_id]["height"] = bstate->get_block_height();
                result[block_id]["class"] = bstate->get_block_class();
                result[block_id]["viewid"] = bstate->get_block_viewid();
                result[block_id]["last_hash"] = xstring_utl::to_hex(bstate->get_last_block_hash());
            } else {
                result["account"] = bstate->get_account();
                result["height"] = bstate->get_block_height();
                result["class"] = bstate->get_block_class();
                result["viewid"] = bstate->get_block_viewid();
                result["last_hash"] = xstring_utl::to_hex(bstate->get_last_block_hash());
            }
        }
    }

    void query_contract_property(std::string const & account, std::string const & prop_name, uint64_t height, xJson::Value & jph) {
        static std::set<std::string> property_names = {
            XPROPERTY_CONTRACT_ELECTION_EXECUTED_KEY,
            XPROPERTY_CONTRACT_STANDBYS_KEY,
            XPROPERTY_CONTRACT_GROUP_ASSOC_KEY,
            XPORPERTY_CONTRACT_GENESIS_STAGE_KEY,
            XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_YEARLY,
            XPORPERTY_CONTRACT_REG_KEY,
            XPORPERTY_CONTRACT_TICKETS_KEY,
            XPORPERTY_CONTRACT_WORKLOAD_KEY,
            XPORPERTY_CONTRACT_VALIDATOR_WORKLOAD_KEY,
            XPORPERTY_CONTRACT_TASK_KEY,
            XPORPERTY_CONTRACT_VOTES_KEY1,
            XPORPERTY_CONTRACT_VOTES_KEY2,
            XPORPERTY_CONTRACT_VOTES_KEY3,
            XPORPERTY_CONTRACT_VOTES_KEY4,
            XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY1,
            XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY2,
            XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY3,
            XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY4,
            XPORPERTY_CONTRACT_NODE_REWARD_KEY,
            XPORPERTY_CONTRACT_REFUND_KEY,
            XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE,
            XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY,
            XPROPERTY_CONTRACT_SLASH_INFO_KEY,
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

    xobject_ptr_t<mbus::xmessage_bus_face_t> m_bus;
    xobject_ptr_t<store::xstore_face_t> m_store;
    xobject_ptr_t<base::xvblockstore_t> m_blockstore;
    xobject_ptr_t<base::xvnodesrv_t> m_nodesvr_ptr;
    std::shared_ptr<rpc::xrpc_handle_face_t> m_getblock;
};

class db_reset_t {
public:
    db_reset_t(db_export_tools & tools) {
        m_blockstore = tools.m_blockstore;
    }
    ~db_reset_t(){};
    void generate_reset_check_file(std::vector<std::string> const & accounts) {
        json property_json;
        get_contract_stake_property_map_string_string(property_json);
        get_contract_stake_property_string(property_json);
        get_contract_table_stake_property_map_string_string(property_json);
        get_contract_table_stake_property_string(property_json);
        get_unit_set_property(accounts, property_json);
        std::ofstream out_json("all_property_check.json");
        out_json << std::setw(4) << property_json["contract_account_parse"];
        out_json << std::setw(4) << property_json["user_account_parse"];
        std::cout << "===> all_property_check.json generated success!" << std::endl;
    }

    void verify(json const & contract, json const & user) {
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
            table_reward += decimals / xstake::REWARD_PRECISION;
            if (decimals % xstake::REWARD_PRECISION != 0) {
                table_reward += 1;
            }
            if (static_cast<uint64_t>(it->at("$0")) != 0 || table_reward != 0) {
                // std::cout << "table " << i << ", $0: " << static_cast<uint64_t>(it->at("$0")) << ", calc_table: " << table_reward << ", miss: " << abs(table_reward - static_cast<uint64_t>(it->at("$0"))) << std::endl;
            }
        }
        total_reward += total_reward_decimals / xstake::REWARD_PRECISION;
        total_reward_decimals %= xstake::REWARD_PRECISION;
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
private:
    void get_unit_set_property(std::vector<std::string> const & accounts_vec, json & accounts_json) {
        std::set<std::string> accounts_set(accounts_vec.begin(), accounts_vec.end());
        // 1. get all new sys contract address
        auto const & sys_contract_accounts_vec = db_export_tools::get_unit_contract_accounts();
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
                    base::xstream_t key{xcontext_t::instance(), (uint8_t*)v.first.data(), static_cast<uint32_t>(v.first.size())};
                    key >> duration;
                    key >> lock_time;
                    base::xstream_t val{xcontext_t::instance(), (uint8_t*)v.second.data(), static_cast<uint32_t>(v.second.size())};
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

    void get_contract_stake_property_string(json & stake_json) {
        for(auto const & _pair:stake_string_pair_list) {
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
            if (property_key == xstake::XPORPERTY_CONTRACT_GENESIS_STAGE_KEY) {
                xstake::xactivation_record record;
                if (!value.empty()) {
                    base::xstream_t stream{xcontext_t::instance(), (uint8_t *)value.data(), static_cast<uint32_t>(value.size())};
                    record.serialize_from(stream);
                }
                j["activated"] = record.activated;
                j["activation_time"] = record.activation_time;
            } else if (property_key == xstake::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_YEARLY) {
                xstake::xaccumulated_reward_record record;
                if (!value.empty()) {
                    base::xstream_t stream{xcontext_t::instance(), (uint8_t *)value.data(), static_cast<uint32_t>(value.size())};
                    record.serialize_from(stream);
                }
                j["last_issuance_time"] = record.last_issuance_time;
                j["issued_until_last_year_end"] = static_cast<uint64_t>(record.issued_until_last_year_end / xstake::REWARD_PRECISION);
                j["issued_until_last_year_end_decimals"] = static_cast<uint32_t>(record.issued_until_last_year_end % xstake::REWARD_PRECISION);
            }
            stake_json["contract_account_parse"][addr][property_key] = j;
        }
    }

    void get_contract_stake_property_map_string_string(json & stake_json) {
        for (auto const & _pair:stake_map_string_string_pair_list) {
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
            if (property_key == xstake::XPORPERTY_CONTRACT_TICKETS_KEY) {
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
            } else if (property_key == xstake::XPORPERTY_CONTRACT_REG_KEY) {
                for (auto m : value) {
                    xstake::xreg_node_info reg_node_info;
                    xstream_t stream(xcontext_t::instance(), (uint8_t *)m.second.data(), m.second.size());
                    reg_node_info.serialize_from(stream);
                    json j;
                    j["account_addr"] = reg_node_info.m_account.value();
                    j["node_deposit"] = static_cast<unsigned long long>(reg_node_info.m_account_mortgage);
                    if (reg_node_info.m_genesis_node) {
                        j["registered_node_type"] = std::string{"advance,validator,edge"};
                    } else {
                        j["registered_node_type"] = common::to_string(reg_node_info.m_registered_role);
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
            } else if (property_key == xstake::XPROPERTY_CONTRACT_SLASH_INFO_KEY) {
                for (auto const & m : value) {
                    xstake::xslash_info s_info;
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
            } else if (property_key == xstake::XPORPERTY_CONTRACT_REFUND_KEY) {
                for (auto m : value) {
                    xstake::xrefund_info refund;
                    auto detail = m.second;
                    base::xstream_t stream{xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
                    refund.serialize_from(stream);
                    json jv;
                    jv["refund_amount"] = refund.refund_amount;
                    jv["create_time"] = refund.create_time;
                    deser_res[m.first] = jv;
                }
            } else if (property_key == xstake::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE) {
                for (auto const & m : value) {
                    deser_res[m.first] = base::xstring_utl::touint64(m.second);
                }
            } else if (property_key == xstake::XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY) {
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
            } else if (property_key == xstake::XPORPERTY_CONTRACT_TASK_KEY) {
                for (auto m : value) {
                    auto const & detail = m.second;
                    if (detail.empty())
                        continue;

                    xstake::xreward_dispatch_task task;
                    base::xstream_t stream{xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
                    task.serialize_from(stream);
                    json jv;
                    json jvn;
                    int no = 0;
                    jv["task_id"] = m.first;
                    jv["onchain_timer_round"] = (xJson::UInt64)task.onchain_timer_round;
                    jv["contract"] = task.contract;
                    jv["action"] = task.action;
                    if (task.action == xstake::XREWARD_CLAIMING_ADD_NODE_REWARD || task.action == xstake::XREWARD_CLAIMING_ADD_VOTER_DIVIDEND_REWARD) {
                        base::xstream_t stream_params{xcontext_t::instance(), (uint8_t *)task.params.data(), static_cast<uint32_t>(task.params.size())};
                        uint64_t onchain_timer_round;
                        std::map<std::string, top::xstake::uint128_t> rewards;
                        stream_params >> onchain_timer_round;
                        stream_params >> rewards;

                        for (auto v : rewards) {
                            jvn[v.first] = std::to_string(static_cast<uint64_t>(v.second / xstake::REWARD_PRECISION)) + std::string(".")
                                + std::to_string(static_cast<uint32_t>(v.second % xstake::REWARD_PRECISION));
                        }

                        jv["rewards"] = jvn;
                    } else if (task.action == xstake::XTRANSFER_ACTION) {
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

    void get_contract_table_stake_property_string(json & stake_json){
        for(auto const & _pair:table_stake_string_pair_list) {
            std::string const addr = _pair.first;
            std::string const property_key = _pair.second;
            for (auto index = 0; index < enum_vledger_const::enum_vbucket_has_tables_count; ++index) {
                std::string table_addr = addr + "@" + std::to_string(index);
                json j;
                stake_json["contract_account_parse"][addr + "@" + std::to_string(index)][property_key] = j;
            }
        }
    }

    void get_contract_table_stake_property_map_string_string(json & stake_json) {
        for (auto const & _pair:table_stake_map_string_string_pair_list) {
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
                if (property_key == xstake::XPORPERTY_CONTRACT_NODE_REWARD_KEY) {
                    for (auto m : value) {
                        json jv;
                        xstake::xreward_node_record record;
                        auto detail = m.second;
                        base::xstream_t stream{xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
                        record.serialize_from(stream);
                        jv["accumulated"] = static_cast<uint64_t>(record.m_accumulated / xstake::REWARD_PRECISION);
                        jv["accumulated_decimals"] = static_cast<uint32_t>(record.m_accumulated % xstake::REWARD_PRECISION);
                        jv["unclaimed"] = static_cast<uint64_t>(record.m_unclaimed / xstake::REWARD_PRECISION);
                        jv["unclaimed_decimals"] = static_cast<uint32_t>(record.m_unclaimed % xstake::REWARD_PRECISION);
                        jv["last_claim_time"] = record.m_last_claim_time;
                        jv["issue_time"] = record.m_issue_time;
                        j[m.first] = jv;
                    }
                } else if (property_key == xstake::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY1 || property_key == xstake::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY2 ||
                        property_key == xstake::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY3 || property_key == xstake::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY4) {
                    for (auto m : value) {
                        xstake::xreward_record record;
                        auto detail = m.second;
                        base::xstream_t stream{xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
                        record.serialize_from(stream);
                        json jv;
                        jv["accumulated"] = static_cast<uint64_t>(record.accumulated / xstake::REWARD_PRECISION);
                        jv["accumulated_decimals"] = static_cast<uint32_t>(record.accumulated % xstake::REWARD_PRECISION);
                        jv["unclaimed"] = static_cast<uint64_t>(record.unclaimed / xstake::REWARD_PRECISION);
                        jv["unclaimed_decimals"] = static_cast<uint32_t>(record.unclaimed % xstake::REWARD_PRECISION);
                        jv["last_claim_time"] = record.last_claim_time;
                        jv["issue_time"] = record.issue_time;
                        json jvm;
                        int no = 0;
                        for (auto n : record.node_rewards) {
                            json jvn;
                            jvn["account_addr"] = n.account;
                            jvn["accumulated"] = static_cast<uint64_t>(n.accumulated / xstake::REWARD_PRECISION);
                            jvn["accumulated_decimals"] = static_cast<uint32_t>(n.accumulated % xstake::REWARD_PRECISION);
                            jvn["unclaimed"] = static_cast<uint64_t>(n.unclaimed / xstake::REWARD_PRECISION);
                            jvn["unclaimed_decimals"] = static_cast<uint32_t>(n.unclaimed % xstake::REWARD_PRECISION);
                            jvn["last_claim_time"] = n.last_claim_time;
                            jvn["issue_time"] = n.issue_time;
                            jvm[no++] = jvn;
                        }
                        jv["node_dividend"] = jvm;

                        j[m.first] = jv;
                    }
                } else if (property_key == xstake::XPORPERTY_CONTRACT_POLLABLE_KEY) {
                    for (auto m : value) {
                        j[m.first] = base::xstring_utl::touint64(m.second);
                    }
                } else if (property_key == xstake::XPORPERTY_CONTRACT_VOTES_KEY1 || property_key == xstake::XPORPERTY_CONTRACT_VOTES_KEY2 || 
                        property_key == xstake::XPORPERTY_CONTRACT_VOTES_KEY3 || property_key == xstake::XPORPERTY_CONTRACT_VOTES_KEY4) {
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

    top::xobject_ptr_t<base::xvblockstore_t> m_blockstore;

    std::vector<std::pair<std::string, std::string>> stake_map_string_string_pair_list = {
        std::make_pair(sys_contract_rec_registration_addr, xstake::XPORPERTY_CONTRACT_TICKETS_KEY),
        std::make_pair(sys_contract_rec_registration_addr, xstake::XPORPERTY_CONTRACT_REG_KEY),
        std::make_pair(sys_contract_rec_registration_addr, xstake::XPROPERTY_CONTRACT_SLASH_INFO_KEY),
        std::make_pair(sys_contract_rec_registration_addr, xstake::XPORPERTY_CONTRACT_REFUND_KEY),
        std::make_pair(sys_contract_zec_reward_addr, xstake::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE),
        std::make_pair(sys_contract_zec_reward_addr, xstake::XPORPERTY_CONTRACT_TASK_KEY),
        std::make_pair(sys_contract_zec_reward_addr, xstake::XPORPERTY_CONTRACT_WORKLOAD_KEY),
        std::make_pair(sys_contract_zec_reward_addr, xstake::XPORPERTY_CONTRACT_VALIDATOR_WORKLOAD_KEY),
        std::make_pair(sys_contract_zec_slash_info_addr, xstake::XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY),
        std::make_pair(sys_contract_zec_vote_addr, xstake::XPORPERTY_CONTRACT_TICKETS_KEY),
    };

    std::vector<std::pair<std::string, std::string>> stake_string_pair_list = {
        std::make_pair(sys_contract_rec_registration_addr, xstake::XPORPERTY_CONTRACT_GENESIS_STAGE_KEY),
        std::make_pair(sys_contract_zec_reward_addr, xstake::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_YEARLY),
    };

    std::vector<std::pair<std::string, std::string>> table_stake_string_pair_list = {
    };

    std::vector<std::pair<std::string, std::string>> table_stake_map_string_string_pair_list = {
        std::make_pair(sys_contract_sharding_reward_claiming_addr, xstake::XPORPERTY_CONTRACT_NODE_REWARD_KEY),
        std::make_pair(sys_contract_sharding_reward_claiming_addr, xstake::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY1),
        std::make_pair(sys_contract_sharding_reward_claiming_addr, xstake::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY2),
        std::make_pair(sys_contract_sharding_reward_claiming_addr, xstake::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY3),
        std::make_pair(sys_contract_sharding_reward_claiming_addr, xstake::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY4),
        std::make_pair(sys_contract_sharding_vote_addr, xstake::XPORPERTY_CONTRACT_POLLABLE_KEY),
        std::make_pair(sys_contract_sharding_vote_addr, xstake::XPORPERTY_CONTRACT_VOTES_KEY1),
        std::make_pair(sys_contract_sharding_vote_addr, xstake::XPORPERTY_CONTRACT_VOTES_KEY2),
        std::make_pair(sys_contract_sharding_vote_addr, xstake::XPORPERTY_CONTRACT_VOTES_KEY3),
        std::make_pair(sys_contract_sharding_vote_addr, xstake::XPORPERTY_CONTRACT_VOTES_KEY4),
    };
};

void usage() {
    std::cout << "------- usage -------" << std::endl;
    std::cout << "- ./xdb_export <config_json_file> <function_name>" << std::endl;
    std::cout << "    - <function_name>:" << std::endl;
    std::cout << "        - check_fast_sync [table|unit|account]" << std::endl;
    std::cout << "        - check_block_exist <account> <height>" << std::endl;
    std::cout << "        - check_block_info <account> <height|last|all>" << std::endl;
    std::cout << "        - check_block_basic <account> <height|last|all>" << std::endl;
    std::cout << "        - check_state_basic <account> <height|last|all>" << std::endl;
    std::cout << "        - check_tx_info [table] [starttime] [endtime]" << std::endl;
    std::cout << "        - check_latest_fullblock" << std::endl;
    std::cout << "        - check_contract_property <account> <prop> <last|all>" << std::endl;
    std::cout << "-------  end  -------" << std::endl;
}

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
#ifdef XDB_EXPORT_LOG
    xinit_log("./xdb_export.log", true, true);
    xset_log_level(enum_xlog_level_debug);
    xdbg("------------------------------------------------------------------");
    xinfo("new log start here");
#endif
    db_export_tools tools{db_path};
    std::string function_name{argv[2]};
    if (function_name == "check_db_reset") {
        std::string file = "all_account.json";
        if (access(file.c_str(), 0) != 0) {
            std::cout << file << " not exist!" << std::endl;
            return -1;
        }
        std::ifstream file_stream(file);
        json j;
        file_stream >> j;
        if (j.empty()) {
            std::cout << file << " not exist!" << std::endl;
            return -1;
        }
        db_reset_t reset(tools);
        reset.generate_reset_check_file(tools.get_db_unit_accounts());
    } else if (function_name == "verify") {
        if (argc < 4) {
            usage();
            return -1;
        }
        std::string file{argv[3]};
        if (access(file.c_str(), 0) != 0) {
            std::cout << "file: " << file << " not found" << std::endl;
            return -1;
        }
        json contract;
        json user;
        std::ifstream file_stream(file);
        file_stream >> contract;
        file_stream >> user;
        db_reset_t reset(tools);
        reset.verify(contract, user);
    } else if (function_name == "check_fast_sync") {
        if (argc == 3) {
            auto const & table_account_vec = db_export_tools::get_table_contract_accounts();
            auto const & unit_account_vec = tools.get_db_unit_accounts();
            tools.query_all_sync_result(table_account_vec, true);
            tools.query_all_sync_result(unit_account_vec, false);
        } else if (argc == 4) {
            std::string method_name{argv[3]};
            if (method_name == "table") {
                auto const & table_account_vec = db_export_tools::get_table_contract_accounts();
                tools.query_all_sync_result(table_account_vec, true);
            } else if (method_name == "unit") {
                auto const & unit_account_vec = tools.get_db_unit_accounts();
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
        uint32_t start_timestamp = 0;
        uint32_t end_timestamp = UINT_MAX;
        char * start_time_str = nullptr;
        char * end_time_str = nullptr;
        if (argc == 5) {
            start_time_str = argv[3];
            end_time_str = argv[4];
        } else if (argc == 6) {
            start_time_str = argv[4];
            end_time_str = argv[5];
        }
        if (argc >= 5) {
            int year, month, day, hour, minute,second;
            if (start_time_str != nullptr && sscanf(start_time_str,"%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6) {
                tm tm_;
                tm_.tm_year = year - 1900;
                tm_.tm_mon = month - 1;
                tm_.tm_mday = day;
                tm_.tm_hour = hour;
                tm_.tm_min = minute;
                tm_.tm_sec = second;
                tm_.tm_isdst = 0;
                start_timestamp = mktime(&tm_);
            }
            if (end_time_str != nullptr && sscanf(end_time_str,"%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6) {
                tm tm_;
                tm_.tm_year = year - 1900;
                tm_.tm_mon = month - 1;
                tm_.tm_mday = day;
                tm_.tm_hour = hour;
                tm_.tm_min = minute;
                tm_.tm_sec = second;
                tm_.tm_isdst = 0;
                end_timestamp = mktime(&tm_);
            }
            std::cout << "start_timestamp: " << start_timestamp << ", end_timestamp: " << end_timestamp << std::endl;
        }

        if (argc == 3 || argc == 5) {
            auto const & account_vec = db_export_tools::get_table_contract_accounts();
            tools.query_table_tx_info(account_vec, start_timestamp, end_timestamp);
        } else if (argc == 4 || argc == 6) {
            std::vector<std::string> account = {argv[3]};
            tools.query_table_tx_info(account, start_timestamp, end_timestamp);
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
        tools.query_block_info(argv[3], argv[4]);
    } else if (function_name == "check_block_basic") {
        if (argc < 5) {
            usage();
            return -1;
        }
        tools.query_block_basic(argv[3], argv[4]);
    } else if (function_name == "check_state_basic") {
        if (argc < 5) {
            usage();
            return -1;
        }
        tools.query_state_basic(argv[3], argv[4]);
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
