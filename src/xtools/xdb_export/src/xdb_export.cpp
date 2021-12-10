#include "../xdb_export.h"

#include "xbasic/xasio_io_context_wrapper.h"
#include "xblockstore/xblockstore_face.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xrootblock.h"
#include "xdata/xtable_bstate.h"
#include "xelection/xvnode_house.h"
#include "xrpc/xgetblock/get_block.h"
#include "xstake/xstake_algorithm.h"
#include "xvledger/xvledger.h"
#include "xvm/manager/xcontract_manager.h"

#define NODE_ID "T00000LgGPqEpiK6XLCKRj9gVPN8Ej1aMbyAb3Hu"
#define SIGN_KEY "ONhWC2LJtgi9vLUyoa48MF3tiXxqWf7jmT9KtOg/Lwo="

NS_BEG2(top, db_export)

xdb_export_tools_t::xdb_export_tools_t(std::string const & db_path) {
    XMETRICS_INIT();
    top::config::config_register.get_instance().set(config::xmin_free_gas_asset_onchain_goverance_parameter_t::name, std::to_string(ASSET_TOP(100)));
    top::config::config_register.get_instance().set(config::xfree_gas_onchain_goverance_parameter_t::name, std::to_string(25000));
    top::config::config_register.get_instance().set(config::xmax_validator_stake_onchain_goverance_parameter_t::name, std::to_string(5000));
    top::config::config_register.get_instance().set(config::xchain_name_configuration_t::name, std::string{top::config::chain_name_testnet});
    top::config::config_register.get_instance().set(config::xroot_hash_configuration_t::name, std::string{});
    data::xrootblock_para_t para;
    data::xrootblock_t::init(para);
    auto io_obj = std::make_shared<xbase_io_context_wrapper_t>();
    m_timer_driver = make_unique<xbase_timer_driver_t>(io_obj);
    m_bus = top::make_object_ptr<mbus::xmessage_bus_t>(true, 1000);
    m_store = top::store::xstore_factory::create_store_with_kvdb(db_path);
    base::xvchain_t::instance().set_xdbstore(m_store.get());
    base::xvchain_t::instance().set_xevmbus(m_bus.get());
    m_blockstore.attach(store::get_vblockstore());
    m_txstore = xobject_ptr_t<base::xvtxstore_t>(
        txstore::create_txstore(top::make_observer<mbus::xmessage_bus_face_t>(m_bus.get()), 
                                top::make_observer<xbase_timer_driver_t>(m_timer_driver.get())));
    base::xvchain_t::instance().set_xtxstore(m_txstore.get());
    m_nodesvr_ptr = make_object_ptr<election::xvnode_house_t>(common::xnode_id_t{NODE_ID}, SIGN_KEY, m_blockstore, make_observer(m_bus.get()));
    m_getblock = std::make_shared<chain_info::get_block_handle>(m_store.get(), m_blockstore.get(), nullptr);
    contract::xcontract_manager_t::instance().init(make_observer(m_store), xobject_ptr_t<store::xsyncvstore_t>{});
    contract::xcontract_manager_t::set_nodesrv_ptr(m_nodesvr_ptr);
}

std::vector<std::string> xdb_export_tools_t::get_system_contract_accounts() {
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
        sys_contract_sharding_statistic_info_addr,
    };
    for (auto const & u : unit) {
        v.emplace_back(u);
    }
    for (auto const & t : table) {
        for (auto i = 0; i < enum_vledger_const::enum_vbucket_has_tables_count; i++) {
            v.emplace_back(make_address_by_prefix_and_subaddr(t, uint16_t(i)).value());
        }
    }
    return v;
}

std::vector<std::string> xdb_export_tools_t::get_table_accounts() {
    std::vector<std::string> v;
    const std::vector<std::pair<std::string, int>> table = {
        std::make_pair(std::string{sys_contract_sharding_table_block_addr}, enum_vledger_const::enum_vbucket_has_tables_count),
        std::make_pair(std::string{sys_contract_zec_table_block_addr}, MAIN_CHAIN_ZEC_TABLE_USED_NUM),
        std::make_pair(std::string{sys_contract_beacon_table_block_addr}, MAIN_CHAIN_REC_TABLE_USED_NUM),
    };
    for (auto const & t : table) {
        for (auto i = 0; i < t.second; i++) {
            v.emplace_back(make_address_by_prefix_and_subaddr(t.first, uint16_t(i)).value());
        }
    }
    return v;
}

std::vector<std::string> xdb_export_tools_t::get_db_unit_accounts() {
    auto const s = query_db_unit_accounts();
    std::vector<std::string> v;
    v.assign(s.begin(), s.end());
    return v;
}

