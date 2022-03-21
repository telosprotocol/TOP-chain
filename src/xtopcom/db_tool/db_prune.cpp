#include "db_prune.h"

#include "xbasic/xasio_io_context_wrapper.h"
#include "xblockstore/xblockstore_face.h"
#include "xchain_upgrade/xchain_data_processor.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xrootblock.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xdata/xtable_bstate.h"
#include "xdepends/include/asio/post.hpp"
#include "xdepends/include/asio/thread_pool.hpp"
#include "xelection/xvnode_house.h"
#include "xloader/src/xgenesis_info.h"
#include "xloader/xconfig_genesis_loader.h"
#include "xrpc/xgetblock/get_block.h"
#include "xvledger/xvdbkey.h"
#include "xvledger/xvledger.h"
#include "xvm/manager/xcontract_manager.h"
#include "xvledger/xvdbkey.h"
#include "xdb/xdb_factory.h"
#include "xvledger/xvaccount.h"
#include "xbase/xhash.h"
#include "xdata/xcheckpoint.h"

#include "xconfig/xconfig_register.h"
#include "xdata/xrootblock.h"


NS_BEG2(top, db_prune)
#define NODE_ID "T00000LgGPqEpiK6XLCKRj9gVPN8Ej1aMbyAb3Hu"
#define SIGN_KEY "ONhWC2LJtgi9vLUyoa48MF3tiXxqWf7jmT9KtOg/Lwo="

DbPrune & DbPrune::instance() {
    static DbPrune prune;
    return prune;
}
int DbPrune::db_init(const std::string datadir) {
    auto hash_plugin = new xtop_hash_t();
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

    int db_path_num = 0;
    std::vector<db::xdb_path_t> db_data_paths;
    std::string extra_config = datadir + "/.extra_conf.json"; 

    std::ifstream keyfile(extra_config, std::ios::in);
    xinfo("xdb_export_tools_t::read db start extra_config %s", extra_config.c_str());
    if (keyfile) {
        xJson::Value key_info_js;
        std::stringstream buffer;
        buffer << keyfile.rdbuf();
        keyfile.close();
        std::string key_info = buffer.str();
        xJson::Reader reader;
        // ignore any error when parse
        reader.parse(key_info, key_info_js);
        if (key_info_js["db_path_num"] > 1) {   
            db_path_num = key_info_js["db_path_num"].asInt();
            for (int i = 0; i < db_path_num; i++) {
                std::string key_db_path = "db_path_" + std::to_string(i+1);
                std::string key_db_size = "db_path_size_" + std::to_string(i+1);
                std::string db_path_result =  key_info_js[key_db_path].asString();
                uint64_t db_size_result = key_info_js[key_db_size].asUInt64(); 
                if (db_path_result.empty() || db_size_result < 1) {
                    db_path_num = 1;
                    xwarn("xtop_application::read db %i path %s size %lld config failed!", i , db_path_result.c_str(), db_size_result);
                    break;
                }
                xinfo("xtop_application::read db  %i path %s size %lld sucess!",i , db_path_result.c_str(), db_size_result);
                db_data_paths.emplace_back(db_path_result, db_size_result);
            }
        }
    }
    
    std::shared_ptr<db::xdb_face_t> db;
    std::string db_path = datadir + DB_PATH;
    if (db_path_num > 1)    {
        db = db::xdb_factory_t::instance(db_path, db_data_paths);
    } else {
        db = db::xdb_factory_t::instance(db_path);
    }
    m_store = top::store::xstore_factory::create_store_with_static_kvdb(db);
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
    return 0;
}

