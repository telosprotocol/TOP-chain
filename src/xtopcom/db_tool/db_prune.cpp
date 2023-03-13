#include "db_prune.h"

#include "xbase/xhash.h"
#include "xblockstore/xblockstore_face.h"
#include "xchain_upgrade/xchain_data_processor.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xcheckpoint.h"
#include "xdata/xcheckpoint.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xrootblock.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xdata/xtable_bstate.h"
#include "xdb/xdb_factory.h"
#include "xelection/xvnode_house.h"
#include "xloader/xgenesis_info.h"
#include "xloader/xconfig_genesis_loader.h"
#include "xrpc/xrpc_query_manager.h"
#include "xstore/xstore_error.h"
#include "xvledger/xvaccount.h"
#include "xvledger/xvdbkey.h"
#include "xvledger/xvledger.h"
#include "xdb/xdb_factory.h"
#include "xvledger/xvaccount.h"
#include "xbase/xhash.h"
#include "xdata/xcheckpoint.h"
#include "xbase/xbase.h"
#include "xstatestore/xstatestore_face.h"

NS_BEG2(top, db_prune)

DbPrune & DbPrune::instance() {
    static DbPrune prune;
    return prune;
}
int DbPrune::db_init(const std::string datadir) {
    /*auto hash_plugin = */new xtop_hash_t();
    data::xrootblock_para_t para;
    data::xrootblock_t::init(para);

    int db_path_num = 0;
    std::vector<db::xdb_path_t> db_data_paths;
    std::string extra_config = datadir + "/.extra_conf.json"; 

    std::ifstream keyfile(extra_config, std::ios::in);
    xinfo("xdb_export_tools_t::read db start extra_config %s", extra_config.c_str());
    if (keyfile) {
        Json::Value key_info_js;
        std::stringstream buffer;
        buffer << keyfile.rdbuf();
        keyfile.close();
        std::string key_info = buffer.str();
        Json::Reader reader;
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
    auto _static_blockstore = store::create_vblockstore(m_store.get());

    m_blockstore.attach(_static_blockstore);
    base::xvchain_t::instance().set_xblockstore(_static_blockstore);
    return 0;
}
int DbPrune::db_close() {
    m_store->close();
    return 1;
}
int DbPrune::update_meta(base::xvaccount_t& _vaddr, const uint64_t& height) {
    std::string meta_key = base::xvdbkey_t::create_account_meta_key(_vaddr);
    const std::string meta_content = m_store->get_value(meta_key);
    base::xvactmeta_t* meta_ptr = base::xvactmeta_t::load(_vaddr, meta_content);
    if (meta_ptr == NULL)
        return 1;

    if (true == meta_ptr->set_latest_deleted_block(height)) {
        std::string meta_bin;
        meta_ptr->serialize_to_string(meta_bin);
        std::string full_meta_path = base::xvdbkey_t::create_account_meta_key(_vaddr);
        m_store->set_value(full_meta_path, meta_bin);
    }
    delete meta_ptr;
    return 0;
}
int DbPrune::db_check(const std::string& node_addr, const std::string& datadir, std::ostringstream& out_str) {
    if (node_addr.empty()) {
        out_str << "please set default account." << std::endl;
        return 1;
    }
    std::cout << "account: " << node_addr << std::endl;
    std::cout << "init db..." << std::endl;
    db_init(datadir);

    std::string value_str;
    int ret = statestore::xstatestore_hub_t::instance()->map_get(rec_registration_contract_address, top::data::system_contract::XPORPERTY_CONTRACT_REG_KEY, node_addr, value_str);

    if (ret == store::xstore_success && !value_str.empty()) {
        data::system_contract::xreg_node_info node_info;
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.c_str(), (uint32_t)value_str.size());

        node_info.serialize_from(stream);

        if (node_info.could_be_archive() || node_info.could_be_exchange() || node_info.genesis()) {
            out_str << "can not prune database at archive node." << std::endl;
            db_close();
            return 1;
        }
        std::cout << "start prune db(" << to_string(node_info.miner_type()) << ")..." << std::endl;
    } else {
        xwarn("[register_node_callback] get node register info fail, node_addr: %s", node_addr.c_str());
        std::cout << "not find account in sys_contract_rec_registration_addr." << std::endl;
        std::cout << "start prune db..." << std::endl;
    }
    return 0;
}
int DbPrune::db_prune(const std::string& node_addr, const std::string& datadir, std::ostringstream& out_str) {
    if (db_check(node_addr, datadir, out_str) != 0) {
        return 1;
    }
    do_db_prune(datadir, "advance", out_str);
    return 0;
}
int DbPrune::db_convert(const std::string& miner_type, const std::string& datadir, std::ostringstream& out_str) {
    std::cout << "db_convert db: " << datadir  << std::endl;
    db_init(datadir);

    do_db_prune(datadir, miner_type, out_str);
    db_close();

    return 0;
}
int DbPrune::do_db_prune(const std::string& datadir, const std::string& miner_type, std::ostringstream& out_str) {
    std::cout << "convert type: " << miner_type << std::endl;
    // init checkpoint
    data::xchain_checkpoint_t::load();

    std::string sys_account[] = {sys_contract_beacon_timer_addr, sys_drand_addr};
    for (size_t i = 0; i < sizeof(sys_account)/sizeof(sys_account[0]); i++) {
       auto vblock = m_blockstore->get_latest_cert_block(sys_account[i]);
        data::xblock_t * block = dynamic_cast<data::xblock_t *>(vblock.get());
        if (block == nullptr || block->get_height() <= 100) {
            //std::cout << " account " << sys_account[i] << " get_latest_cert_block null" << std::endl;
            continue;
        }

        base::xvaccount_t account_obj{sys_account[i]};
        //prune 
        const std::string begin_delete_key = base::xvdbkey_t::create_prunable_block_height_key(account_obj, 1);
        const uint64_t target_height = block->get_height()-100;
        const std::string end_delete_key = base::xvdbkey_t::create_prunable_block_height_key(account_obj, target_height);
        m_store->delete_range(begin_delete_key, end_delete_key);
        update_meta(account_obj, target_height);
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
        update_meta(account_obj, block->get_height());
    }
    
    //prune table
    std::cout << " start table account!" << std::endl;
    auto const tables = get_table_accounts();
    for (auto const & table_account : tables) {
        auto vblock = m_blockstore->get_latest_committed_full_block(table_account);
        data::xblock_t * block = dynamic_cast<data::xblock_t *>(vblock.get());
        if (block == nullptr || block->get_height() < 8) {
            std::cout << " table_account " << table_account << " not exist." << std::endl;
            continue;
        }

        common::xaccount_address_t _vaddress(table_account);
        std::error_code err;
        auto checkpoint = data::xchain_checkpoint_t::get_latest_checkpoint(_vaddress, err);
        if (err) {
            std::cout << "Error: table_account " << table_account << " get_latest_checkpoint " << err << std::endl;
            continue;
        }
        base::xvaccount_t account_obj{table_account};
        const std::string begin_delete_key = base::xvdbkey_t::create_prunable_block_height_key(account_obj, 1);

        uint64_t end_height;
        auto zone_id = account_obj.get_zone_index();
        if (miner_type == "edge" && (zone_id != base::enum_chain_zone_zec_index && zone_id != base::enum_chain_zone_beacon_index)) {
            end_height = block->get_height();
        } else {
            end_height = checkpoint.height;
        }
        const std::string end_delete_key = base::xvdbkey_t::create_prunable_block_height_key(account_obj, end_height);
        std::cout << "prune table: " << table_account << ", " << end_height << std::endl;
        m_store->delete_range(begin_delete_key, end_delete_key);
        update_meta(account_obj, end_height);
    }
    out_str << "prune database ok." << std::endl;
    db_close();
    return 0;  
}
std::vector<std::string> DbPrune::get_db_unit_accounts() {
    std::set<std::string> accounts;
    auto const tables = get_table_accounts();
    for (auto const & table : tables) {
        auto latest_block = m_blockstore->get_latest_committed_block(table);
        if (latest_block == nullptr) {
            std::cerr << table << " not exist." << std::endl;
            continue;
        }
        std::error_code ec;
        auto table_accounts = statestore::xstatestore_hub_t::instance()->get_all_accountindex(latest_block.get(), ec);
        if (ec) {
            std::cerr << table << " get_block_state null!" << std::endl;
            continue;
        }
        for (auto & v : table_accounts) {
            accounts.insert(v.first.to_string());
        }
    }

    std::vector<std::string> v;
    v.assign(accounts.begin(), accounts.end());

    std::cout << "total " << v.size() << " units in db" << std::endl;
    return v;
}
std::vector<std::string> DbPrune::get_table_accounts() {
    std::vector<std::string> v;
    const std::vector<std::pair<std::string, int>> table = {
        std::make_pair(common::con_table_base_address.to_string(), enum_vledger_const::enum_vbucket_has_tables_count),
        std::make_pair(common::zec_table_base_address.to_string(), MAIN_CHAIN_ZEC_TABLE_USED_NUM),
        std::make_pair(common::rec_table_base_address.to_string(), MAIN_CHAIN_REC_TABLE_USED_NUM),
    };
    for (auto const & t : table) {
        for (auto i = 0; i < t.second; i++) {
            v.emplace_back(data::make_address_by_prefix_and_subaddr(t.first, uint16_t(i)).to_string());
        }
    }
    return v;
}
void DbPrune::compact_db(const std::string datadir, std::ostringstream& out_str) {
    std::cout << "compact db: " << datadir << std::endl;
    db_init(datadir);

    std::string begin_key;
    std::string end_key;
    m_store->compact_range(begin_key, end_key);
    out_str << "compact database ok." << std::endl;
    db_close();
}
NS_END2