void xdb_export_tools_t::query_all_sync_result(std::vector<std::string> const & accounts_vec, bool is_table) {
    json result_json;
    uint32_t thread_num = 8;
    if (accounts_vec.size() < thread_num) {
        for (auto const & account : accounts_vec) {
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
        auto thread_helper = [&accounts_vec_split, &j_vec](xdb_export_tools_t * arg, int index) {
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

void xdb_export_tools_t::query_table_latest_fullblock() {
    json result_json;
    auto const account_vec = xdb_export_tools_t::get_table_accounts();
    for (auto const & _p : account_vec) {
        query_table_latest_fullblock(_p, result_json[_p]);
    }
    std::string filename = "all_latest_fullblock_info.json";
    std::ofstream out_json(filename);
    out_json << std::setw(4) << result_json;
    std::cout << "===> " << filename << " generated success!" << std::endl;
}

void xdb_export_tools_t::query_table_tx_info(std::vector<std::string> const & address_vec, const uint32_t start_timestamp, const uint32_t end_timestamp) {
#if 1  // TODO(jimmy) use two thread
    auto query_and_make_file = [start_timestamp, end_timestamp](xdb_export_tools_t *arg, std::string account) {
        json result_json;
        arg->query_table_tx_info(account, start_timestamp, end_timestamp, result_json);
        std::string filename = "./all_table_tx_info/" + account + "_tx_info.json";
        std::ofstream out_json(filename);
        out_json << std::setw(4) << result_json[account];
        std::cout << "===> " << filename << " generated success!" << std::endl; 
    };
    mkdir("all_table_tx_info", 0750);
    uint32_t thread_num = 4;
    std::cout << "query_table_tx_info with thread num " << thread_num << std::endl;
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
        auto thread_helper = [&query_and_make_file, &address_vec_split](xdb_export_tools_t *arg, int index) {
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
#else
    mkdir("all_table_tx_info", 0750);
    for (size_t i = 0; i < address_vec.size(); i++) {
        auto t0 = xtime_utl::time_now_ms();
        json result_json;
        query_table_tx_info(address_vec[i], start_timestamp, end_timestamp, result_json);
        auto t1 = xtime_utl::time_now_ms();
        std::string filename = "./all_table_tx_info/" + address_vec[i] + "_tx_info.json";
        std::ofstream out_json(filename);
        out_json << std::setw(4) << result_json[address_vec[i]];
        std::cout << "===> " << filename << " generated success!" << std::endl;
        result_json.clear();
        auto t2 = xtime_utl::time_now_ms();
        std::cout << "table=" << address_vec[i] << " t1-t0=" << t1 - t0 << " t2-t1=" << t2 - t1 << std::endl;
    }
#endif
}

void xdb_export_tools_t::query_block_exist(std::string const & address, const uint64_t height) {
    auto const block_vec = m_blockstore->load_block_object(address, height).get_vector();
    std::cout << "account: " << address << " , height: " << height << " , block exist, total num: " << block_vec.size() << std::endl;
    for (auto const & block : block_vec) {
        if (block != nullptr) {
            std::cout << block->dump2() << std::endl;
        } else {
            std::cerr << "exist one null block!!!" << std::endl;
        }
    }
}

void xdb_export_tools_t::query_block_info(std::string const & account, std::string const & param) {
    xJson::Value root;
    if (param == "last") {
        auto const h = m_blockstore->get_latest_committed_block_height(base::xvaccount_t{account});
        std::cout << "account: " << account << ", latest committed height: " << h << ", block info:" << std::endl;
        query_block_info(account, h, root);
    } else if (param != "all") {
        auto const h = base::xstring_utl::touint64(param);
        std::cout << "account: " << account << ", height: " << h << ", block info:" << std::endl;
        query_block_info(account, h, root);
    } else {
        auto const h = m_blockstore->get_latest_committed_block_height(base::xvaccount_t{account});
        for (size_t i = 0; i <= h; i++) {
            xJson::Value j;
            query_block_info(account, i, j);
            root["height" + std::to_string(i)] = j;
        }
    }
    std::string filename = account + "_all_block_info.json";
    std::ofstream out_json(filename);
    out_json << std::setw(4) << root;
    std::cout << "===> " << filename << " generated success!" << std::endl;
    out_json.flush();
    out_json.close();
}

void xdb_export_tools_t::query_block_basic(std::vector<std::string> const & account_vec, std::string const & param) {
    for (auto const & account : account_vec) {
        query_block_basic(account, param);
    }
}

void xdb_export_tools_t::query_block_basic(std::string const & account, std::string const & param) {
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
    } else if (param != "all") {
        uint64_t h = std::stoi(param);
        std::cout << "account: " << account << ", height: " << h << ", block info:" << std::endl;
        query_block_basic(account, h, root);
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
    }
    std::string filename = "./all_block_basic_info/" + account + "_all_block_basic.json";
    std::ofstream out_json(filename);
    out_json << std::setw(4) << root;
    std::cout << "===> " << filename << " generated success!" << std::endl;
    out_json.flush();
    out_json.close();
}

void xdb_export_tools_t::query_state_basic(std::vector<std::string> const & account_vec, std::string const & param) {
    for (auto const & account : account_vec) {
        query_state_basic(account, param);
    }
}

void xdb_export_tools_t::query_state_basic(std::string const & account, std::string const & param) {
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
    } else if (param != "all") {
        uint64_t h = std::stoi(param);
        std::cout << "account: " << account << ", height: " << h << ", state info:" << std::endl;
        query_state_basic(account, h, root);
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
    }
    std::string filename = "./all_state_basic_info/" + account + "_all_state_basic.json";
    std::ofstream out_json(filename);
    out_json << std::setw(4) << root;
    std::cout << "===> " << filename << " generated success!" << std::endl;
    out_json.flush();
    out_json.close();
}

void xdb_export_tools_t::query_state_basic(std::string const & account, const uint64_t h, json & result) {
    auto block_vec = m_blockstore->load_block_object(account, h).get_vector();
    if (block_vec.empty()) {
        std::cerr << "account: " << account << ", height: " << h << " block null" << std::endl;
        return;
    }
    for (size_t i = 0; i < block_vec.size(); i++) {
        auto const & vblock = block_vec[i];
        if (vblock == nullptr) {
            std::cerr << "account: " << account << ", height: " << h << " block[" << i << "] null" << std::endl;
            continue;
        }
        auto bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(vblock);
        if (bstate == nullptr) {
            std::cerr << "account: " << account << ", height: " << h << " state null" << std::endl;
            continue;
        }
        if (block_vec.size() > 1) {
            std::string block_id = std::string{"block"} + std::to_string(i);
            result[block_id]["account"] = bstate->get_account();
            result[block_id]["height"] = bstate->get_block_height();
            result[block_id]["class"] = bstate->get_block_class();
            result[block_id]["viewid"] = bstate->get_block_viewid();
            result[block_id]["last_hash"] = base::xstring_utl::to_hex(bstate->get_last_block_hash());
        } else {
            result["account"] = bstate->get_account();
            result["height"] = bstate->get_block_height();
            result["class"] = bstate->get_block_class();
            result["viewid"] = bstate->get_block_viewid();
            result["last_hash"] = base::xstring_utl::to_hex(bstate->get_last_block_hash());
        }
    }
}

void xdb_export_tools_t::query_meta(std::vector<std::string> const & account_vec) {
    json root;
    for (auto const & account : account_vec) {
        query_meta(account, root[account]);
    }
    std::string filename = "all_meta_data.json";
    std::ofstream out_json(filename);
    out_json << std::setw(4) << root;
    std::cout << "===> " << filename << " generated success!" << std::endl;
    out_json.flush();
    out_json.close();
}

void xdb_export_tools_t::query_meta(std::string const & account) {
    json root;
    query_meta(account, root);
    std::string filename = account + "_meta_data.json";
    std::ofstream out_json(filename);
    out_json << std::setw(4) << root;
    std::cout << "===> " << filename << " generated success!" << std::endl;
    out_json.flush();
    out_json.close();
}

void xdb_export_tools_t::query_meta(std::string const & account, json & result) {
    base::xvaccount_t account_vid{account};
    auto target_table = base::xvchain_t::instance().get_table(account_vid.get_xvid());
    if (target_table == nullptr) {
        std::cerr << "account " << account << " invalid!" << std::endl;
        return;
    }
    auto accountobj = target_table->get_account(account_vid);
    if (target_table == nullptr) {
        std::cerr << "account " << account << " not found!" << std::endl;
        return;
    }
    auto meta_data = accountobj->get_full_meta();
    auto block_meta = meta_data.clone_block_meta();
    result["block_meta"]["lowest_vkey2_block_height"] = block_meta._lowest_vkey2_block_height;
    result["block_meta"]["highest_deleted_block_height"] = block_meta._highest_deleted_block_height;
    result["block_meta"]["highest_cert_block_height"] = block_meta._highest_cert_block_height;
    result["block_meta"]["highest_lock_block_height"] = block_meta._highest_lock_block_height;
    result["block_meta"]["highest_commit_block_height"] = block_meta._highest_commit_block_height;
    result["block_meta"]["highest_full_block_height"] = block_meta._highest_full_block_height;
    result["block_meta"]["highest_connect_block_height"] = block_meta._highest_connect_block_height;
    result["block_meta"]["highest_connect_block_hash"] = base::xstring_utl::to_hex(block_meta._highest_connect_block_hash);
    result["block_meta"]["block_level"] = block_meta._block_level;
    auto state_meta = meta_data.clone_state_meta();
    result["state_meta"]["lowest_execute_block_height"] = state_meta._lowest_execute_block_height;
    result["state_meta"]["highest_execute_block_height"] = state_meta._highest_execute_block_height;
    result["state_meta"]["highest_execute_block_hash"] = base::xstring_utl::to_hex(state_meta._highest_execute_block_hash);
    auto index_meta = meta_data.clone_index_meta();
    result["index_meta"]["latest_unit_height"] = index_meta.m_latest_unit_height;
    result["index_meta"]["latest_unit_viewid"] = index_meta.m_latest_unit_viewid;
    result["index_meta"]["latest_tx_nonce"] = index_meta.m_latest_tx_nonce;
    result["index_meta"]["account_flag"] = index_meta.m_account_flag;
    auto sync_meta = meta_data.clone_sync_meta();
    result["sync_meta"]["highest_genesis_connect_height"] = sync_meta._highest_genesis_connect_height;
    result["sync_meta"]["highest_genesis_connect_hash"] = sync_meta._highest_genesis_connect_hash;
    result["sync_meta"]["highest_sync_height"] = sync_meta._highest_sync_height;
}

void xdb_export_tools_t::query_table_unit_state(std::vector<std::string> const & table_vec) {
    for (auto const & table : table_vec) {
        query_table_unit_state(table);
    }
}

void xdb_export_tools_t::query_table_unit_state(std::string const & table) {
    if (!is_table_address(common::xaccount_address_t{table})) {
        std::cerr << table << " is not a table address!" << std::endl;
        return;
    }
    json root;
    query_table_unit_state(table, root);
    std::string filename = "./all_table_unit_state/" + table + "_table_unit_state.json";
    std::ofstream out_json(filename);
    out_json << std::setw(4) << root;
    std::cout << "===> " << filename << " generated success!" << std::endl;
    out_json.flush();
    out_json.close();
}

void xdb_export_tools_t::query_table_unit_state(std::string const & table, json & root) {
    auto vblock = m_blockstore->get_latest_committed_block(base::xvaccount_t{table});
    if (vblock == nullptr) {
        std::cerr << "table: " << table << ", latest committed block nullptr!" << std::endl;
        return;
    }
    auto bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(vblock.get());
    if (bstate == nullptr) {
        std::cerr << "table: " << table << ", latest committed block state constructed failed" << std::endl;
        return;
    }
    
    {
        json json_table;
        json_table["account"] = bstate->get_account();
        json_table["commit_height"] = bstate->get_block_height();
        json_table["class"] = bstate->get_block_class();
        json_table["type"] = bstate->get_block_type();
        json_table["level"] = bstate->get_block_level();
        json_table["viewid"] = bstate->get_block_viewid();
        json_table["last_hash"] = base::xstring_utl::to_hex(bstate->get_last_block_hash());
        json_table["last_fullblock_hash"] = base::xstring_utl::to_hex(bstate->get_last_fullblock_hash());
        json_table["last_fullblock_height"] = bstate->get_last_fullblock_height();
        {
            json j_property;
            property_json(bstate, j_property);
            json_table["system_property"] = j_property["system_property"];
            json_table["user_property"] = j_property["user_property"];
        }
        root[table] = json_table;
    }

    std::map<std::string, std::string> table_account_index;
    if (false == bstate->find_property(XPROPERTY_TABLE_ACCOUNT_INDEX)) {
        std::cerr << "table: " << table << ", latest committed block state fail-find property XPROPERTY_TABLE_ACCOUNT_INDEX " << XPROPERTY_TABLE_ACCOUNT_INDEX << std::endl;
    } else {
        auto propobj = bstate->load_string_map_var(XPROPERTY_TABLE_ACCOUNT_INDEX);
        table_account_index = propobj->query();
    }
    for (auto const & pair : table_account_index) {
        auto const & account  = pair.first;
        json j_unit;
        root[account] = j_unit;
        auto unit_vblock = m_blockstore->get_latest_committed_block(base::xvaccount_t{account});
        if (unit_vblock == nullptr) {
            std::cerr << "table: " << table << ", unit: " << account << ", latest committed block nullptr!" << std::endl;
            continue;
        }
        auto unit_bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(unit_vblock.get());
        if (unit_bstate == nullptr) {
            std::cerr << "table: " << table << ", unit: " << account << ", latest committed block state constructed failed" << std::endl;
            continue;
        }
        j_unit["account"] = unit_bstate->get_account();
        j_unit["commit_height"] = unit_bstate->get_block_height();
        j_unit["class"] = unit_bstate->get_block_class();
        j_unit["type"] = unit_bstate->get_block_type();
        j_unit["level"] = unit_bstate->get_block_level();
        j_unit["viewid"] = unit_bstate->get_block_viewid();
        j_unit["last_hash"] = base::xstring_utl::to_hex(unit_bstate->get_last_block_hash());
        j_unit["last_fullblock_hash"] = base::xstring_utl::to_hex(unit_bstate->get_last_fullblock_hash());
        j_unit["last_fullblock_height"] = unit_bstate->get_last_fullblock_height();
        {
            json j_property;
            property_json(unit_bstate, j_property);
            j_unit["system_property"] = j_property["system_property"];
            j_unit["user_property"] = j_property["user_property"];
        }
        root[account] = j_unit;
    }
}

void xdb_export_tools_t::query_property(std::string const & account, std::string const & prop_name, std::string const & param) {
    auto const latest_height = m_blockstore->get_latest_committed_block_height(account);

    json jph;
    if (param == "last") {
        query_property(account, prop_name, latest_height, jph);
    } else if (param != "all") {
        auto const h = base::xstring_utl::touint64(param);
        query_property(account, prop_name, h, jph["height " + base::xstring_utl::tostring(h)]);
    } else {
        for (uint64_t i = 0; i <= latest_height; i++) {
            query_property(account, prop_name, i, jph["height " + base::xstring_utl::tostring(i)]);
        }
    }
    std::string filename = account + "_" + prop_name + "_" + param + "_property.json";
    std::ofstream out_json(filename);
    out_json << std::setw(4) << jph;
    std::cout << "===> " << filename << " generated success!" << std::endl;
    out_json.flush();
    out_json.close();
}

void xdb_export_tools_t::query_property(std::string const & account, std::string const & prop_name, const uint64_t height, json & j) {
    auto const block = m_blockstore->load_block_object(account, height, 0, false);
    if (block == nullptr) {
        std::cerr << account << " height: " << height << " block null!" << std::endl;
        return;
    }
    auto const bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(block.get());
    if (bstate == nullptr) {
        std::cerr << account << " height: " << height << " bstate null!" << std::endl;
        return;
    }
    property_json(bstate, j);
}

void xdb_export_tools_t::query_balance() {
    json root;
    uint64_t balance = 0;
    uint64_t lock_balance = 0;
    uint64_t tgas_balance = 0;
    uint64_t vote_balance = 0;
    uint64_t burn_balance = 0;
    auto const table_vec = xdb_export_tools_t::get_table_accounts();
    for (auto const & table : table_vec) {
        json j_table;
        query_balance(table, root[table], j_table);
        uint64_t table_balance = 0;
        uint64_t table_lock_balance = 0;
        uint64_t table_tgas_balance = 0;
        uint64_t table_vote_balance = 0;
        uint64_t table_burn_balance = 0;
        if (j_table == nullptr) {
            table_balance = 0;
            table_lock_balance = 0;
            table_tgas_balance = 0;
            table_vote_balance = 0;
            table_burn_balance = 0;
        } else {
            table_balance = j_table[XPROPERTY_BALANCE_AVAILABLE].get<uint64_t>();
            table_lock_balance = j_table[XPROPERTY_BALANCE_LOCK].get<uint64_t>();
            table_tgas_balance = j_table[XPROPERTY_BALANCE_PLEDGE_TGAS].get<uint64_t>();
            table_vote_balance = j_table[XPROPERTY_BALANCE_PLEDGE_VOTE].get<uint64_t>();
            table_burn_balance = j_table[XPROPERTY_BALANCE_BURN].get<uint64_t>();
        }
        std::cout << "table: " << table;
        std::cout << ", available balance: " << table_balance;
        std::cout << ", lock balance: " << table_lock_balance;
        std::cout << ", tgas balance: " << table_tgas_balance;
        std::cout << ", vote balance: " << table_vote_balance;
        std::cout << ", burn balance: " << table_burn_balance << std::endl;
        balance += table_balance;
        lock_balance += table_lock_balance;
        tgas_balance += table_tgas_balance;
        vote_balance += table_vote_balance;
        burn_balance += table_burn_balance;
    }
    std::cout << "===> top system" << std::endl;
    std::cout << "total available balance: " << balance;
    std::cout << ", total lock balance: " << lock_balance;
    std::cout << ", total tgas balance: " << tgas_balance;
    std::cout << ", total vote balance: " << vote_balance;
    std::cout << ", total burn balance: " << burn_balance << std::endl;
    uint64_t total_balance = balance + lock_balance + tgas_balance + vote_balance;
    std::cout << "===> top system calculate" << std::endl;
    std::cout << "total balance: " << total_balance;
    std::cout << ", total burn balance: " << burn_balance;
    std::cout << ", usable balance: " << total_balance - burn_balance << std::endl;

    std::cout << "===> top reward issurance" << std::endl;
    auto reward_vblock = m_blockstore->get_latest_committed_block(base::xvaccount_t{sys_contract_zec_reward_addr});
    auto reward_bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(reward_vblock.get());
    if (reward_bstate != nullptr && true == reward_bstate->find_property(xstake::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE)) {
        auto map = reward_bstate->load_string_map_var(xstake::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE)->query();
        std::cout << map["total"] << std::endl;
    } else {
        std::cerr << "load error!" << std::endl;
    }

    std::string filename = "top_balance_detail.json";
    std::ofstream out_json(filename);
    out_json << std::setw(4) << root;
    std::cout << "===> " << filename << " generated success!" << std::endl;
    out_json.flush();
    out_json.close();
}

void xdb_export_tools_t::query_archive_db() {
    std::string filename = "check_archive_db.log";
    std::ofstream file(filename);
    // step 1: check table
    std::cout << "step 1 ===> checking table accounts..." << std::endl;
    auto const table_accounts = xdb_export_tools_t::get_table_accounts();
    for (auto const & table : table_accounts) {
        uint32_t error_num = 0;
        error_num += query_block_continuity_and_integrity(table, query_account_table, file);
        error_num += query_cert_continuity(table, query_account_table, file);
        if (error_num) {
            std::cout << "table: " << table << ", check not ok, error num: " << error_num << std::endl;
        } else {
            std::cout << "table: " << table << ", check ok" << std::endl;
        }
    }
    // step 2: check unit
    std::cout << "step 2 ===> checking unit accounts..." << std::endl;
    auto const unit_accounts = get_db_unit_accounts();
    for (auto const & unit : unit_accounts) {
        uint32_t error_num = 0;
        error_num += query_block_continuity_and_integrity(unit, query_account_unit, file);
        error_num += query_cert_continuity(unit, query_account_unit, file);
        if (error_num) {
            std::cout << "unit: " << unit << ", check not ok, error num: " << error_num << std::endl;
        } else {
            std::cout << "unit: " << unit << ", check ok" << std::endl;
        }
    }
    // step 3: check drand
    std::cout << "step 3 ===> checking drand..." << std::endl;
    {
        uint32_t error_num = 0;
        std::string drand{sys_drand_addr};
        error_num += query_block_continuity(drand, query_account_unit, file);
        error_num += query_cert_continuity(drand, query_account_unit, file);
        if (error_num) {
            std::cout << "drand: " << drand << ", check not ok, error num: " << error_num << std::endl;
        } else {
            std::cout << "drand: " << drand << ", check ok" << std::endl;
        }
    }
    file.flush();
    file.close();
}

uint32_t xdb_export_tools_t::query_block_continuity_and_integrity(std::string const & account, enum_query_account_type type, std::ofstream & file) {
    uint32_t error_num = 0;
    std::string type_str;
    if (type == query_account_table) {
        type_str = "table";
    } else if (type == query_account_unit) {
        type_str = "unit";
    }
    auto const committd_height = m_blockstore->get_latest_committed_block_height(account);
    auto const connected_height = m_blockstore->get_latest_connected_block_height(account);
    file << "[info] " << type_str << ": " << account << ", committd_height: " << committd_height << ", connected_height: " << connected_height
         << ", genesis_connected_height: " << m_blockstore->get_latest_genesis_connected_block_height(account)
         << ", executed_height: " << m_blockstore->get_latest_executed_block_height(account) << std::endl;
    if (committd_height != connected_height) {
        file << "[warn] " << type_str << ": " << account << ", committd_height and connected_height not equal, " << committd_height << ", " << connected_height << std::endl;
        error_num++;
    }
    for (size_t h = 0; h < connected_height; h++) {
        auto const block = m_blockstore->load_block_object(account, h, 0, true);
        if (block == nullptr) {
            file << "[warn] " << type_str << ": " << account << ", height: " << h << ", block is nullptr!" << std::endl;
            error_num++;
            continue;
        }
        if (block->is_input_ready(true) == false) {
            file << "[warn] " << type_str << ": " << account << ", height: " << h << ", block input is not ready!" << std::endl;
            error_num++;
        }
        if (block->is_output_ready(true) == false) {
            file << "[warn] " << type_str << ": " << account << ", height: " << h << ", block output is not ready!" << std::endl;
            error_num++;
        }
        if (block->is_deliver(true) == false) {
            file << "[warn] " << type_str << ": " << account << ", height: " << h << ", block not deliver!" << std::endl;
            error_num++;
        }
        if (type == query_account_unit) {
            continue;
        }
        auto const & _table_inentitys = block->get_input()->get_entitys();
        auto const entitys_count = _table_inentitys.size();
        for (size_t index = 0; index < entitys_count; index++) {  // unit entity from index#1
            auto _table_unit_inentity = dynamic_cast<base::xvinentity_t *>(_table_inentitys[index]);
            auto const & input_actions = _table_unit_inentity->get_actions();
            for (auto & action : input_actions) {
                if (action.get_org_tx_hash().empty()) {  // not txaction
                    continue;
                }
                data::xlightunit_action_t txaction(action);
                auto txindex = base::xvchain_t::instance().get_xtxstore()->load_tx_idx(txaction.get_tx_hash(), txaction.get_tx_subtype());
                if (txindex == nullptr) {
                    file << "[warn] " << type_str << ": " << account << ", height: " << h << ", tx: " << base::xstring_utl::to_hex(txaction.get_tx_hash()) << ", load tx idx null!" << std::endl;
                    error_num++;
                }
            }
        }
    }
    return error_num;
}

uint32_t xdb_export_tools_t::query_block_continuity(std::string const & account, enum_query_account_type type, std::ofstream & file) {
    uint32_t error_num = 0;
    std::string type_str;
    if (type == query_account_table) {
        type_str = "table";
    } else if (type == query_account_unit) {
        type_str = "unit";
    }
    auto const committd_height = m_blockstore->get_latest_committed_block_height(account);
    auto const connected_height = m_blockstore->get_latest_connected_block_height(account);
    file << "[info] " << type_str << ": " << account << ", committd_height: " << committd_height << ", connected_height: " << connected_height
         << ", genesis_connected_height: " << m_blockstore->get_latest_genesis_connected_block_height(account)
         << ", executed_height: " << m_blockstore->get_latest_executed_block_height(account) << std::endl;
    if (committd_height != connected_height) {
        file << "[warn] " << type_str << ": " << account << ", committd_height and connected_height not equal, " << committd_height << ", " << connected_height << std::endl;
        error_num++;
    }
    for (size_t h = 0; h < connected_height; h++) {
        auto const block = m_blockstore->load_block_object(account, h, 0, true);
        if (block == nullptr) {
            file << "[warn] " << type_str << ": " << account << ", height: " << h << ", block is nullptr!" << std::endl;
            error_num++;
        }
    }
    return error_num;
}

uint32_t xdb_export_tools_t::query_cert_continuity(std::string const & account, enum_query_account_type type, std::ofstream & file) {
    uint32_t error_num = 0;
    std::string type_str;
    if (type == query_account_table) {
        type_str = "table";
    } else if (type == query_account_unit) {
        type_str = "unit";
    }
    auto const cert_block = m_blockstore->get_latest_cert_block(account);
    if (cert_block == nullptr)  {
        file << "[warn] " << type_str << ": " << account << ", latest cert block is nullptr!" << std::endl;
        error_num++;
    } else {
        auto bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(cert_block.get());
        if (bstate == nullptr) {
            file << "[warn] " << type_str << ": " << account << ", latest cert state is nullptr!" << std::endl;
            error_num++;
        }
    }
    return error_num;
}

std::set<std::string> xdb_export_tools_t::query_db_unit_accounts() {
    std::ifstream file_stream("all_account.json");
    if (!file_stream.good()) {
        return generate_db_unit_accounts_file(QUERY_ACCOUNT_V2);
    }
    json j;
    file_stream >> j;
    if (!j.empty()) {
        std::set<std::string> accounts;
        for (auto _table : j) {
            for (auto _acc : _table) {
                accounts.insert(_acc.get<std::string>());
            }
        }
        return accounts;
    } else {
        return generate_db_unit_accounts_file(QUERY_ACCOUNT_V2);
    }
}

std::set<std::string> xdb_export_tools_t::generate_db_unit_accounts_file(enum_query_account_version version) {
    std::set<std::string> accounts;
    json j;
    std::cout << "all_account.json generating..." << std::endl;
    auto const tables = get_table_accounts();
    if (version == QUERY_ACCOUNT_V1) {
        for (auto const & table : tables) {
            auto const & units = query_unit_account(table);
            accounts.insert(units.cbegin(), units.cend());
            for (auto const & unit : units) {
                j[table].push_back(unit);
            }
        }
    } else if (version == QUERY_ACCOUNT_V2) {
        for (auto const & table : tables) {
            auto const & units = query_unit_account2(table);
            accounts.insert(units.cbegin(), units.cend());
            for (auto const & unit : units) {
                j[table].push_back(unit);
            }
        }
    } else {
        assert(false);
    }

    std::ofstream file_stream("all_account.json");
    file_stream << std::setw(4) << j;
    std::cout << "===> all_account.json generated success!" << std::endl;
    return accounts;
}

std::set<std::string> xdb_export_tools_t::query_unit_account(std::string const & account) {
    std::set<std::string> accounts_set;
    auto const block_height = m_blockstore->get_latest_committed_block_height(account);
    for (uint64_t h = 0; h <= block_height; ++h) {
        auto vblock = m_blockstore->load_block_object(account, h, 0, true);
        data::xblock_t * block = dynamic_cast<data::xblock_t *>(vblock.get());
        if (block != nullptr) {
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
    return accounts_set;
}

std::set<std::string> xdb_export_tools_t::query_unit_account2(std::string const & account) {
    std::set<std::string> accounts_set;
    auto latest_block = m_blockstore->get_latest_committed_block(account);
    if (latest_block == nullptr) {
        std::cerr << account << " get_latest_committed_block null!" << std::endl;
        return {};
    }
    base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(latest_block.get());
    if (bstate == nullptr) {
        std::cerr << account << " get_block_state null!" << std::endl;
        return query_unit_account(account);
    } else {
        auto state = std::make_shared<xtable_bstate_t>(bstate.get());
        return state->get_all_accounts();
    }
}

void xdb_export_tools_t::query_sync_result(std::string const & account, const uint64_t h_s, const uint64_t h_e, std::string & result, int init_s, int init_e) {
    int start = h_s;
    int end = -1;
    if (init_s != -1) {
        start = init_s;
    }
    if (init_e != -1) {
        end = init_e;
    }
    for (uint64_t h = h_s; h <= h_e; h++) {
        auto vblock = m_blockstore->load_block_object(account, h, 0, false);
        data::xblock_t * block = dynamic_cast<data::xblock_t *>(vblock.get());
        if (block == nullptr) {
            if (end != -1) {
                if (start == end) {
                    result += std::to_string(start) + ',';
                } else {
                    result += std::to_string(start) + '-' + std::to_string(end) + ',';
                }
            }
            start = h + 1;
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

void xdb_export_tools_t::query_sync_result(std::string const & account, json & result_json) {
    auto const block_height = m_blockstore->get_latest_committed_block_height(account);
    auto const connect_height = m_blockstore->get_latest_genesis_connected_block_height(account);
    if (connect_height > block_height) {
        std::cout << account << " connect_height: " << connect_height << " > block_height: " << block_height << ", error" << std::endl;
        result_json = "error";
        return;
    }
    if (block_height == connect_height) {
        result_json = "0-" + base::xstring_utl::tostring(block_height) + ",";
    } else {
        std::string result;
        query_sync_result(account, connect_height + 1, block_height, result, 0, connect_height);
        result_json = result;
    }
}

void xdb_export_tools_t::query_table_latest_fullblock(std::string const & account, json & j) {
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

void xdb_export_tools_t::read_info_from_table_block(const data::xblock_t * block, xdbtool_table_info_t & table_info, std::vector<tx_ext_t> & txinfos) {
    assert(block->get_block_level() == base::enum_xvblock_level_table);
    if (block->get_block_class() == base::enum_xvblock_class_nil) {
        table_info.empty_table_block_num++;
        return;
    } else if (block->get_block_class() == base::enum_xvblock_class_full) {
        table_info.full_table_block_num++;
        return;
    } else {
        table_info.light_table_block_num++;
    }

    const uint64_t timestamp = block->get_timestamp();

    auto unit_headers = block->get_sub_block_headers();
    table_info.total_unit_block_num += unit_headers.size();
    for (auto & _unit_header : unit_headers) {
        if (_unit_header->get_block_class() == base::enum_xvblock_class_nil) {
            table_info.empty_unit_block_num++;
        } else if (_unit_header->get_block_class() == base::enum_xvblock_class_full) {
            table_info.full_unit_block_num++;
        } else {
            table_info.light_unit_block_num++;
        }
    }


    auto input_actions = block->get_tx_actions();
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
                    table_info.tx_v2_num++;
                    table_info.tx_v2_total_size += tx_size;
                } else {
                    table_info.tx_v1_num++;
                    table_info.tx_v1_total_size += tx_size;
                }
            }
        }
        tx_ext_t tx_ext;
        if (tx_ptr != nullptr) {
            tx_ext.src = tx_ptr->get_source_addr();
            tx_ext.target = tx_ptr->get_target_addr();
            tx_ext.fire_timestamp = tx_ptr->get_fire_timestamp();
        }
        // tx_ext.tableid = tableid;
        tx_ext.height = block->get_height();
        tx_ext.timestamp = timestamp;
        tx_ext.hash = "0x" + txaction.get_tx_hex_hash();
        // tx_ext.unit_height = unit_height;
        tx_ext.phase = txaction.get_tx_subtype();
        txinfos.push_back(tx_ext);

        auto type = txaction.get_tx_subtype();
        if (type == enum_transaction_subtype_self) {
            table_info.selftx_num++;
        } else if (type == enum_transaction_subtype_send) {
            table_info.sendtx_num++;
        } else if (type == enum_transaction_subtype_recv) {
            table_info.recvtx_num++;
        } else if (type == enum_transaction_subtype_confirm) {
            table_info.confirmtx_num++;
        }
    }
}

void xdb_export_tools_t::set_txinfo_to_json(json & j, const tx_ext_t & txinfo) {
    json tx;
    tx["table height"] = txinfo.height;
    tx["timestamp"] = txinfo.timestamp;
    tx["source address"] = txinfo.src;
    tx["target address"] = txinfo.target;
    tx["unit height"] = txinfo.unit_height;
    tx["phase"] = txinfo.phase;
    tx["table id"] = txinfo.tableid;
    j[txinfo.hash] = tx;
}

void xdb_export_tools_t::set_txinfos_to_json(json & j, const std::map<std::string, tx_ext_t> & txinfos) {
    for (auto & v : txinfos) {
        auto & txinfo = v.second;
        set_txinfo_to_json(j, txinfo);
    }
}

void xdb_export_tools_t::set_txinfos_to_json(json & j, const std::vector<tx_ext_t> & txinfos) {
    for (auto & v : txinfos) {
        auto & txinfo = v;
        set_txinfo_to_json(j, txinfo);
    }
}

void xdb_export_tools_t::set_confirmed_txinfo_to_json(json & j, const tx_ext_t & send_txinfo, const tx_ext_t & confirm_txinfo) {
    uint64_t delay_from_send_to_confirm = confirm_txinfo.timestamp - send_txinfo.timestamp;
    uint64_t delay_from_fire_to_confirm = confirm_txinfo.timestamp > send_txinfo.fire_timestamp;

    json tx;
    tx["time cost"] = delay_from_send_to_confirm;
    tx["time cost from fire"] = delay_from_fire_to_confirm;
    tx["send time"] =  send_txinfo.timestamp;        
    tx["confirm time"] = confirm_txinfo.timestamp;
    tx["send table height"] = send_txinfo.height;
    tx["confirm table height"] = confirm_txinfo.height;
    tx["send unit height"] = send_txinfo.unit_height;
    tx["confirm unit height"] =confirm_txinfo.unit_height;
    tx["source address"] = send_txinfo.src;
    tx["target address"] = send_txinfo.target;
    j[send_txinfo.hash] = tx;
}

void xdb_export_tools_t::set_table_txdelay_time(xdbtool_table_info_t & table_info, const tx_ext_t & send_txinfo, const tx_ext_t & confirm_txinfo) {
    uint64_t delay_from_send_to_confirm = confirm_txinfo.timestamp - send_txinfo.timestamp;
    uint64_t delay_from_fire_to_confirm = confirm_txinfo.timestamp > send_txinfo.fire_timestamp ? confirm_txinfo.timestamp - send_txinfo.fire_timestamp : 0;

    table_info.total_confirm_time_from_send += delay_from_send_to_confirm;
    if (delay_from_send_to_confirm > table_info.max_confirm_time_from_send) {
        table_info.max_confirm_time_from_send = delay_from_send_to_confirm;
    }
    table_info.total_confirm_time_from_fire += delay_from_fire_to_confirm;
    if (delay_from_fire_to_confirm > table_info.max_confirm_time_from_fire) {
        table_info.max_confirm_time_from_fire = delay_from_fire_to_confirm;
    }
}

void xdb_export_tools_t::query_table_tx_info(std::string const & account, const uint32_t start_timestamp, const uint32_t end_timestamp, json & result_json) {
    auto const block_height = m_blockstore->get_latest_committed_block_height(account);
    std::map<std::string, tx_ext_t> sendonly;  // sendtx without confirmed
    std::map<std::string, tx_ext_t> confirmonly;  // confirmtx without send
    std::vector<tx_ext_t> multi_txs; // ERROR tx is multi packed in block
    std::map<std::string, uint16_t> tx_phase_count; // tx phase count for check multi txs statistic
    json txs_confirmed_j;
    json txs_self_j;
    json j; // the total json

    xdbtool_table_info_t table_info;
    int tableid{-1};
    {
        std::vector<std::string> parts;
        if(base::xstring_utl::split_string(account,'@',parts) >= 2) {
            tableid = base::xstring_utl::toint32(parts[1]);
        } else {
            std::cout << account << " parse table id error" << tableid << std::endl;
            return;
        }
    }
    auto t1 = base::xtime_utl::time_now_ms();
    base::xvaccount_t _vaccount(account);
    for (uint64_t h = 0; h <= block_height; h++) {
        auto const vblock = m_blockstore->load_block_object(account,h,0,false);
        const data::xblock_t * block = dynamic_cast<data::xblock_t *>(vblock.get());
        if (block == nullptr) {
            table_info.missing_table_block_num++;
            std::cout << account << " ERROR:missing block at height " << h << std::endl;
            continue;
        }
        if (block->get_block_class() == base::enum_xvblock_class_light) {
            m_blockstore->load_block_input(_vaccount, vblock.get());
        }

        const uint64_t timestamp = block->get_timestamp();
        if (timestamp < start_timestamp || timestamp > end_timestamp) {
            continue;
        }

        std::vector<tx_ext_t> txinfos;
        read_info_from_table_block(block, table_info, txinfos);

        for (auto & tx_ext : txinfos) {
            auto type = tx_ext.phase;
            uint16_t phase_count = 0;
            auto iter = tx_phase_count.find(tx_ext.hash);
            if (iter != tx_phase_count.end()) {
                phase_count = iter->second;
            }
            phase_count++;
            tx_phase_count[tx_ext.hash] = phase_count;

            if (type == enum_transaction_subtype_self || type == enum_transaction_subtype_send || type == enum_transaction_subtype_recv) {
                if (phase_count > 1) {
                    multi_txs.push_back(tx_ext);
                }
            } else if (type == enum_transaction_subtype_confirm) {
                if (phase_count > 2) {
                    multi_txs.push_back(tx_ext);
                }
            }
        }

        for (auto & tx_ext : txinfos) {
            auto type = tx_ext.phase;
            if (type == enum_transaction_subtype_self) {
                set_txinfo_to_json(txs_self_j, tx_ext);
            } else if (type == enum_transaction_subtype_send) {
                sendonly[tx_ext.hash] = tx_ext;
            } else if (type == enum_transaction_subtype_confirm) {
                auto iter = sendonly.find(tx_ext.hash);
                if (iter != sendonly.end()) {
                    set_table_txdelay_time(table_info, iter->second, tx_ext);
                    set_confirmed_txinfo_to_json(txs_confirmed_j, iter->second, tx_ext);
                    sendonly.erase(tx_ext.hash);
                    table_info.confirmedtx_num++;
                } else {
                    // may appear when missing table blocks
                    confirmonly[tx_ext.hash] = tx_ext;
                }
            }
        }
    }

    auto t2 = base::xtime_utl::time_now_ms();
    j["table info"]["table height"] = block_height;
    j["table info"]["total table block num"] = block_height + 1;
    j["table info"]["miss table block num"] = table_info.missing_table_block_num;
    j["table info"]["empty table block num"] = table_info.empty_table_block_num;
    j["table info"]["light table block num"] = table_info.light_table_block_num;
    j["table info"]["full table block num"] = table_info.full_table_block_num;
    j["table info"]["total unit block num"] = table_info.total_unit_block_num;
    j["table info"]["empty unit block num"] = table_info.empty_unit_block_num;
    j["table info"]["light unit block num"] = table_info.light_unit_block_num;
    j["table info"]["full unit block num"] = table_info.full_unit_block_num;
    j["table info"]["total self num"] = table_info.selftx_num;
    j["table info"]["total send num"] = table_info.sendtx_num;
    j["table info"]["total recv num"] = table_info.recvtx_num;
    j["table info"]["total confirm num"] = table_info.confirmtx_num;
    j["table info"]["total tx v1 num"] = table_info.tx_v1_num;
    j["table info"]["total tx v1 size"] = table_info.tx_v1_total_size;
    if (table_info.tx_v1_num != 0) {
        j["table info"]["tx v1 avg size"] = table_info.tx_v1_total_size / table_info.tx_v1_num;
    }
    j["table info"]["total tx v2 num"] = table_info.tx_v2_num;
    j["table info"]["total tx v2 size"] = table_info.tx_v2_total_size;
    if (table_info.tx_v2_num != 0) {
        j["table info"]["tx v2 avg size"] = table_info.tx_v2_total_size / table_info.tx_v2_num;
    }

    j["confirmed conut"] = table_info.confirmedtx_num;
    j["send only count"] = sendonly.size();
    j["confirmed only count"] = confirmonly.size();
    j["confirmed total time"] = table_info.total_confirm_time_from_send;
    j["confirmed max time"] = table_info.max_confirm_time_from_send;
    j["confirmed avg time"] = float(table_info.total_confirm_time_from_send) / table_info.confirmedtx_num;
    j["confirmed_from_fire total time"] = table_info.total_confirm_time_from_fire;
    j["confirmed_from_fire max time"] = table_info.max_confirm_time_from_fire;
    j["confirmed_from_fire avg time"] = float(table_info.total_confirm_time_from_fire) / table_info.confirmedtx_num;
    j["confirmed detail"] = txs_confirmed_j;
    j["self detail"] = txs_self_j;
    {
        json txs_sendonly_j;
        set_txinfos_to_json(txs_sendonly_j, sendonly);
        j["send only detail"] = txs_sendonly_j;
    }
    {
        json txs_confirmonly_j;
        set_txinfos_to_json(txs_confirmonly_j, confirmonly);
        j["confirmed only detail"] = txs_confirmonly_j;
    }
    {            
        json txs_multi_txs_j;
        set_txinfos_to_json(txs_multi_txs_j, multi_txs);
        j["multi detail"] = txs_multi_txs_j;            
    }
    result_json[account] = j;
    auto t3 = base::xtime_utl::time_now_ms();
    std::cout << "block_height=" << block_height << " t2-t1=" << t2-t1 << " t3-t2=" << t3-t2 << std::endl;
}

void xdb_export_tools_t::query_block_info(std::string const & account, const uint64_t h, xJson::Value & root) {
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
    root = dynamic_cast<chain_info::get_block_handle *>(m_getblock.get())->get_block_json(bp);
}

void xdb_export_tools_t::query_block_basic(std::string const & account, const uint64_t h, json & result) {
    auto block_vec = m_blockstore->load_block_object(account, h).get_vector();
    if (block_vec.empty()) {
        std::cerr << "account: " << account << ", height: " << h << " block null" << std::endl;
        return;
    }
    for (size_t i = 0; i < block_vec.size(); i++) {
        auto const & vblock = block_vec[i];
        if (vblock == nullptr) {
            std::cerr << "account: " << account << ", height: " << h << " block[" << i << "] null" << std::endl;
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
            result[block_id]["hash"] = base::xstring_utl::to_hex(vblock->get_block_hash());
            result[block_id]["last_hash"] = base::xstring_utl::to_hex(vblock->get_last_block_hash());

        } else {
            result["account"] = vblock->get_account();
            result["height"] = vblock->get_height();
            result["class"] = vblock->get_block_class();
            result["viewid"] = vblock->get_viewid();
            result["viewtoken"] = vblock->get_viewtoken();
            result["clock"] = vblock->get_clock();
            result["hash"] = base::xstring_utl::to_hex(vblock->get_block_hash());
            result["last_hash"] = base::xstring_utl::to_hex(vblock->get_last_block_hash());
        }
    }
}

void xdb_export_tools_t::query_balance(std::string const & table, json & j_unit, json & j_table) {
    auto vblock = m_blockstore->get_latest_committed_block(base::xvaccount_t{table});
    if (vblock == nullptr) {
        std::cerr << "table: " << table << ", latest committed block nullptr!" << std::endl;
        j_table = nullptr;
        return;
    }
    auto bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(vblock.get());
    if (bstate == nullptr) {
        std::cerr << "table: " << table << ", latest committed block state constructed failed" << std::endl;
        j_table = nullptr;
        return;
    }
    std::map<std::string, std::string> table_account_index;
    if (false == bstate->find_property(XPROPERTY_TABLE_ACCOUNT_INDEX)) {
        std::cerr << "table: " << table << ", latest committed block state fail-find property XPROPERTY_TABLE_ACCOUNT_INDEX " << XPROPERTY_TABLE_ACCOUNT_INDEX << std::endl;
    } else {
        auto propobj = bstate->load_string_map_var(XPROPERTY_TABLE_ACCOUNT_INDEX);
        table_account_index = propobj->query();
    }
    uint64_t balance = 0;
    uint64_t lock_balance = 0;
    uint64_t tgas_balance = 0;
    uint64_t vote_balance = 0;
    uint64_t burn_balance = 0;
    for (auto const & pair : table_account_index) {
        auto const & account  = pair.first;
        json j;
        auto unit_vblock = m_blockstore->get_latest_committed_block(base::xvaccount_t{account});
        if (unit_vblock == nullptr) {
            std::cerr << "table: " << table << ", unit: " << account << ", latest committed block nullptr!" << std::endl;
            continue;
        }
        auto unit_bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(unit_vblock.get());
        if (unit_bstate == nullptr) {
            std::cerr << "table: " << table << ", unit: " << account << ", latest committed block state constructed failed" << std::endl;
            continue;
        }
        uint64_t unit_balance = 0;
        uint64_t unit_lock_balance = 0;
        uint64_t unit_tgas_balance = 0;
        uint64_t unit_vote_balance = 0;
        uint64_t unit_burn_balance = 0;
        if (unit_bstate->find_property(XPROPERTY_BALANCE_AVAILABLE)) {
            unit_balance = unit_bstate->load_token_var(XPROPERTY_BALANCE_AVAILABLE)->get_balance();
        }
        if (unit_bstate->find_property(XPROPERTY_BALANCE_LOCK)) {
            unit_lock_balance = unit_bstate->load_token_var(XPROPERTY_BALANCE_LOCK)->get_balance();
        }
        if (unit_bstate->find_property(XPROPERTY_BALANCE_PLEDGE_TGAS)) {
            unit_tgas_balance = unit_bstate->load_token_var(XPROPERTY_BALANCE_PLEDGE_TGAS)->get_balance();
        }
        if (unit_bstate->find_property(XPROPERTY_BALANCE_PLEDGE_VOTE)) {
            unit_vote_balance = unit_bstate->load_token_var(XPROPERTY_BALANCE_PLEDGE_VOTE)->get_balance();
        }
        if (unit_bstate->find_property(XPROPERTY_BALANCE_BURN)) {
            unit_burn_balance = unit_bstate->load_token_var(XPROPERTY_BALANCE_BURN)->get_balance();
        }
        j_unit[account][XPROPERTY_BALANCE_AVAILABLE] = unit_balance;
        j_unit[account][XPROPERTY_BALANCE_LOCK] = unit_lock_balance;
        j_unit[account][XPROPERTY_BALANCE_PLEDGE_TGAS] = unit_tgas_balance;
        j_unit[account][XPROPERTY_BALANCE_PLEDGE_VOTE] = unit_vote_balance;
        j_unit[account][XPROPERTY_BALANCE_BURN] = unit_burn_balance;
        balance += unit_balance;
        lock_balance += unit_lock_balance;
        tgas_balance += unit_tgas_balance;
        vote_balance += unit_vote_balance;
        burn_balance += unit_burn_balance;
    }
    j_table[XPROPERTY_BALANCE_AVAILABLE] = balance;
    j_table[XPROPERTY_BALANCE_BURN] = burn_balance;
    j_table[XPROPERTY_BALANCE_LOCK] = lock_balance;
    j_table[XPROPERTY_BALANCE_PLEDGE_TGAS] = tgas_balance;
    j_table[XPROPERTY_BALANCE_PLEDGE_VOTE] = vote_balance;
}

NS_END2
