#include "dirent.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../xdb_export.h"
#include "xdbstore/xstore.h"
#include "xdbstore/xstore_face.h"
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
#include "xdata/xblocktool.h"
#include "xdepends/include/asio/post.hpp"
#include "xdepends/include/asio/thread_pool.hpp"
#include "xelection/xvnode_house.h"
#include "xevm_common/trie/xtrie.h"
#include "xevm_common/trie/xtrie_iterator.h"
#include "xevm_common/trie/xtrie_kv_db.h"
#include "xevm_common/trie/xsecure_trie.h"
#include "xstate_mpt/xstate_mpt.h"
#include "xloader/src/xgenesis_info.h"
#include "xloader/xconfig_genesis_loader.h"
#include "xvledger/xvdbkey.h"
#include "xrpc/xrpc_query_manager.h"
#include "xvledger/xvledger.h"
#include "xvm/manager/xcontract_manager.h"
#include "xvledger/xvdbkey.h"
#include "xdb/xdb_factory.h"
#include "xvledger/xvaccount.h"
#include "xbase/xhash.h"
#include "xbase/xutl.h"
#include "xdata/xcheckpoint.h"
#include "xpbase/base/top_utils.h"
#include "../xerror.h"
#include "xbasic/xutility.h"
#include "xdata/xunit_bstate.h"

#define NODE_ID "T00000LgGPqEpiK6XLCKRj9gVPN8Ej1aMbyAb3Hu"
#define SIGN_KEY "ONhWC2LJtgi9vLUyoa48MF3tiXxqWf7jmT9KtOg/Lwo="

NS_BEG2(top, db_export)

xdb_export_tools_t::xdb_export_tools_t(std::string const & db_path) {
    XMETRICS_INIT();
    auto io_obj = std::make_shared<xbase_io_context_wrapper_t>();
    m_timer_driver = make_unique<xbase_timer_driver_t>(io_obj);
    m_bus = top::make_object_ptr<mbus::xmessage_bus_t>(true, 1000);

    int dst_db_kind = top::db::xdb_kind_kvdb;
    std::vector<db::xdb_path_t> db_data_paths {};
    base::xvchain_t::instance().get_db_config_custom(db_data_paths, dst_db_kind);
    std::cout << "--------db_path:" << db_path << std::endl;
    std::shared_ptr<db::xdb_face_t> db = top::db::xdb_factory_t::create(dst_db_kind, db_path, db_data_paths);

    m_store = top::store::xstore_factory::create_store_with_static_kvdb(db);
   // m_store = top::store::xstore_factory::create_store_with_kvdb(db_path);
    base::xvchain_t::instance().set_xdbstore(m_store.get());
    base::xvchain_t::instance().set_xevmbus(m_bus.get());
    m_blockstore.attach(store::get_vblockstore());
    m_txstore = xobject_ptr_t<base::xvtxstore_t>(
        txstore::create_txstore(top::make_observer<mbus::xmessage_bus_face_t>(m_bus.get()),
                                top::make_observer<xbase_timer_driver_t>(m_timer_driver.get())));
    base::xvchain_t::instance().set_xtxstore(m_txstore.get());
    m_nodesvr_ptr = make_object_ptr<election::xvnode_house_t>(common::xnode_id_t{NODE_ID}, m_blockstore, make_observer(m_bus.get()));
    m_getblock =
        std::make_shared<xrpc::xrpc_query_manager>(observer_ptr<base::xvblockstore_t>(m_blockstore.get()), nullptr, nullptr);
    contract::xcontract_manager_t::instance().init(xobject_ptr_t<store::xsyncvstore_t>{});
    contract::xcontract_manager_t::set_nodesrv_ptr(m_nodesvr_ptr);
}

xdb_export_tools_t::~xdb_export_tools_t() {
    m_store->close();
}

std::vector<std::string> xdb_export_tools_t::get_system_contract_accounts() {
    std::vector<std::string> v;
    const std::vector<std::string> unit = {
        sys_contract_rec_registration_addr,
        sys_contract_rec_elect_edge_addr,
        sys_contract_rec_elect_archive_addr,
        sys_contract_rec_elect_exchange_addr,
        sys_contract_rec_elect_rec_addr,
        sys_contract_rec_elect_zec_addr,
        sys_contract_rec_elect_fullnode_addr,
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
            v.emplace_back(data::make_address_by_prefix_and_subaddr(t, uint16_t(i)).to_string());
        }
    }
    return v;
}

std::vector<std::string> xdb_export_tools_t::get_table_accounts() {
    return data::xblocktool_t::make_all_table_addresses();
}

std::vector<std::string> xdb_export_tools_t::get_db_unit_accounts() {
    std::set<std::string> accounts;
    auto const tables = get_table_accounts();
    for (auto const table : tables) {
        auto latest_block = m_blockstore->get_latest_committed_block(table);
        if (latest_block == nullptr) {
            std::cerr << table << " get_latest_committed_block null!" << std::endl;
            continue;
        }

        std::error_code ec;
        auto const & state_root = data::xblockextract_t::get_state_root(latest_block.get(), ec);
        if (state_root.empty()) {
            base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(latest_block.get());
            if (bstate == nullptr) {
                std::cerr << table << " get_block_state null!" << std::endl;
                continue;
            }
            auto table_state = std::make_shared<data::xtable_bstate_t>(bstate.get());
            auto const & units = table_state->get_all_accounts();
            accounts.insert(units.cbegin(), units.cend());
        } else {
            auto const kv_db = std::make_shared<evm_common::trie::xkv_db_t>(base::xvchain_t::instance().get_xdbstore(), common::xaccount_address_t(table));
            auto xtrie_db = evm_common::trie::xtrie_db_t::NewDatabase(kv_db);
            auto const & leafs = top::evm_common::trie::xtrie_simple_iterator_t::trie_leafs(state_root, make_observer(xtrie_db));
            for (auto & leaf : leafs) {
                state_mpt::xaccount_info_t info;
                info.decode({leaf.begin(), leaf.end()});
                accounts.insert(info.m_account.to_string());
            }
        }
    }

    std::vector<std::string> v;
    v.assign(accounts.begin(), accounts.end());

    std::cout << "total " << v.size() << " units in db" << std::endl;
    return v;
}