int DbPrune::db_prune(const std::string datadir, std::ostringstream& out_str) {
    std::cout << "init db..." <<std::endl;
    db_init(datadir);

    std::cout << "start prune db..." <<std::endl;
    // init checkpoint
    data::xchain_checkpoint_t::load();

    std::string sys_account[] = {sys_contract_beacon_timer_addr, sys_drand_addr};
    for (size_t i = 0; i < sizeof(sys_account)/sizeof(sys_account[0]); i++) {
       auto vblock = m_blockstore->get_latest_cert_block(sys_account[i]);
        data::xblock_t * block = dynamic_cast<data::xblock_t *>(vblock.get());
        if (block == nullptr || block->get_height() < 8) {
            //std::cout << " account " << sys_account[i] << " get_latest_cert_block null" << std::endl;
            continue;
        }

        base::xvaccount_t account_obj{sys_account[i]};
        //prune 
        const std::string begin_delete_key = base::xvdbkey_t::create_prunable_block_height_key(account_obj, 1);
        const uint64_t target_height = block->get_height()-100;
        const std::string end_delete_key = base::xvdbkey_t::create_prunable_block_height_key(account_obj, target_height);
        m_store->delete_range(begin_delete_key, end_delete_key);
    }

    //prune account
    auto const unit_account_vec = get_db_unit_accounts();
    std::cout << " start prune unit account!" << std::endl;
    for (auto unit_account: unit_account_vec) {
        if (data::is_sys_contract_address(common::xaccount_address_t{ unit_account })) {
            continue;
        }
        auto vblock = m_blockstore->get_latest_committed_full_block(unit_account);
        data::xblock_t * block = dynamic_cast<data::xblock_t *>(vblock.get());
        if (block == nullptr || block->get_height() < 8) {
           // std::cout << " account " << unit_account << " get_latest_committed_full_block null" <<  << std::endl;
            continue;
        }

        base::xvaccount_t account_obj{unit_account};
        //prune 
        const std::string begin_delete_key = base::xvdbkey_t::create_prunable_block_height_key(account_obj, 1);
        const std::string end_delete_key = base::xvdbkey_t::create_prunable_block_height_key(account_obj, block->get_height());
        m_store->delete_range(begin_delete_key, end_delete_key);
    }
    
    //prune table
    std::cout << " start table account!" << std::endl;
    auto const tables = get_table_accounts();
    for (auto const table_account : tables) {
        auto vblock = m_blockstore->get_latest_committed_full_block(table_account);
        data::xblock_t * block = dynamic_cast<data::xblock_t *>(vblock.get());
        if (block == nullptr || block->get_height() < 8) {
            std::cout << " table_account " << table_account << " get_latest_committed_full_block null" << std::endl;
            continue;
        }

        common::xaccount_address_t _vaddress(table_account);
        std::error_code err;
        auto checkpoint = xtop_chain_checkpoint::get_latest_checkpoint(_vaddress, err);
        if (err) {
            //std::cout << "Error: table_account " << table_account << " get_latest_checkpoint " << err << std::endl;
            continue;
        }
        base::xvaccount_t account_obj{table_account};
        const std::string begin_delete_key = base::xvdbkey_t::create_prunable_block_height_key(account_obj, 1);
        const std::string end_delete_key = base::xvdbkey_t::create_prunable_block_height_key(account_obj, checkpoint.height);
        m_store->delete_range(begin_delete_key, end_delete_key);
    }

    std::string begin_key;
    std::string end_key;
    m_store->compact_range(begin_key, end_key);  
    return 0;  
}
std::vector<std::string> DbPrune::get_db_unit_accounts() {
    std::set<std::string> accounts;
    auto const tables = get_table_accounts();
    for (auto const table : tables) {
        auto latest_block = m_blockstore->get_latest_committed_block(table);
        if (latest_block == nullptr) {
            std::cerr << table << " get_latest_committed_block null!" << std::endl;
            continue;
        }
        base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(latest_block.get());
        if (bstate == nullptr) {
            std::cerr << table << " get_block_state null!" << std::endl;
            continue;
        }
        auto table_state = std::make_shared<xtable_bstate_t>(bstate.get());
        auto const & units = table_state->get_all_accounts();
        accounts.insert(units.cbegin(), units.cend());
    }

    std::vector<std::string> v;
    v.assign(accounts.begin(), accounts.end());

    std::cout << "total " << v.size() << " units in db" << std::endl;
    return v;
}
std::vector<std::string> DbPrune::get_table_accounts() {
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
NS_END2