void xdb_export_tools_t::query_all_account_data(std::vector<std::string> const & accounts_vec, bool is_table, const xdb_check_data_func_face_t & func) {
    json result_json;
    uint32_t thread_num = 8;
    if (accounts_vec.size() < thread_num) {
        for (auto const & account : accounts_vec) {
            json j;
            query_account_data(account, j, func);
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
        auto thread_helper = [&accounts_vec_split, &j_vec, &func](xdb_export_tools_t * arg, int index) {
            for (auto const & account : accounts_vec_split[index]) {
                json j;
                arg->query_account_data(account, j, func);
                j_vec[index][account] = j;
                std::cout << account << " " << func.data_type() << " query finish: " << j.get<std::string>() << std::endl;
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
        filename = accounts_vec[0] + "_" + func.data_type() + ".json";
    } else {
        if (is_table) {
            filename = "all_table_" + func.data_type() + ".json";
        } else {
            filename = "all_unit_" + func.data_type() + ".json";
        }
    }
    std::ofstream out_json(filename);
    out_json << std::setw(4) << result_json;
    std::cout << "===> " << filename << " generated success!" << std::endl;
}

void xdb_export_tools_t::query_all_table_mpt(std::vector<std::string> const & accounts_vec) {
    json result_json;
    for (auto const & account : accounts_vec) {
        json j;
        query_table_mpt(account, j);
        result_json[account] = j;
        std::cout << account << " mpt query finish: " << j.get<std::string>() << std::endl;
    }

    std::string filename = "all_table_mpt.json";
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

std::string get_ave_string(uint64_t numerator, uint64_t denominator) {
    float ave = ((float)numerator)/((float)denominator);
    std::string ave_str = std::to_string(ave);
    return ave_str.substr(0, ave_str.find(".") + 3);
}

void xdb_export_tools_t::query_table_performance(std::string const & account) {
    json root;
    auto const h = m_blockstore->get_latest_committed_block_height(base::xvaccount_t{account});
    auto const vblock1 = m_blockstore->load_block_object(account, 1, 0, false);
    if (vblock1 == nullptr) {
        std::cerr << "account: " << account << ", height: " << h << " block null" << std::endl;
        return;
    }
    const data::xblock_t * block1 = dynamic_cast<data::xblock_t *>(vblock1.get());
    uint64_t last_viewid = block1->get_viewid();
    uint64_t last_gmtime = block1->get_second_level_gmtime();
    auto input_action1 = data::xblockextract_t::unpack_txactions(vblock1.get());
    uint32_t tx_num1 = input_action1.size();

    std::string result1 = "viewid " + std::to_string(last_viewid) + " intval " + std::to_string(0) + " tx " + std::to_string(tx_num1);
    root["h1"] = result1;

    std::string no_blocks_viewid;
    uint32_t no_blocks_viewid_num = 0;
    uint32_t tx_num_total = tx_num1;

    std::string tps_per_10_blocks;
    uint32_t tx_num_in_10_blocks = tx_num1;
    uint64_t gmttime_start_for_10_blocks = last_gmtime;
    for (size_t i = 2; i <= h; i++) {
        auto const vblock = m_blockstore->load_block_object(account, i, 0, false);
        if (vblock == nullptr) {
            std::cerr << "account: " << account << ", height: " << i << " block null" << std::endl;
            return;
        }

        const data::xblock_t * block = dynamic_cast<data::xblock_t *>(vblock.get());
        uint64_t gmtime = block->get_second_level_gmtime();
        uint64_t inteval = (gmtime <= last_gmtime) ? 0 : (gmtime - last_gmtime);
        auto input_actions = data::xblockextract_t::unpack_txactions(vblock.get());
        uint32_t tx_num = input_actions.size();
        uint64_t viewid = block->get_viewid();

        std::string result = "viewid " + std::to_string(viewid) + " intval " + std::to_string(inteval) + " tx " + std::to_string(tx_num);
        root["h" + std::to_string(i)] = result;

        if (viewid != last_viewid + 1) {
            for (uint64_t id = last_viewid + 1; id < viewid; id++) {
                no_blocks_viewid += std::to_string(id)+ ',';
            }
            no_blocks_viewid_num += (viewid - last_viewid - 1);
        }
        last_viewid = viewid;
        last_gmtime = gmtime;
        tx_num_total += tx_num;

        tx_num_in_10_blocks += tx_num;
        if (i%10 == 0) {
            tps_per_10_blocks += get_ave_string(tx_num_in_10_blocks, gmtime - gmttime_start_for_10_blocks) + ", ";
            tx_num_in_10_blocks = 0;
            gmttime_start_for_10_blocks = gmtime;
        }
    }
    root["no_blocks_viewid"] = no_blocks_viewid;
    root["no_blocks_viewid_num"] = std::to_string(no_blocks_viewid_num);
    root["tx_num_total"] = std::to_string(tx_num_total);
    root["tx_num_per_block"] = get_ave_string(tx_num_total, h);
    root["tps_per_10_blocks"] = tps_per_10_blocks;
    generate_json_file(std::string{account + "_perfromance.json"}, root);
}

void xdb_export_tools_t::query_all_table_performance(std::vector<std::string> const & accounts_vec) {
    for (auto const & account : accounts_vec) {
        query_table_performance(account);
        std::cout << account << " table performance query finish" << std::endl;
    }
}

void GetFiles(const std::string& img_dir_path,std::vector<std::string> &img_file_paths)
{
    DIR* dir;
    if ((dir = opendir(img_dir_path.c_str())) == nullptr) {
        throw std::runtime_error("directory " + img_dir_path + " does not exist");
    }
    dirent* dp;
    for (dp = readdir(dir); dp != nullptr; dp = readdir(dir)) {
        const std::string img_file_name = dp->d_name;
        std::cout << "d_name " << img_file_name << std::endl;
        if (img_file_name == "." || img_file_name == "..") {
            continue;
        }
        img_file_paths.push_back(img_dir_path + "/" + img_file_name);
    }
    closedir(dir);

    std::sort(img_file_paths.begin(), img_file_paths.end());
}

void xdb_export_tools_t::read_external_tx_firestamp() {
    std::vector<std::string> files;
    GetFiles("./tx_fire_files", files);

    for (auto & tx_file : files) {
        std::ifstream ifs;
        ifs.open(tx_file.c_str(), std::ifstream::in);
        char data[256];
        while (1) {
            ifs.getline(data, 256);
            if (data[0] == 0)
                break;
            std::vector<std::string> values;
            base::xstring_utl::split_string(std::string(data), '\t', values);
            assert(values.size() == 2);

            std::string hash = values[0];
            if (hash.substr(0,2) == "0x" || hash.substr(0,2) == "0X")
                hash = hash.substr(2);
            hash = HexDecode(hash);
            m_txs_fire_timestamp[hash] = std::strtoul(values[1].c_str(), NULL, 0);
        }
        ifs.close();
    }
    std::cout << "read tx_file: " << m_txs_fire_timestamp.size() << std::endl;

    sleep(5);
}

void xdb_export_tools_t::query_tx_info(std::vector<std::string> const & tables, const uint32_t thread_num, const uint32_t start_timestamp, const uint32_t end_timestamp) {
    uint32_t threads = 0;
    std::cout << "start_timestamp: " << start_timestamp << ", end_timestamp: " << end_timestamp << std::endl;
    if (thread_num != 0) {
        threads = thread_num;
        std::cout << "use thread num: " << threads << std::endl;
    } else {
        // default
        threads = 4;
        std::cout << "use default thread num: " << threads << std::endl;
    }
    asio::thread_pool pool(threads);
    for (size_t i = 0; i < tables.size(); i++) {
        asio::post(pool, std::bind(&xdb_export_tools_t::query_tx_info_internal, this, tables[i], start_timestamp, end_timestamp));
    }
    pool.join();

    print_all_table_txinfo_to_file();
}

void xdb_export_tools_t::query_block_exist(std::string const & address, const uint64_t height) {
    auto const block_vec = m_blockstore->load_block_object(address, height).get_vector();
    std::cout << "account: " << address << " , height: " << height << " , block exist, total num: " << block_vec.size() << std::endl;
    for (auto const & block : block_vec) {
        if (block != nullptr) {
            std::cout << block->dump2() ;//<< std::endl;
            printf("real-flags=0x%x\n", (int32_t)block->get_block_flags());
        } else {
            std::cerr << "exist one null block!!!" << std::endl;
        }
    }

    auto const block_bindex_vec = m_blockstore->load_block_index(address, height).get_vector();
    // std::cout << "account: " << address << " , height: " << height << " , block exist, total num: " << block_vec.size() << std::endl;
    for (auto const & block : block_bindex_vec) {
        if (block != nullptr) {
            std::cout << block->dump() ;//<< std::endl;
            printf("real-flags=0x%x\n", (int32_t)block->get_block_flags());
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
    auto const h = m_blockstore->get_latest_committed_block_height(base::xvaccount_t{account});
    if (param == "last") {
        query_block_basic(account, h, root);
    } else if (param != "all") {
        uint64_t h = std::stoi(param);
        query_block_basic(account, h, root);
    } else {
        for (size_t i = 0; i <= h; i++) {
            query_block_basic(account, i, root["height" + std::to_string(i)]);
        }
    }
    generate_json_file(std::string{account + "_all_block_basic.json"}, root);
}

void xdb_export_tools_t::query_block_basic(std::string const & account, const uint64_t h, json & result) {
    auto const vblock = m_blockstore->load_block_object(account, h, 0, false);
    if (vblock == nullptr) {
        std::cerr << "account: " << account << ", height: " << h << " block null" << std::endl;
        return;
    }
    result["account"] = vblock->get_account();
    result["height"] = vblock->get_height();
    result["class"] = vblock->get_block_class();
    result["viewid"] = vblock->get_viewid();
    result["viewtoken"] = vblock->get_viewtoken();
    result["clock"] = vblock->get_clock();
    result["hash"] = base::xstring_utl::to_hex(vblock->get_block_hash());
    result["last_hash"] = base::xstring_utl::to_hex(vblock->get_last_block_hash());
    auto txactions = data::xblockextract_t::unpack_txactions(vblock.get());
    for (auto & txaction : txactions) {
        result["tx"].push_back(base::xstring_utl::to_hex(txaction.get_org_tx_hash()));
    }
}

void xdb_export_tools_t::query_state_basic(std::vector<std::string> const & account_vec, std::string const & param) {
    for (auto const & account : account_vec) {
        query_state_basic(account, param);
    }
}

void xdb_export_tools_t::query_state_basic(std::string const & account, std::string const & param) {
    json root;
    auto const h = m_blockstore->get_latest_committed_block_height(base::xvaccount_t{account});
    if (param == "last") {
        query_state_basic(account, h, root);
    } else if (param != "all") {
        uint64_t h = std::stoi(param);
        query_state_basic(account, h, root);
    } else {
        for (size_t i = 0; i <= h; i++) {
            query_state_basic(account, i, root["height" + std::to_string(i)]);
        }
    }
    generate_json_file(std::string{account + "_all_state_basic.json"}, root);
}

void xdb_export_tools_t::query_state_basic(std::string const & account, const uint64_t h, json & result) {
    auto const vblock = m_blockstore->load_block_object(account, h, 0, false);
    if (vblock == nullptr) {
        std::cout << "account: " << account << ", height: " << h << " block null" << std::endl;
        return;
    }
    auto const bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(vblock.get());
    if (bstate == nullptr) {
        std::cout << "account: " << account << ", height: " << h << " state null" << std::endl;
        return;
    }
    result["account"] = bstate->get_account();
    result["height"] = bstate->get_block_height();
    result["class"] = bstate->get_block_class();
    result["viewid"] = bstate->get_block_viewid();
    result["last_hash"] = base::xstring_utl::to_hex(bstate->get_last_block_hash());
    property_json(bstate, result["property"]);
}

void xdb_export_tools_t::query_meta(std::vector<std::string> const & account_vec) {
    json root;
    for (auto const & account : account_vec) {
        query_meta(account, root[account]);
    }

    auto const unit_account_vec = get_db_unit_accounts();
    for (auto const & account : unit_account_vec) {
        query_meta(account, root[account]);
    }
    generate_json_file(std::string{"all_meta_data.json"}, root);
}

void xdb_export_tools_t::query_meta(std::string const & account) {
    json root;
    query_meta(account, root);
    generate_json_file(std::string{account + "_meta_data.json"}, root);
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
    result["block_meta"]["highest_cp_connect_block_height"] = block_meta._highest_cp_connect_block_height;
    result["block_meta"]["highest_cp_connect_block_hash"] = base::xstring_utl::to_hex(block_meta._highest_cp_connect_block_hash);
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
    result["sync_meta"]["highest_genesis_connect_hash"] = base::xstring_utl::to_hex(sync_meta._highest_genesis_connect_hash);
    result["sync_meta"]["highest_sync_height"] = sync_meta._highest_sync_height;
}

void xdb_export_tools_t::query_table_unit_info(std::vector<std::string> const & account_vec) {
    load_db_unit_accounts_info();

    const uint32_t thread_num = 4;
    uint32_t accounts_per_thread = account_vec.size() / thread_num;
    std::vector<std::vector<std::string>> account_vec_split;
    for (size_t i = 0; i < thread_num; i++) {
        uint32_t start_index = i * accounts_per_thread;
        uint32_t end_index = (i == (thread_num - 1)) ? account_vec.size() : ((i + 1) * accounts_per_thread);
        std::vector<std::string> thread_accounts;
        for (auto j = start_index; j < end_index; j++) {
            thread_accounts.emplace_back(account_vec[j]);
        }
        account_vec_split.emplace_back(thread_accounts);
    }
    auto thread_helper = [&account_vec_split](xdb_export_tools_t * arg, int index) {
        for (auto const & account : account_vec_split[index]) {
            arg->query_table_unit_info(account);
        }
    };
    std::vector<std::thread> all_thread;
    for (size_t i = 0; i < thread_num; i++) {
        std::thread th(thread_helper, this, i);
        all_thread.emplace_back(std::move(th));
    }
    for (size_t i = 0; i < thread_num; i++) {
        all_thread[i].join();
    }

    std::set<std::string> genesis_only;
    auto const accounts_set = get_db_unit_accounts_v2();
    auto const contracts = get_system_contract_accounts();
    for (auto const & contract : contracts) {
        if (!accounts_set.count(contract)) {
            genesis_only.insert(contract);
        }
    }
    std::vector<chain_data::data_processor_t> reset_data;
    chain_data::xchain_data_processor_t::get_all_user_data(reset_data);
    for (auto const & user : reset_data) {
        if (!accounts_set.count(user.address)) {
            genesis_only.insert(user.address);
        }
    }
    auto genesis_loader = std::make_shared<loader::xconfig_genesis_loader_t>("{}");
    data::xrootblock_para_t rootblock_para;
    genesis_loader->extract_genesis_para(rootblock_para);
    auto const genesis_accounts = rootblock_para.m_account_balances;
    for (auto const & account : genesis_accounts) {
        if (!accounts_set.count(account.first)) {
            genesis_only.insert(account.first);
        }
    }
    auto const seed_nodes = rootblock_para.m_genesis_nodes;
    for (auto const & node : seed_nodes) {
        if (!accounts_set.count(node.m_account.to_string())) {
            genesis_only.insert(node.m_account.to_string());
        }
    }

    for (auto const account : genesis_only) {
        json root_unit;
        query_block_basic(account, 0, root_unit["block0"]);
        query_state_basic(account, 0, root_unit["state"]);
        query_meta(account, root_unit["meta"]);
        generate_json_file(std::string{account + "_basic_info.json"}, root_unit);
    }
}

void xdb_export_tools_t::query_table_unit_info(std::string const & account) {
    json root;
    auto const h = m_blockstore->get_latest_committed_block_height(base::xvaccount_t{account});
    for (size_t i = 0; i <= h; i++) {
        query_block_basic(account, i, root["block" + std::to_string(i)]);
    }
    query_state_basic(account, h, root["state"]);
    query_meta(account, root["meta"]);
    generate_json_file(std::string{account + "_basic_info.json"}, root);

    auto const vblock = m_blockstore->load_block_object(account, h, 0, false);
    if (vblock == nullptr) {
        std::cerr << "account: " << account << ", height: " << h << " block null" << std::endl;
        return;
    }
    auto const bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(vblock.get());
    if (bstate == nullptr) {
        std::cerr << "account: " << account << ", height: " << h << " state null" << std::endl;
        return;
    }
    auto const table_state = std::make_shared<data::xtable_bstate_t>(bstate.get());
    auto const units = table_state->get_all_accounts();
    for (auto const & unit : units) {
        json root_unit;
        base::xaccount_index_t index;
        if (table_state->get_account_index(unit, index) == false) {
            std::cout << "account: " << unit << " get index failed " << std::endl;
            continue;
        }
        auto const h_unit = index.get_latest_unit_height();
        for (size_t i = 0; i <= h_unit; i++) {
            query_block_basic(unit, i, root_unit["block" + std::to_string(i)]);
        }
        query_state_basic(unit, h_unit, root_unit["state"]);
        query_meta(unit, root_unit["meta"]);
        generate_json_file(std::string{unit + "_basic_info.json"}, root_unit);
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
            table_balance = j_table[data::XPROPERTY_BALANCE_AVAILABLE].get<uint64_t>();
            table_lock_balance = j_table[data::XPROPERTY_BALANCE_LOCK].get<uint64_t>();
            table_tgas_balance = j_table[data::XPROPERTY_BALANCE_PLEDGE_TGAS].get<uint64_t>();
            table_vote_balance = j_table[data::XPROPERTY_BALANCE_PLEDGE_VOTE].get<uint64_t>();
            table_burn_balance = j_table[data::XPROPERTY_BALANCE_BURN].get<uint64_t>();
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
    if (reward_bstate != nullptr && true == reward_bstate->find_property(data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE)) {
        auto map = reward_bstate->load_string_map_var(data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE)->query();
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

void xdb_export_tools_t::query_archive_db(const uint32_t redundancy) {
    std::string filename = "check_archive_db.log";
    std::ofstream file(filename);
    uint32_t total_errors{0};
    std::cout << "redundancy = " << redundancy << std::endl;
    // step 1: check table
    std::cout << "step 1 ===> checking table accounts..." << std::endl;
    auto t1 = base::xtime_utl::time_now_ms();
    {
        asio::thread_pool pool(8);
        auto const tables = xdb_export_tools_t::get_table_accounts();
        for (size_t i = 0; i < tables.size(); i++) {
            asio::post(pool, std::bind(&xdb_export_tools_t::query_archive_db_internal, this, tables[i], query_account_table, redundancy, std::ref(file), std::ref(total_errors)));
        }
        pool.join();
    }
    auto t2 = base::xtime_utl::time_now_ms();
    std::cout << "time1: " << (t2 - t1) / 1000 << "s." << std::endl;

    // step 2: check unit
    std::cout << "step 2 ===> checking unit accounts..." << std::endl;
    {
        asio::thread_pool pool(8);
        auto const units = get_db_unit_accounts();
        for (size_t i = 0; i < units.size(); i++) {
            asio::post(pool, std::bind(&xdb_export_tools_t::query_archive_db_internal, this, units[i], query_account_unit, 0, std::ref(file), std::ref(total_errors)));
        }
        pool.join();
    }
    auto t3 = base::xtime_utl::time_now_ms();
    std::cout << "time2: " << (t3 - t2) / 1000 << "s." << std::endl;

    // step 3: check drand
    std::cout << "step 3 ===> checking drand..." << std::endl;
    {
        query_archive_db_internal(sys_drand_addr, query_account_system, redundancy, file, total_errors);
    }
    file.close();
    auto t4 = base::xtime_utl::time_now_ms();
    std::cout << "time3: " << (t4 - t3) / 1000 << "s." << std::endl;

    if (total_errors != 0) {
        std::cerr << "total error num: " << total_errors << std::endl;
        std::string filename_error = "check_archive_db_error.log";
        std::ofstream file_error(filename_error);
        file_error << "[error] " << total_errors  << std::endl;
        file_error.close();
    } else {
        generate_common_file("success", {});
    }
}

void xdb_export_tools_t::query_archive_db_internal(std::string const & account, enum_query_account_type type, const uint32_t redundancy, std::ofstream & file, uint32_t & errors) {
    uint32_t error_num = 0;
    std::string type_str;
    if (type == query_account_table) {
        type_str = "table";
    } else if (type == query_account_unit) {
        type_str = "unit";
    } else {
        type_str = "system";
    }
    do {
        auto const committd_height = m_blockstore->get_latest_committed_block_height(account);
        auto const connected_height = m_blockstore->get_latest_connected_block_height(account);
        auto const genesis_height_str = m_blockstore->get_genesis_height(account);
        uint64_t span_genesis_height = 0;
        if (!genesis_height_str.empty()) {
            base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)genesis_height_str.c_str(), genesis_height_str.size());
            stream >> span_genesis_height;
        }
        auto latest_cert_block = m_blockstore->get_latest_cert_block(account);
        if (latest_cert_block == nullptr) {
            file << "[warn] " << type_str << ": " << account << ", latest cert block is nullptr!" << std::endl;
            error_num++;
            break;
        }
        auto const latest_cert_height = latest_cert_block->get_height();
        auto const latest_cert_hash = latest_cert_block->get_block_hash();
        file << "[info] " << type_str << ": " << account << ", committd_height: " << committd_height << ", connected_height: " << connected_height
             << ", genesis_connected_height: " << m_blockstore->get_latest_genesis_connected_block_height(account)
             << ", executed_height: " << m_blockstore->get_latest_executed_block_height(account) << ", span_genesis_height " << span_genesis_height << ", latest_cert_height "
             << latest_cert_height << ", latest_cert_hash: " << base::xstring_utl::to_hex(latest_cert_hash) << std::endl;
        if (committd_height > connected_height + redundancy) {
            file << "[warn] " << type_str << ": " << account << ", committd_height and connected_height not equal, " << committd_height << ", " << connected_height << std::endl;
            error_num++;
        }
        if ((committd_height > span_genesis_height + redundancy) && (type == query_account_table)) {
            file << "[warn] " << type_str << ": " << account << ", committd_height and span_genesis_height not equal, " << committd_height << ", " << span_genesis_height
                 << std::endl;
            error_num++;
        }
        if (redundancy == 0) {
            auto latest_cert_bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(latest_cert_block.get());
            if (latest_cert_bstate == nullptr) {
                file << "[warn] " << type_str << ": " << account << ", latest cert state is nullptr!" << std::endl;
                error_num++;
                break;
            }
        }
        auto last_hash = latest_cert_hash;
        auto h = latest_cert_height;
        if (redundancy > 0) {
            h = (h > redundancy) ? (h - redundancy) : 0;
            auto const block = m_blockstore->load_block_object(account, h, 0, false);
            last_hash = block->get_block_hash();
        }
        do {
            auto const block = m_blockstore->load_block_object(account, h, last_hash, true);
            if (block == nullptr) {
                file << "[warn] " << type_str << ": " << account << ", height: " << h << ", hash: " << base::xstring_utl::to_hex(last_hash) << ", block is nullptr!" << std::endl;
                error_num++;
                break;
            }
            last_hash = block->get_last_block_hash();
            if (type == query_account_system) {
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
            auto txactions = data::xblockextract_t::unpack_txactions(block.get());
            for (auto & txaction : txactions) {
                auto txindex = base::xvchain_t::instance().get_xtxstore()->load_tx_idx(txaction.get_tx_hash(), txaction.get_tx_subtype());
                if (txindex == nullptr) {
                    file << "[warn] " << type_str << ": " << account << ", height: " << h << ", tx: " << base::xstring_utl::to_hex(txaction.get_tx_hash())
                            << ", load tx idx null!" << std::endl;
                    error_num++;
                }
            }
        } while (h-- > 0);
    } while (0);

    if (error_num) {
        std::cout << account << ", type: " << type << ", check not ok, error num: " << error_num << std::endl;
        {
            std::lock_guard<std::mutex> guard(m_write_lock);
            errors += error_num;
        }
    }
}

void xdb_export_tools_t::query_checkpoint(const uint64_t clock) {
    json j;
    auto const clock_str = base::xstring_utl::tostring(clock);
    auto const tables = get_table_accounts();
    auto genesis_only = get_special_genesis_accounts();
    std::vector<json> j_data(tables.size());
    asio::thread_pool pool(4);

    for (size_t i = 0; i < tables.size(); i++) {
        asio::post(pool, std::bind(&xdb_export_tools_t::query_checkpoint_internal, this, tables[i], genesis_only, clock, std::ref(j_data[i])));
    }
    pool.join();
    for (size_t i = 0; i < tables.size(); i++) {
        j[clock_str][tables[i]] = std::move(j_data[i]);
    }

    generate_json_file("checkpoint_data.json", j);
}

void xdb_export_tools_t::query_checkpoint_internal(std::string const & table, std::set<std::string> const & genesis_only, const uint64_t clock, json & j_data) {
    auto const genesis_block = base::xvchain_t::instance().get_xblockstore()->get_genesis_block(table);
    if (nullptr == genesis_block) {
        std::cerr << table << " genesis block nullptr!" << std::endl;
        return;
    }
    if (false == base::xvchain_t::instance().get_xblockstore()->load_block_output(table, genesis_block.get())) {
        std::cerr << table << " genesis block load_block_output failed!" << std::endl;
        return;
    }
    xobject_ptr_t<base::xvbstate_t> current_state = make_object_ptr<base::xvbstate_t>(*genesis_block.get());
    if (genesis_block->get_block_class() != base::enum_xvblock_class_nil) {
        std::string binlog = genesis_block->get_binlog();
        if(false == current_state->apply_changes_of_binlog(binlog)) {
            std::cerr << table << " genesis block apply_changes_of_binlog failed!" << std::endl;
            return;
        }
    }
    auto const height = m_blockstore->get_latest_committed_block_height(table);
    auto table_height = genesis_block->get_height();
    auto table_hash = genesis_block->get_block_hash();
    for (size_t h = 1; h <= height; h++) {
        auto const vblock = m_blockstore->load_block_object(table, h, 0, false);
        if (vblock == nullptr) {
            std::cerr << table << " height " << h << " block nullptr!" << std::endl;
            break;
        }
        if (vblock->get_clock() > clock) {
            break;
        }
        xassert(vblock->get_height() == current_state->get_block_height() + 1);
        table_height = vblock->get_height();
        table_hash = vblock->get_block_hash();
        current_state = make_object_ptr<base::xvbstate_t>(*vblock.get(), *current_state.get());
        if (vblock->get_block_class() == base::enum_xvblock_class_light) {
            if (false == base::xvchain_t::instance().get_xblockstore()->load_block_output(table, vblock.get())) {
                std::cerr << table << " height " << h << " load_block_output false!" << std::endl;
                return;
            }
            xassert(!vblock->get_binlog_hash().empty());
            std::string binlog = vblock->get_binlog();
            xassert(!binlog.empty());
            if(false == current_state->apply_changes_of_binlog(binlog)) {
                std::cerr << table << " height " << h << " apply_changes_of_binlog false!" << std::endl;
                return;
            }
        }
    }
    j_data["table_data"]["height"] = base::xstring_utl::tostring(table_height);
    j_data["table_data"]["hash"] = base::xstring_utl::to_hex(table_hash);
    // std::string bin_data;
    // current_state->serialize_to_string(bin_data);
    // j_state["table_state"] = base::xstring_utl::to_hex(bin_data);

    auto const & table_bstate = std::make_shared<data::xtable_bstate_t>(current_state.get());
    auto const & table_units = table_bstate->get_all_accounts();
    json j_unit_data;
    // json j_unit_state;
    for (auto const & unit : table_units) {
        base::xaccount_index_t index;
        if (table_bstate->get_account_index(unit, index) == false) {
            std::cerr << table << " " << unit << " get index failed!" << std::endl;
            continue;
        }
        auto const unit_height = index.get_latest_unit_height();
        auto const & unit_vblock = m_blockstore->load_block_object(unit, unit_height, 0, false);
        if (unit_vblock == nullptr) {
            std::cerr << unit << " height " << unit_height << " block nullptr!" << std::endl;
            continue;
        }
        // auto const & unit_bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(unit_vblock.get());
        // if (unit_bstate == nullptr) {
        //     std::cerr << unit << " height " << unit_height << " state nullptr!" << std::endl;
        //     continue;
        // }
        j_unit_data[unit]["height"] = base::xstring_utl::tostring(unit_height);
        j_unit_data[unit]["hash"] = base::xstring_utl::to_hex(unit_vblock->get_block_hash());
        // std::string bin_data;
        // unit_bstate->serialize_to_string(bin_data);
        // j_unit_state[unit] = base::xstring_utl::to_hex(bin_data);
    }
    base::xvaccount_t table_account{table};
    auto table_shortid = table_account.get_tableid().to_table_shortid();
    for (auto const & genesis : genesis_only) {
        base::xvaccount_t genesis_account{genesis};
        if (table_shortid != genesis_account.get_tableid().to_table_shortid() || table_units.count(genesis)) {
            continue;
        }
        auto const & unit_vblock = m_blockstore->get_genesis_block(genesis);
        if (unit_vblock == nullptr) {
            std::cerr << genesis << " genesis block nullptr!" << std::endl;
            continue;
        }
        // auto const & unit_bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(unit_vblock.get());
        // if (unit_bstate == nullptr) {
        //     std::cerr << genesis << " genesis state nullptr!" << std::endl;
        //     continue;
        // }
        j_unit_data[genesis]["height"] = base::xstring_utl::tostring(0);
        j_unit_data[genesis]["hash"] = base::xstring_utl::to_hex(unit_vblock->get_block_hash());
        // std::string bin_data;
        // unit_bstate->serialize_to_string(bin_data);
        // j_unit_state[genesis] = base::xstring_utl::to_hex(bin_data);
    }
    j_data["unit_data"] = j_unit_data;
    // j_state["unit_state"] = j_unit_state;
    std::cout << table << " cp checkout finish!" << std::endl;
}

void xdb_export_tools_t::query_account_data(std::string const & account,  const uint64_t h_end, std::string & result, const xdb_check_data_func_face_t & func) {
    uint64_t  num = 0;
    for(uint64_t index = 0; index <= h_end; index++) {
        bool const exist = func.is_data_exist(m_blockstore.get(), account, index);
        if (!exist) {
            if(num != 0) {
                result += std::to_string(index-num) + '-' + std::to_string(index-1) + ',';
                num = 0;
            }
        } else {
            num++;
        }
    }
    if (num != 0) {
        result +=  std::to_string(h_end-(num-1)) + '-' + std::to_string(h_end) ;
    }
}

void xdb_export_tools_t::query_account_data(std::string const & account, json & result_json, const xdb_check_data_func_face_t & func) {
    auto const block_height = m_blockstore->get_latest_committed_block_height(account);
    std::string result;
    query_account_data(account, block_height, result, func);
    result_json = result;
}

bool  xdb_export_tools_t::table_mpt_read_callback(const std::string& key, const std::string& value,void *cookie)
{
    uint64_t * node_num = (uint64_t *)cookie;
    (*node_num)++;
    return true;
}

void xdb_export_tools_t::query_table_mpt(std::string const & account, json & result_json) {
    uint64_t node_num = 0;
    auto key_prefix = base::xvdbkey_t::create_prunable_mpt_node_key(account, {});
    base::xvaccount_t vaccount(account);

    base::xvdbstore_t* xvdbstore =  base::xvchain_t::instance().get_xdbstore();
    top::store::xstore * pxstore = dynamic_cast< top::store::xstore*>(xvdbstore);
    pxstore->read_range_callback(key_prefix,  table_mpt_read_callback, (void*)&node_num);

    std::string result = std::to_string(node_num);
    result_json = result;
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
        j["last_full_block"]["hash"] = data::to_hex_str(block->get_fullstate_hash());
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
    query_account_data(account, height_full, s, xdb_check_data_func_block_t());
    j["exist_block"] = s;
}

json xdb_export_tools_t::set_txinfo_to_json(tx_ext_t const & txinfo) {
    json tx;
    tx["table height"] = txinfo.height;
    tx["self table"] = txinfo.self_table;
    tx["peer table"] = txinfo.peer_table;
    tx["timestamp"] = txinfo.timestamp;
    tx["source address"] = txinfo.src;
    tx["target address"] = txinfo.target;
    // tx["unit height"] = txinfo.unit_height;
    tx["phase"] = txinfo.phase;
    return tx;
}

json xdb_export_tools_t::set_confirmed_txinfo_to_json(const tx_ext_sum_t & tx_ext_sum) {
    json tx;
    tx["is_self"] = tx_ext_sum.is_self;
    tx["not_need_confirm"] = tx_ext_sum.not_need_confirm;
    tx["time cost"] = tx_ext_sum.get_delay_from_send_to_confirm();
    tx["time cost from fire"] = tx_ext_sum.get_delay_from_fire_to_confirm();
    tx["fire time"] =  tx_ext_sum.fire_timestamp;
    tx["send time"] =  tx_ext_sum.send_block_info.timestamp;
    tx["recv time"] =  tx_ext_sum.recv_block_info.timestamp;
    tx["confirm time"] = tx_ext_sum.confirm_block_info.timestamp;
    tx["send table height"] = tx_ext_sum.send_block_info.height;
    tx["recv table height"] = tx_ext_sum.recv_block_info.height;
    tx["confirm table height"] = tx_ext_sum.confirm_block_info.height;
    tx["self table"] = tx_ext_sum.self_table;
    tx["peer table"] = tx_ext_sum.peer_table;
    return tx;
}

json xdb_export_tools_t::set_unconfirmed_txinfo_to_json(const tx_ext_sum_t & tx_ext_sum) {
    json tx;
    tx["fire time"] =  tx_ext_sum.fire_timestamp;
    tx["send time"] =  tx_ext_sum.send_block_info.timestamp;
    tx["recv time"] =  tx_ext_sum.recv_block_info.timestamp;
    tx["confirm time"] = tx_ext_sum.confirm_block_info.timestamp;
    tx["send table height"] = tx_ext_sum.send_block_info.height;
    tx["recv table height"] = tx_ext_sum.recv_block_info.height;
    tx["confirm table height"] = tx_ext_sum.confirm_block_info.height;
    tx["self table"] = tx_ext_sum.self_table;
    tx["peer table"] = tx_ext_sum.peer_table;
    return tx;
}

void xdb_export_tools_t::xdbtool_all_table_info_t::set_table_txdelay_time(const tx_ext_sum_t & tx_ext_sum) {
    if (tx_ext_sum.fire_timestamp == 0) {
        // TODO(jimmy) invalid tx should drop it
        return;
    }

    uint32_t delay_from_send_to_confirm = tx_ext_sum.get_delay_from_send_to_confirm();
    uint32_t delay_from_fire_to_confirm = tx_ext_sum.get_delay_from_fire_to_confirm();

    // total_confirm_time_from_send += delay_from_send_to_confirm;
    if (delay_from_send_to_confirm > max_confirm_time_from_send) {
        max_confirm_time_from_send = delay_from_send_to_confirm;
    }
    // total_confirm_time_from_fire += delay_from_fire_to_confirm;
    if (delay_from_fire_to_confirm > max_confirm_time_from_fire) {
        max_confirm_time_from_fire = delay_from_fire_to_confirm;
    }
    // xassert(total_confirm_time_from_fire >= total_confirm_time_from_send);
}

bool xdb_export_tools_t::xdbtool_all_table_info_t::all_table_set_txinfo(const tx_ext_t & tx_ext, base::enum_transaction_subtype subtype, tx_ext_sum_t & tx_ext_sum) {
    if (subtype == base::enum_transaction_subtype_self) {
        tx_ext_sum = tx_ext_sum_t(tx_ext, subtype);
        set_table_txdelay_time(tx_ext_sum);
        return true;  // TODO(jimmy) drop invalid fire timestamp tx
    }

    std::lock_guard<std::mutex> lck(m_lock);

    auto iter = unconfirmed_tx_map.find(tx_ext.hash);
    if (iter == unconfirmed_tx_map.end()) {
        unconfirmed_tx_map[tx_ext.hash] = tx_ext_sum_t(tx_ext, subtype);
    } else {
        iter->second.copy_tx_ext(tx_ext, subtype);
        if (iter->second.is_confirmed()) {
            tx_ext_sum = iter->second;
            unconfirmed_tx_map.erase(iter);
            set_table_txdelay_time(tx_ext_sum);
            return true;
        }
    }

    return false;
}

void xdb_export_tools_t::print_all_table_txinfo_to_file() {
    std::string info_file = m_outfile_folder + "all_tx_info.json";
    std::string abnormal_file = m_outfile_folder + "all_tx_abnormal.json";
    std::ofstream info_stream(info_file);
    std::ofstream abnormal_stream(abnormal_file);

    json j; // info json
    j["send only detail"] = nullptr;
    j["no need confirm recv only detail"] = nullptr;
    j["confirmed only detail"] = nullptr;
    j["multi detail"] = nullptr;

    // int confirmedtx_num = 0;
    uint64_t max_confirm_time_from_send = 0;
    uint64_t max_confirm_time_from_fire = 0;
    uint32_t send_only_count = 0;
    uint32_t recv_only_count = 0;
    uint32_t confirm_only_count = 0;
    for (uint32_t i = 0; i < TOTAL_TABLE_NUM; i++) {
        // confirmedtx_num += m_all_table_info[i].confirmedtx_num;
        if (max_confirm_time_from_send < m_all_table_info[i].max_confirm_time_from_send) {
            max_confirm_time_from_send = m_all_table_info[i].max_confirm_time_from_send;
        }
        if (max_confirm_time_from_fire < m_all_table_info[i].max_confirm_time_from_fire) {
            max_confirm_time_from_fire = m_all_table_info[i].max_confirm_time_from_fire;
        }

        for (auto & v : m_all_table_info[i].unconfirmed_tx_map) {
            auto & tx_ext_sum = v.second;
            if (tx_ext_sum.send_block_info.height != 0) {
                j["send only detail"][tx_ext_sum.hash] = set_unconfirmed_txinfo_to_json(tx_ext_sum);
                send_only_count++;
            } else if (tx_ext_sum.recv_block_info.height != 0) {
                if (tx_ext_sum.not_need_confirm) {
                    j["recv only detail"][tx_ext_sum.hash] = set_unconfirmed_txinfo_to_json(tx_ext_sum);
                    recv_only_count++;
                }
            } else {
                j["confirm only detail"][tx_ext_sum.hash] = set_unconfirmed_txinfo_to_json(tx_ext_sum);
                confirm_only_count++;
            }
        }
    }
    abnormal_stream << std::setw(4) << j << std::endl;

    // j.clear();
    // j["confirmed count"] = confirmedtx_num;
    j["send only count"] = send_only_count;
    j["no need confirm recv only count"] = recv_only_count;
    j["confirmed only count"] = confirm_only_count;
    j["confirmed max time"] = max_confirm_time_from_send;
    j["confirmed_from_fire max time"] = max_confirm_time_from_fire;
    info_stream << std::setw(4) << j << std::endl;

    info_stream.close();
    abnormal_stream.close();
}

bool xdb_export_tools_t::all_table_set_txinfo(const tx_ext_t & tx_ext, base::enum_transaction_subtype subtype, tx_ext_sum_t & tx_ext_sum) {
    // uint32_t r = (uint32_t)base::xhash64_t::digest(tx_ext.hash);
    base::xtable_index_t tableindex(tx_ext.sendtableid);
    uint32_t r = tableindex.to_total_table_index();
    return m_all_table_info[r].all_table_set_txinfo(tx_ext, subtype, tx_ext_sum);
}

void xdb_export_tools_t::get_txinfo_from_txaction(const data::xlightunit_action_t & txaction, const data::xblock_t * block, const data::xtransaction_ptr_t & tx_ptr, std::vector<tx_ext_t> & batch_tx_exts) {
    base::xtable_shortid_t tableid;
    if (tx_ptr != nullptr) {
        base::xvaccount_t _vaddr(tx_ptr->get_source_addr());
        tableid = _vaddr.get_short_table_id();
    } else {
        tableid = txaction.get_rawtx_source_tableid();
    }

    {
        tx_ext_t tx_ext;
        tx_ext.sendtableid = tableid;
        if (tx_ptr != nullptr) {
            tx_ext.src = tx_ptr->get_source_addr();
            tx_ext.target = tx_ptr->get_target_addr();
            tx_ext.fire_timestamp = tx_ptr->get_fire_timestamp();
            if (tx_ext.fire_timestamp == 0) {
                auto iter = m_txs_fire_timestamp.find(tx_ptr->get_digest_str());
                if (iter != m_txs_fire_timestamp.end()) {
                    tx_ext.fire_timestamp = iter->second;
                    // std::cout << "find fire_timestamp " << tx_ptr->dump() << std::endl;
                } else {
                    // std::cout << "not find fire_timestamp " << tx_ptr->dump() << std::endl;
                }
            }
            tx_ext.self_table = txaction.get_receipt_id_self_tableid();
            tx_ext.peer_table = txaction.get_receipt_id_peer_tableid();
        }
        tx_ext.not_need_confirm = txaction.get_not_need_confirm();
        tx_ext.height = block->get_height();
        tx_ext.timestamp = block->get_second_level_gmtime();  // here should use second level gmtime for statistic
        tx_ext.hash = "0x" + txaction.get_tx_hex_hash();
        tx_ext.phase = txaction.get_tx_subtype();
        batch_tx_exts.push_back(tx_ext);
    }
    if (txaction.get_inner_table_flag()) {
        tx_ext_t tx_ext2;
        tx_ext2.sendtableid = tableid;
        tx_ext2.not_need_confirm = txaction.get_not_need_confirm();
        tx_ext2.height = block->get_height();
        tx_ext2.timestamp = block->get_second_level_gmtime();  // here should use second level gmtime for statistic
        tx_ext2.hash = "0x" + txaction.get_tx_hex_hash();
        tx_ext2.phase = base::enum_transaction_subtype_recv;
        batch_tx_exts.push_back(tx_ext2);

        if (!txaction.get_not_need_confirm()) {
            tx_ext_t tx_ext3;
            tx_ext3.sendtableid = tableid;
            tx_ext3.not_need_confirm = txaction.get_not_need_confirm();
            tx_ext3.height = block->get_height();
            tx_ext3.timestamp = block->get_second_level_gmtime();  // here should use second level gmtime for statistic
            tx_ext3.hash = "0x" + txaction.get_tx_hex_hash();
            tx_ext3.phase = base::enum_transaction_subtype_confirm;
            batch_tx_exts.push_back(tx_ext3);
        }
    }
}

void xdb_export_tools_t::query_tx_info_internal(std::string const & account, const uint32_t start_timestamp, const uint32_t end_timestamp) {
    xdbtool_table_info_t table_info;
    // std::map<std::string, tx_ext_t> sendonly;  // sendtx without confirmed
    // std::map<std::string, tx_ext_t> recvonly;  // sendtx that not need confirm without recv
    // std::map<std::string, tx_ext_t> confirmonly;  // confirmtx without send
    std::vector<tx_ext_t> multi_txs; // ERROR tx is multi packed in block
    std::map<std::string, uint16_t> tx_phase_count; // tx phase count for check multi txs statistic

    // 4 files per table
    std::string info_file = m_outfile_folder + account + "_tx_info.json";
    std::string normal_file = m_outfile_folder + account + "_tx_normal.json";
    std::string abnormal_file = m_outfile_folder + account + "_tx_abnormal.json";
    std::ofstream info_stream(info_file);
    std::ofstream normal_stream(normal_file);
    std::ofstream abnormal_stream(abnormal_file);

    auto const block_height = m_blockstore->get_latest_committed_block_height(account);
    for (uint64_t h = 0; h <= block_height; h++) {
        auto vblock = m_blockstore->load_block_object(account, h, 0, false);
        const data::xblock_t * block = dynamic_cast<data::xblock_t *>(vblock.get());
        if (block == nullptr) {
            table_info.missing_table_block_num++;
            std::cerr << account << " ERROR:missing block at height " << h << std::endl;
            continue;
        }
        if (block->get_timestamp() < start_timestamp || block->get_timestamp() > end_timestamp) {
            continue;
        }
        if (block->get_block_class() == base::enum_xvblock_class_nil) {
            table_info.empty_table_block_num++;
            continue;
        } else if (block->get_block_class() == base::enum_xvblock_class_full) {
            table_info.full_table_block_num++;
            continue;
        } else {
            table_info.light_table_block_num++;
            m_blockstore->load_block_input(account, vblock.get());
        }
        auto units_index = block->get_subblocks_index();
        table_info.total_unit_block_num += units_index.size();
        for (auto & unit_index : units_index) {
            if (unit_index.get_block_class() == base::enum_xvblock_class_nil) {
                table_info.empty_unit_block_num++;
            } else if (unit_index.get_block_class() == base::enum_xvblock_class_full) {
                table_info.full_unit_block_num++;
            } else {
                table_info.light_unit_block_num++;
            }
        }
        // step all actions
        auto input_actions = data::xblockextract_t::unpack_txactions(vblock.get());
        for (auto & txaction : input_actions) {
            if (txaction.is_self_tx()) {
                table_info.selftx_num++;
            } else {
                if (txaction.is_send_tx()) {
                    table_info.sendtx_num++;
                } else if (txaction.is_recv_tx()) {
                    table_info.recvtx_num++;
                } else if (txaction.is_confirm_tx()) {
                    table_info.confirmtx_num++;
                }
            }

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

            // statistic
            // construct tx_ext info
            std::vector<tx_ext_t> batch_tx_exts;
            get_txinfo_from_txaction(txaction, block, tx_ptr, batch_tx_exts);
            for (auto & tx_ext : batch_tx_exts) {
                json j;
                base::enum_transaction_subtype type = (base::enum_transaction_subtype)tx_ext.phase;
                tx_ext_sum_t tx_ext_sum;
                bool confirmed = all_table_set_txinfo(tx_ext, type, tx_ext_sum);
                if (confirmed) {
                    if (tx_ext_sum.fire_timestamp == 0) {
                        table_info.no_firestamptx_num++;
                    } else {
                        table_info.confirmedtx_num++;
                        j[tx_ext.hash] = set_confirmed_txinfo_to_json(tx_ext_sum);
                        normal_stream << std::setw(4) << j << std::endl;
                    }
                }
            }
        }
    }

    json j; // info json
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
    j["table info"]["total real confirm num"] = table_info.confirmtx_num;
    j["table info"]["total confirm num"] = table_info.confirmedtx_num; // should modify with QA script at the same time!!!
    j["table info"]["total no_firestamp num"] = table_info.no_firestamptx_num;
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

    // j["confirmed conut"] = table_info.confirmedtx_num;
    // j["send only count"] = sendonly.size();
    // j["no need confirm recv only count"] = recvonly.size();
    // j["confirmed only count"] = confirmonly.size();
    // // j["confirmed total time"] = table_info.total_confirm_time_from_send;
    // j["confirmed max time"] = table_info.max_confirm_time_from_send;
    // // j["confirmed avg time"] = float(table_info.total_confirm_time_from_send) / table_info.confirmedtx_num;
    // // j["confirmed_from_fire total time"] = table_info.total_confirm_time_from_fire;
    // j["confirmed_from_fire max time"] = table_info.max_confirm_time_from_fire;
    // // j["confirmed_from_fire avg time"] = float(table_info.total_confirm_time_from_fire) / table_info.confirmedtx_num;
    info_stream << std::setw(4) << j << std::endl;
    j.clear();

    // j["send only detail"] = nullptr;
    // j["confirmed only detail"] = nullptr;
    j["multi detail"] = nullptr;
    // for (auto & v : sendonly) {
    //     auto & txinfo = v.second;
    //     j["send only detail"][txinfo.hash] = set_txinfo_to_json(txinfo);
    // }
    // for (auto & v : recvonly) {
    //     auto & txinfo = v.second;
    //     j["no need confirm recv only detail"][txinfo.hash] = set_txinfo_to_json(txinfo);
    // }
    // for (auto & v : confirmonly) {
    //     auto & txinfo = v.second;
    //     j["confirmed only detail"][txinfo.hash] = set_txinfo_to_json(txinfo);
    // }
    for (auto & v : multi_txs) {
        j["multi detail"][v.hash] = set_txinfo_to_json(v);
    }
    abnormal_stream << std::setw(4) << j << std::endl;

    info_stream.close();
    normal_stream.close();
    abnormal_stream.close();
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
    root = dynamic_cast<xrpc::xrpc_query_manager *>(m_getblock.get())->get_block_json(bp);
    root["real-flags"] = base::xstring_utl::tostring((int32_t)bp->get_block_flags());
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
    if (false == bstate->find_property(data::XPROPERTY_TABLE_ACCOUNT_INDEX)) {
        std::cerr << "table: " << table << ", latest committed block state fail-find property XPROPERTY_TABLE_ACCOUNT_INDEX " << data::XPROPERTY_TABLE_ACCOUNT_INDEX << std::endl;
    } else {
        auto propobj = bstate->load_string_map_var(data::XPROPERTY_TABLE_ACCOUNT_INDEX);
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
        if (unit_bstate->find_property(data::XPROPERTY_BALANCE_AVAILABLE)) {
            unit_balance = unit_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance();
        }
        if (unit_bstate->find_property(data::XPROPERTY_BALANCE_LOCK)) {
            unit_lock_balance = unit_bstate->load_token_var(data::XPROPERTY_BALANCE_LOCK)->get_balance();
        }
        if (unit_bstate->find_property(data::XPROPERTY_BALANCE_PLEDGE_TGAS)) {
            unit_tgas_balance = unit_bstate->load_token_var(data::XPROPERTY_BALANCE_PLEDGE_TGAS)->get_balance();
        }
        if (unit_bstate->find_property(data::XPROPERTY_BALANCE_PLEDGE_VOTE)) {
            unit_vote_balance = unit_bstate->load_token_var(data::XPROPERTY_BALANCE_PLEDGE_VOTE)->get_balance();
        }
        if (unit_bstate->find_property(data::XPROPERTY_BALANCE_BURN)) {
            unit_burn_balance = unit_bstate->load_token_var(data::XPROPERTY_BALANCE_BURN)->get_balance();
        }
        j_unit[account][data::XPROPERTY_BALANCE_AVAILABLE] = unit_balance;
        j_unit[account][data::XPROPERTY_BALANCE_LOCK] = unit_lock_balance;
        j_unit[account][data::XPROPERTY_BALANCE_PLEDGE_TGAS] = unit_tgas_balance;
        j_unit[account][data::XPROPERTY_BALANCE_PLEDGE_VOTE] = unit_vote_balance;
        j_unit[account][data::XPROPERTY_BALANCE_BURN] = unit_burn_balance;
        balance += unit_balance;
        lock_balance += unit_lock_balance;
        tgas_balance += unit_tgas_balance;
        vote_balance += unit_vote_balance;
        burn_balance += unit_burn_balance;
    }
    j_table[data::XPROPERTY_BALANCE_AVAILABLE] = balance;
    j_table[data::XPROPERTY_BALANCE_BURN] = burn_balance;
    j_table[data::XPROPERTY_BALANCE_LOCK] = lock_balance;
    j_table[data::XPROPERTY_BALANCE_PLEDGE_TGAS] = tgas_balance;
    j_table[data::XPROPERTY_BALANCE_PLEDGE_VOTE] = vote_balance;
}

void xdb_export_tools_t::load_db_unit_accounts_info() {
    auto const & tables = get_table_accounts();
    for (auto const table : tables) {
        auto const latest_block = m_blockstore->get_latest_committed_block(table);
        if (latest_block == nullptr) {
            std::cerr << table << " get_latest_committed_block null!" << std::endl;
            continue;
        }
        auto const bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(latest_block.get());
        if (bstate == nullptr) {
            std::cerr << table << " get_block_state null!" << std::endl;
            continue;
        }
        auto const table_bstate = std::make_shared<data::xtable_bstate_t>(bstate.get());
        auto const & table_units = table_bstate->get_all_accounts();
        std::map<std::string, base::xaccount_index_t> table_indexs;
        for (auto const & unit : table_units) {
            base::xaccount_index_t index;
            if (table_bstate->get_account_index(unit, index) == false) {
                std::cerr << "account: " << unit << " get index failed " << std::endl;
                continue;
            }
            table_indexs.insert(std::make_pair(unit, index));
        }
        m_db_units_info.insert(std::make_pair(table, table_indexs));
    }
}

void xdb_export_tools_t::set_outfile_folder(std::string const & folder) {
    mkdir(folder.c_str(), 0750);
    m_outfile_folder = folder;
}



void xdb_export_tools_t::parse_info_set(xdbtool_parse_info_t &info, int db_key_type, uint64_t value_size)
{
    switch (db_key_type) {
        case base::enum_xdbkey_type_block_object:
            info.block_count++;
            info.block_size += value_size;
        case base::enum_xdbkey_type_block_index:
            info.index_count++;
            info.index_size += value_size;
        break;
        case base::enum_xdbkey_type_block_input_resource:
            info.input_count++;
            info.input_size += value_size;
        break;
        case base::enum_xdbkey_type_block_output_resource:
            info.output_count++;
            info.output_size += value_size;
        break;
        case base::enum_xdbkey_type_unit_proof:
            info.proof_count++;
            info.proof_size += value_size;
        break;
        case base::enum_xdbkey_type_state_object:
            info.state_count++;
            info.state_size += value_size;
        default:
            break;
    }
}

std::string xdb_export_tools_t::get_account_key_string(const std::string& key)
{
    std::string type_str = key.substr(0, 2);
    if (type_str == "T2") {
        std::string::size_type idx = key.find(sys_contract_sharding_vote_addr);
        if (idx == std::string::npos) {
            idx = key.find(sys_contract_sharding_reward_claiming_addr);
            if (idx == std::string::npos) {
                idx = key.find(sys_contract_sharding_statistic_info_addr);
                if (idx == std::string::npos) {
                    return key;
                } else {
                    return sys_contract_sharding_statistic_info_addr;
                }
            } else {
                return sys_contract_sharding_reward_claiming_addr;
            }
        } else {
            return sys_contract_sharding_vote_addr;
        }
        assert(0);
    } else if (type_str == "Ta") {
        std::string::size_type idx = key.find(sys_contract_zec_table_block_addr);
        if (idx == std::string::npos) {
            idx = key.find(sys_contract_sharding_table_block_addr);
            if (idx == std::string::npos) {
                return sys_contract_beacon_table_block_addr;
            } else {
                return sys_contract_sharding_table_block_addr;
            }
        } else {
            return sys_contract_zec_table_block_addr;
        }
        assert(0);
    } else if (type_str == "T0") {
        return "T0";
    } else if (type_str == "T8") {
        return "T8";
    } else {
       return key;
    }
}

bool xdb_export_tools_t::db_scan_key_callback(const std::string& key, const std::string& value)
{
    base::enum_xdbkey_type db_key_type = base::xvdbkey_t::get_dbkey_type(key);

    m_dbsize_info.add_key_type(key, db_key_type, key.size(), value.size());

    std::string key_name = base::xvdbkey_t::get_dbkey_type_name(db_key_type);

    switch (db_key_type) {
        case base::enum_xdbkey_type_unknow:
        case base::enum_xdbkey_type_keyvalue:
        case base::enum_xdbkey_type_transaction: {
                auto iter = m_db_parse_info.find(key_name);
                if (iter != m_db_parse_info.end()) {
                    auto& info = iter->second;
                    info.count++;
                    info.size += value.length();
                } else {
                    xdbtool_parse_info_t parse_info { 0 };
                    parse_info.count = 1;
                    parse_info.size = value.length();
                    m_db_parse_info.insert({ key_name, parse_info });
                }
            }
        break;

        case base::enum_xdbkey_type_block_object:
        case base::enum_xdbkey_type_block_input_resource:
        case base::enum_xdbkey_type_block_output_resource:
        case base::enum_xdbkey_type_unit_proof:
        case base::enum_xdbkey_type_block_index:
        case base::enum_xdbkey_type_state_object:
        case base::enum_xdbkey_type_block_out_offdata: {
                const std::string key_name = base::xvdbkey_t::get_account_prefix_key(key);
                //std::cout << "db_key_type" << db_key_type << "key is "<< key << " key_name is " << key_name.c_str() <<std::endl;
                auto iter = m_db_parse_info.find(key_name);
                if (iter != m_db_parse_info.end()) {
                    auto& info = iter->second;
                    parse_info_set(info, db_key_type, value.length());
                } else {
                    xdbtool_parse_info_t parse_info { 0 };
                    m_info_account_count++;
                    parse_info.account_number = m_info_account_count;
                    parse_info_set(parse_info, db_key_type, value.length());
                    m_db_parse_info.insert({ key_name, parse_info });
                }

                std::vector<std::string> sum_values;
                base::xstring_utl::split_string(key_name, '/', sum_values);
                std::string key_str = sum_values[2];
                std::string key_real_str = get_account_key_string(key_str);

                auto info_iter = m_db_sum_info.find(key_real_str);
                if (info_iter != m_db_sum_info.end()) {
                    auto& info = info_iter->second;
                    parse_info_set(info, db_key_type, value.length());
                } else {
                    xdbtool_parse_info_t parse_info { 0 };
                    parse_info_set(parse_info, db_key_type, value.length());
                    m_db_sum_info.insert({ key_real_str, parse_info });
                }
            }
        break;
        default:
        break;
    }
    if ((m_info_key_count++) % 1000000 == 0) {
        std::cout << "db scan key total = " << m_info_key_count << std::endl;
    }
    return true;
}

bool  xdb_export_tools_t::db_scan_key_callback(const std::string& key, const std::string& value,void *cookie)
{
    bool ret = false;
    if (NULL != cookie) {
        xdb_export_tools_t *p_xdb_export_tools_t = (xdb_export_tools_t*)cookie;
        ret = p_xdb_export_tools_t->db_scan_key_callback(key, value);
    }
    return ret;
}

void xdb_export_tools_t::vector_to_json(std::map<std::string, xdbtool_parse_info_t> &db_info, json &json_root)
{
    for (auto const & iter : db_info) {
        json leaf;
        if (iter.first == "unknow" || iter.first == "keyvalue" || iter.first == "transaction") {
           leaf["count"] = iter.second.count;
           leaf["size"] = iter.second.size;
        } else {
            if (iter.second.account_number > 0) {
                leaf["number"] = iter.second.account_number;
            }
            if (iter.second.count > 0) {
                leaf["count"] = iter.second.count;
                leaf["size"] = iter.second.size;
            }
            if (iter.second.input_count > 0) {
                leaf["input_count"] = iter.second.input_count;
                leaf["input_size"] = iter.second.input_size;
            }
            if (iter.second.output_count > 0) {
                leaf["output_count"] = iter.second.output_count;
                leaf["output_size"] = iter.second.output_size;
            }
            if (iter.second.proof_count > 0) {
                leaf["proof_count"] = iter.second.proof_count;
                leaf["proof_size"] = iter.second.proof_size;
            }
            if (iter.second.block_count > 0) {
                leaf["block_count"] = iter.second.block_count;
                leaf["block_size"] = iter.second.block_size;
            }
            if (iter.second.state_count > 0) {
                leaf["state_count"] = iter.second.state_count;
                leaf["state_size"] = iter.second.state_size;
            }
            if (iter.second.index_count > 0) {
                leaf["index_count"] = iter.second.state_count;
                leaf["index_size"] = iter.second.state_size;
            }
        }
        json_root[iter.first] = leaf;
    }
}

void xdb_export_tools_t::db_parse_type_size(const std::string &fileName) {
    m_db_parse_info.clear();
    m_db_sum_info.clear();
    m_info_key_count = 0;
    m_info_account_count = 0;
    base::xvdbstore_t* xvdbstore =  base::xvchain_t::instance().get_xdbstore();
    top::store::xstore * pxstore = dynamic_cast< top::store::xstore*>(xvdbstore);

    pxstore->read_range_callback("",  db_scan_key_callback, this);

    std::cout << "start write json " << std::endl;
    //write json
    json root_sum;
    json root_detail;

    //count db size
    m_dbsize_info.to_json(root_sum);
    vector_to_json(m_db_sum_info, root_sum);
    std::string fileNameSum = fileName + "_sum.json";
    generate_json_file(fileNameSum, root_sum);
    std::string fileNameDetail = fileName + "_detail.json";
    vector_to_json(m_db_parse_info, root_detail);
    generate_json_file(fileNameDetail, root_detail);

}

std::set<std::string> xdb_export_tools_t::get_db_unit_accounts_v2() {
    std::set<std::string> accounts_set;
    if (m_db_units_info.empty()) {
        load_db_unit_accounts_info();
    }
    for (auto const & table : m_db_units_info) {
        for (auto const & unit_info : table.second) {
            accounts_set.insert(unit_info.first);
        }
    }
    return accounts_set;
}

std::set<std::string> xdb_export_tools_t::get_special_genesis_accounts() {
    std::set<std::string> accounts_set;
    auto const & contracts = get_system_contract_accounts();
    for (auto const & contract : contracts) {
        accounts_set.insert(contract);
    }
    std::vector<chain_data::data_processor_t> reset_data;
    chain_data::xchain_data_processor_t::get_all_user_data(reset_data);
    for (auto const & user_data : reset_data) {
        accounts_set.insert(user_data.address);
    }
    auto const genesis_loader = std::make_shared<loader::xconfig_genesis_loader_t>("{}");
    data::xrootblock_para_t  rootblock_para;
    genesis_loader->extract_genesis_para(rootblock_para);
    for (auto const & account : rootblock_para.m_account_balances) {
        accounts_set.insert(account.first);
    }
    for (auto const & node : rootblock_para.m_genesis_nodes) {
        accounts_set.insert(node.m_account.to_string());
    }
    return accounts_set;
}

void xdb_export_tools_t::generate_account_info_file(std::string const & account, const uint64_t height) {
    json j;
    for (size_t i = 0; i <= height; i++) {
        query_block_basic(account, i, j["block" + std::to_string(i)]);
    }
    query_state_basic(account, height, j["state"]);
    query_meta(account, j["meta"]);
    generate_json_file(account + "_basic_info.json", j);
}

void xdb_export_tools_t::generate_json_file(std::string const & filename, json const & j) {
    std::string name{m_outfile_folder + filename};
    std::ofstream out_json(name);
    out_json << std::setw(4) << j;
    out_json.close();
    std::cout << "===> " << name << " generated success!" << std::endl;
}

void xdb_export_tools_t::generate_common_file(std::string const & filename, std::string const & data) {
    std::string name{m_outfile_folder + filename};
    std::ofstream out_file(name);
    if (!data.empty()) {
        out_file << data;
    }
    out_file.close();
    std::cout << "===> " << name << " generated success!" << std::endl;
}

void xdb_export_tools_t::compact_db() {
    std::string begin_key;
    std::string end_key;
    base::xvchain_t::instance().get_xdbstore()->compact_range(begin_key, end_key);
}

void  xdb_export_tools_t::prune_db(){
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
        auto checkpoint = data::xtop_chain_checkpoint::get_latest_checkpoint(_vaddress, err);
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
}

bool xdb_export_tools_t::all_table_check_tx_file(const tx_ext_t & tx_ext,
                                                 base::enum_transaction_subtype type,
                                                 const std::map<std::string, tx_check_info_t> & tx_check_list,
                                                 std::map<std::string, tx_check_result_info_t> & tx_result_list) {
    std::string hash = tx_ext.hash;
    if (hash.substr(0,2) == "0x")
        hash = hash.substr(2);
    hash = HexDecode(hash);
    const auto it = tx_check_list.find(hash);
    if (it == tx_check_list.end()) {
//        std::cout << "not found: " << tx_ext.hash << std::endl;
        return false;
    }
    //std::cout << "found: " << tx_ext.hash << std::endl;
    if (tx_result_list.find(hash) == tx_result_list.end()) {
        tx_check_result_info_t tx_result;
        if (type == base::enum_transaction_subtype_send)
            tx_result.m_tx_send_timestamp = tx_ext.timestamp;
        else if (type == base::enum_transaction_subtype_recv)
            tx_result.m_tx_recv_timestamp = tx_ext.timestamp;
        else if (type == base::enum_transaction_subtype_confirm || type == base::enum_transaction_subtype_self)
            tx_result.m_tx_timestamp = tx_ext.timestamp;
        else
            return false;
        tx_result.m_test_timestamp = it->second.m_test_timestamp;
        tx_result_list[hash] = tx_result;
        //std::cout << "set type: " << type << std::endl;
        return true;
    }

    if (type == base::enum_transaction_subtype_send)
        tx_result_list[hash].m_tx_send_timestamp = tx_ext.timestamp;
    else if (type == base::enum_transaction_subtype_recv)
        tx_result_list[hash].m_tx_recv_timestamp = tx_ext.timestamp;
    else if (type == base::enum_transaction_subtype_confirm || type == base::enum_transaction_subtype_self)
        tx_result_list[hash].m_tx_timestamp = tx_ext.timestamp;
    //std::cout << "set type2: " << type << std::endl;
    return true;
}

bool xdb_check_data_func_block_t::is_data_exist(base::xvblockstore_t * blockstore, std::string const & account, uint64_t height) const {
    auto vblock = blockstore->load_block_object(account, height, base::enum_xvblock_flag_committed, false);
    data::xblock_t * block = dynamic_cast<data::xblock_t *>(vblock.get());
    return (block != nullptr);
}
std::string xdb_check_data_func_block_t::data_type() const {
    return "sync_result";
}

bool xdb_check_data_func_table_state_t::is_data_exist(base::xvblockstore_t * blockstore, std::string const & account, uint64_t height) const {
    auto vblock = blockstore->load_block_object(account, height, base::enum_xvblock_flag_committed, false);
    data::xblock_t * block = dynamic_cast<data::xblock_t *>(vblock.get());
    if (block == nullptr) {
        return false;
    }

    auto state_db_key = base::xvdbkey_t::create_prunable_state_key(account, height, block->get_block_hash());
    const std::string state_db_bin = base::xvchain_t::instance().get_xdbstore()->get_value(state_db_key);
    return !state_db_bin.empty();
}
std::string xdb_check_data_func_table_state_t::data_type() const {
    return "state_data";
}

bool xdb_check_data_func_unit_state_t::is_data_exist(base::xvblockstore_t * blockstore, std::string const & account, uint64_t height) const {
    auto vblock = blockstore->load_block_object(account, height, base::enum_xvblock_flag_committed, false);
    data::xblock_t * block = dynamic_cast<data::xblock_t *>(vblock.get());
    if (block == nullptr) {
        return false;
    }

    auto state_db_key = base::xvdbkey_t::create_prunable_unit_state_key(account, height, block->get_block_hash());
    const std::string state_db_bin = base::xvchain_t::instance().get_xdbstore()->get_value(state_db_key);
    return !state_db_bin.empty();
}
std::string xdb_check_data_func_unit_state_t::data_type() const {
    return "state_data";
}

bool xdb_check_data_func_off_data_t::is_data_exist(base::xvblockstore_t * blockstore, std::string const & account, uint64_t height) const {
    auto vblock = blockstore->load_block_object(account, height, base::enum_xvblock_flag_committed, false);
    data::xblock_t * block = dynamic_cast<data::xblock_t *>(vblock.get());
    if (block == nullptr) {
        return false;
    }
    const std::string key = base::xvdbkey_t::create_prunable_block_output_offdata_key(vblock->get_account(), height, vblock->get_viewid());
    const std::string state_db_bin = base::xvchain_t::instance().get_xdbstore()->get_value(key);
    return !state_db_bin.empty();
}
std::string xdb_check_data_func_off_data_t::data_type() const {
    return "off_data";
}

std::unordered_map<common::xaccount_address_t, uint64_t> xdb_export_tools_t::get_unit_accounts(common::xaccount_address_t const & table_address,
                                                                                               std::uint64_t const table_height,
                                                                                               std::vector<common::xaccount_address_t> const & designated,
                                                                                               std::error_code & ec) const {
    assert(!ec);

    std::unordered_map<common::xaccount_address_t, uint64_t> result;

    if (table_address.base_address().type() != top::base::enum_vaccount_addr_type_block_contract) {
        ec = top::db_export::xerrc_t::invalid_table_address;
        return result;
    }

    std::vector<common::xaccount_address_t> qualified;
    std::copy_if(std::begin(designated), std::end(designated), std::back_inserter(qualified), [&table_address](common::xaccount_address_t const & address) {
        return address.table_address() == table_address;
    });

    assert(m_blockstore != nullptr);
    auto const table_block = m_blockstore->load_block_object(table_address.vaccount(), table_height, base::enum_xvblock_flag_committed, true);
    if (table_block == nullptr) {
        ec = xerrc_t::table_block_not_found;
        std::cerr << "table block at address " << table_address.to_string() << ":" << table_height << " not found" << std::endl;
        return result;
    }

    auto const & root_hash = data::xblockextract_t::get_state_root(table_block.get(), ec);
    if (ec || root_hash.empty()) {
        base::xauto_ptr<base::xvbstate_t> const bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(table_block.get());
        if (bstate == nullptr) {
            ec = xerrc_t::table_state_not_found;
            std::cerr << table_address.to_string() << " get_block_state null!" << std::endl;

            return result;
        }

        auto const table_state = std::make_shared<data::xtable_bstate_t>(bstate.get());
        auto const & account_data = table_state->all_accounts();
        if (qualified.empty()) {
            std::transform(std::begin(account_data),
                           std::end(account_data),
                           std::inserter(result, std::end(result)),
                           [](std::pair<common::xaccount_address_t, base::xaccount_index_t> const & account_datum) {
                    return std::make_pair(top::get<common::xaccount_address_t>(account_datum), top::get<base::xaccount_index_t>(account_datum).get_latest_unit_height());
                });
        } else {
            std::for_each(
                std::begin(account_data), std::end(account_data), [&qualified, &result](std::pair<common::xaccount_address_t, base::xaccount_index_t> const & account_datum) {
                    if (std::find_if(std::begin(qualified), std::end(qualified), [&account_datum](common::xaccount_address_t const & allowed) {
                            return allowed == top::get<common::xaccount_address_t>(account_datum);
                        }) != std::end(qualified)) {
                        result.emplace(top::get<common::xaccount_address_t>(account_datum), top::get<base::xaccount_index_t>(account_datum).get_latest_unit_height());
                    }
                });
        }
    } else {
        auto const kv_db = std::make_shared<evm_common::trie::xkv_db_t>(base::xvchain_t::instance().get_xdbstore(), table_address);
        auto xtrie_db = evm_common::trie::xtrie_db_t::NewDatabase(kv_db);
        auto const & leafs = top::evm_common::trie::xtrie_simple_iterator_t::trie_leafs(root_hash, make_observer(xtrie_db));
        if (qualified.empty()) {
            std::for_each(std::begin(leafs), std::end(leafs), [&result](xbytes_t const & bytes) {
                state_mpt::xaccount_info_t info;
                info.decode({std::begin(bytes), std::end(bytes)});

                result.emplace(info.m_account, info.m_index.get_latest_unit_height());
            });
        } else {
            std::for_each(std::begin(leafs), std::end(leafs), [&qualified, &result](xbytes_t const & bytes) {
                state_mpt::xaccount_info_t info;
                info.decode({std::begin(bytes), std::end(bytes)});

                if (std::find_if(std::begin(qualified), std::end(qualified), [&info](common::xaccount_address_t const & acc) { return acc == info.m_account; }) !=
                    std::end(qualified)) {
                    result.emplace(info.m_account, info.m_index.get_latest_unit_height());
                }
            });
        }
    }

    return result;
}

std::vector<xdb_export_tools_t::exported_account_data> xdb_export_tools_t::get_account_data(std::unordered_map<common::xaccount_address_t, uint64_t> const & accounts,
                                                                                            std::vector<common::xtoken_id_t> const & queried_tokens,
                                                                                            std::vector<std::string> const & queried_properties,
                                                                                            std::error_code & ec) const {
    assert(!ec);
    assert(m_blockstore != nullptr);

    std::vector<exported_account_data> result;
    for (auto const & account_datum : accounts) {
        auto const & account_address = top::get<common::xaccount_address_t const>(account_datum);
        auto const account_height = top::get<uint64_t>(account_datum);

        auto unit_vblock = m_blockstore->load_block_object(account_address.vaccount(), account_height, base::enum_xvblock_flag_committed, true);
        if (unit_vblock == nullptr) {
            ec = xerrc_t::account_data_not_found;
            xwarn("account %s at height %" PRIu64 " with committed state not found. xdb_export_tools_t::get_account_data returns {}",
                  account_address.to_string().c_str(),
                  account_height);
            return {};
        }

        auto unit_vbstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(unit_vblock.get());
        if (unit_vbstate == nullptr) {
            ec = xerrc_t::account_data_not_found;
            xwarn("account %s at height %" PRIu64 " state not found. xdb_export_tools_t::get_account_data returns {}", account_address.to_string().c_str(), account_height);
            return {};
        }

        auto const unit_bstate = top::make_unique<data::xunit_bstate_t>(unit_vbstate.get());

        exported_account_data data;
        data.account_address = account_address;

        for (auto const token_id : queried_tokens) {
            switch (token_id) {  // NOLINT(clang-diagnostic-switch-enum)
            case common::xtoken_id_t::top: {
                data.assets[0][data::XPROPERTY_BALANCE_AVAILABLE] = unit_bstate->balance();
                data.assets[0][data::XPROPERTY_BALANCE_LOCK] = unit_bstate->lock_balance();
                data.assets[0][data::XPROPERTY_BALANCE_PLEDGE_TGAS] = unit_bstate->tgas_balance();
                data.assets[0][data::XPROPERTY_BALANCE_PLEDGE_VOTE] = unit_bstate->vote_balance();
                data.assets[0][data::XPROPERTY_BALANCE_BURN] = unit_bstate->burn_balance();

                break;
            }

            case common::xtoken_id_t::eth:  // NOLINT(bugprone-branch-clone)
                XATTRIBUTE_FALLTHROUGH;
            case common::xtoken_id_t::usdt:
                XATTRIBUTE_FALLTHROUGH;
            case common::xtoken_id_t::usdc: {
                std::string token_id_string{static_cast<char>('0' + static_cast<int>(token_id))};
                data.assets[1][token_id_string] = unit_bstate->tep_token_balance(token_id);
                break;
            }

            default: {
                assert(false);
                ec = xerrc_t::unknown_token;
                break;
            }
            }
        }

        for (auto const & property_name : queried_properties) {
            auto bytes = unit_bstate->get_bstate()->get_property_value_in_bytes(property_name);
            data.properties.emplace(property_name, std::move(bytes));
        }

        result.emplace_back(std::move(data));
    }

    return result;
}

void xdb_export_tools_t::append_to_json(common::xtable_address_t const & table_address,
                                        uint64_t const table_height,
                                        std::vector<exported_account_data> const & data,
                                        std::string const & file_path,
                                        std::error_code & /*ec*/) const {
    nlohmann::json root;
    std::ofstream out{file_path, std::ios::out | std::ios::app};

    auto & table_json = root[table_address.to_string()];
    table_json["account_count"] = data.size();

    for (auto const & account_datum : data) {
        auto const & account_address = account_datum.account_address;
        auto const & top_asset = account_datum.assets[0];
        auto const & tep1_asset = account_datum.assets[1];
        auto const & properties = account_datum.properties;

        assert(account_address.table_address() == table_address);

        nlohmann::json account_json;

        account_json["table_height"] = table_height;

        for (auto const & top_balance_datum : top_asset) {
            account_json["TOP"][top::get<std::string const>(top_balance_datum)] = top::get<evm_common::u256>(top_balance_datum).str();
        }

        for (auto const & tep1_balance_datum : tep1_asset) {
            account_json["TEP1"][top::get<std::string const>(tep1_balance_datum)] = top::get<evm_common::u256>(tep1_balance_datum).str();
        }

        for (auto const & property_datum : properties) {
            auto const & property_name = top::get<std::string const>(property_datum);
            auto const & property_value = top::get<xbytes_t>(property_datum);

            account_json["properties"][property_name] = to_hex(std::begin(property_value), std::end(property_value), "0x");
        }

        table_json[account_address.to_string()] = account_json;
    }

    out << std::setw(4) << root;
    out.close();
}

NS_END2
