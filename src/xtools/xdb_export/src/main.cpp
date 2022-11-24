#include "../xdb_export.h"
#include "../xdb_reset.h"
#include "../xdb_read.h"
#include "../xdb_write.h"
#include "../xdbtool_util.h"
#include "xbase/xhash.h"
#include "xmigrate/xvmigrate.h"
#include "xconfig/xpredefined_configurations.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xrootblock.h"
#include "xcrypto/xckey.h"
#include "xloader/xconfig_genesis_loader.h"

#include <omp.h>

using namespace top;
using namespace top::db_export;

#define XDB_EXPORT_LOG

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
    std::cout << "- ./xdb_export <config_json/db_path> <function_name>" << std::endl;
    std::cout << "    - <function_name>:" << std::endl;
    std::cout << "        - db_migrate_v2_to_v3 new_path" << std::endl;
    std::cout << "        - check_fast_sync <account>" << std::endl;
    std::cout << "        - check_state_data <account>" << std::endl;
    std::cout << "        - check_off_data" << std::endl;
    std::cout << "        - check_mpt" << std::endl;
    std::cout << "        - check_block_exist <account> <height>" << std::endl;
    std::cout << "        - check_block_info <account> <height|last|all>" << std::endl;
    std::cout << "        - check_tx_info [starttime] [endtime] [threads]" << std::endl;
    std::cout << "        - check_tx_file [tx_file] [threads]" << std::endl;
    std::cout << "        - check_latest_fullblock" << std::endl;
    std::cout << "        - check_contract_property <account> <property> <height|last|all>" << std::endl;
    std::cout << "        - check_balance" << std::endl;
    std::cout << "        - check_archive_db [table_address0:height0,table_address1:height1,...]" << std::endl;
    std::cout << "        - check_performance" << std::endl;
    std::cout << "        - parse_checkpoint <height>" << std::endl;
    std::cout << "        - parse_db" << std::endl;
    std::cout << "        - db_read_meta  [db_path] <account>" << std::endl;  // account can be "all_tables"
    std::cout << "        - db_data_parse [db_path] " << std::endl; // ./xdb_export ./db_v3/ db_data_parse
    std::cout << "        - db_compact_db [db_path]" << std::endl;
    std::cout << "        - db_parse_type_size [db_path] " << std::endl;
    std::cout << "        - db_read_block [db_path] <account> <height> " << std::endl;
    std::cout << "        - [db_path] db_read_all_table_height_lists <mode> <redundancy> " << std::endl; // ./xdb_export ./db_v3/ db_read_all_table_height_lists commit_mode 10
    std::cout << "        - db_read_txindex [db_path] <hex_txhash> " << std::endl;  // ./xdb_export ./db_v3/ db_read_txindex txhash send/recv/confirm
    std::cout << "        - correct_all_txindex [db_path] " << std::endl;// ./xdb_export ./db_v3/ correct_all_txindex
    std::cout << "        - correct_one_txindex [db_path] <hex_txhash> " << std::endl; // ./xdb_export ./db_v3/ correct_one_txindex   txhash
    std::cout << "        - db_prune [db_path] " << std::endl;
    std::cout << "        - export <exported.json> <table_address0:height0[,table_address1:height1,...]> [account0[,account1,...]]" << std::endl;
    std::cout << "-------  end  -------" << std::endl;
}

int main(int argc, char ** argv) {
    auto hash_plugin = new xtop_hash_t();
    top::config::config_register.get_instance().set(config::xmin_free_gas_asset_onchain_goverance_parameter_t::name, std::to_string(ASSET_TOP(100)));
    top::config::config_register.get_instance().set(config::xfree_gas_onchain_goverance_parameter_t::name, std::to_string(25000));
    top::config::config_register.get_instance().set(config::xmax_validator_stake_onchain_goverance_parameter_t::name, std::to_string(5000));
    top::config::config_register.get_instance().set(config::xchain_name_configuration_t::name, std::string{top::config::chain_name_testnet});
    top::config::config_register.get_instance().set(config::xroot_hash_configuration_t::name, std::string{});
    data::xrootblock_para_t para;
    data::xrootblock_t::init(para);
    top::utl::xkeyaddress_t pubkey1("");  // just init xsecp256k1_t

    if (argc < 3) {
        usage();
        return -1;
    }

    std::string db_path;
    {
        struct stat s;
        if (stat(argv[1], &s) != 0) {
            std::cout << "db: " << argv[1] << " not found" << std::endl;
            return -1;
        }
        if (s.st_mode & S_IFDIR) {
            db_path = argv[1];
        } else if (s.st_mode & S_IFREG) {
            std::string config_file{argv[1]};
            std::ifstream config_stream(config_file);
            json confg_json;
            config_stream >> confg_json;
            if (!confg_json.count("db_path")) {
                std::cout << "db: " << db_path << "not found!" << std::endl;
                return -1;
            }
            db_path = confg_json.at("db_path");
            if (access(db_path.c_str(), 0) != 0) {
                std::cout << "db: " << db_path << "not found!" << std::endl;
                return -1;
            }
        } else {
            std::cout << "db: " << argv[1] << " not found" << std::endl;
            return -1;
        }
    }
#ifdef XDB_EXPORT_LOG
    mkdir("log", 0750);
    xinit_log("./log/xdb_export.log", true, true);
    xset_log_level(enum_xlog_level_debug);
    xdbg("------------------------------------------------------------------");
    xinfo("new log start here");
#endif

    std::string function_name{argv[2]};

    if (xdb_read_tools_t::is_match_function_name(function_name)) {
        xdb_read_tools_t read_tools{db_path};
        read_tools.process_function(function_name, argc, argv);
        return 0;
    }

    if (xdb_write_tools_t::is_match_function_name(function_name)) {
        xdb_write_tools_t write_tools{db_path};
        write_tools.process_function(function_name, argc, argv);
        return 0;
    }

    if (function_name == "db_compact_db") {
        if (argc != 4) {
            usage();
            return -1;
        }
        std::string v3_db_path = argv[3];
        xdb_export_tools_t tools_v3{v3_db_path};
        tools_v3.compact_db();
        return 0;
    }

    if (function_name == "db_prune") {
        if (argc != 4) {
            usage();
            return -1;
        }
        std::string v3_db_path = argv[3];
        xdb_export_tools_t tools_v3{v3_db_path};
        std::cout << "db_prune start" << std::endl;
        tools_v3.prune_db();
        return 0;
    }

    if (function_name == "db_migrate_v2_to_v3") {
        if (argc != 4) {
            xassert(false);
            usage();
            return -1;
        }
        std::string v2_db_path = db_path;
        std::string v3_db_path = argv[3];
        base::db_migrate_v2_to_v3(v2_db_path, v3_db_path);
        return 0;
    }

    if(function_name == "db_parse_type_size") {
        if (argc != 4) {
            xassert(false);
            usage();
            return -1;
        }

        std::cout << "db_parse_type_size start" << std::endl;
        std::string v3_db_path = argv[3];
        xdb_export_tools_t tools_v3{v3_db_path};
        std::string dir{"db_parse_result/"};
        tools_v3.set_outfile_folder(dir);
        std::string  result_file = "db_parse_result";
        std::vector<std::string> pathNames;
        base::xstring_utl::split_string(v3_db_path, '/', pathNames);
        if (pathNames.size() > 1) {
            result_file = pathNames.back();
        }
        tools_v3.db_parse_type_size(result_file);
        return 0;
    }

    xdb_export_tools_t tools{db_path};
    if (function_name == "check_fast_sync" || function_name == "check_state_data") {
        bool check_block = (function_name == "check_fast_sync");
        if (argc == 3) {
            auto const table_account_vec = xdb_export_tools_t::get_table_accounts();
            auto const unit_account_vec = tools.get_db_unit_accounts();
            if (check_block) {
                tools.query_all_account_data(table_account_vec, true, xdb_check_data_func_block_t());
                tools.query_all_account_data(unit_account_vec, false, xdb_check_data_func_block_t());
            } else {
                tools.query_all_account_data(table_account_vec, true, xdb_check_data_func_table_state_t());
                tools.query_all_account_data(unit_account_vec, false, xdb_check_data_func_unit_state_t());
            }
        } else if (argc == 4) {
            std::string method_name{argv[3]};
            if (method_name == "table") {
                auto const table_account_vec = xdb_export_tools_t::get_table_accounts();
                if (check_block) {
                    tools.query_all_account_data(table_account_vec, true, xdb_check_data_func_block_t());
                } else {
                    tools.query_all_account_data(table_account_vec, true, xdb_check_data_func_table_state_t());
                }
            } else if (method_name == "unit") {
                auto const unit_account_vec = tools.get_db_unit_accounts();
                if (check_block) {
                    tools.query_all_account_data(unit_account_vec, false, xdb_check_data_func_block_t());
                } else {
                    tools.query_all_account_data(unit_account_vec, false, xdb_check_data_func_unit_state_t());
                }
                std::cout << "not support currently." << std::endl;
            } else if (method_name == "account") {
                std::vector<std::string> account = {argv[4]};
                if (check_block) {
                    tools.query_all_account_data(account, false, xdb_check_data_func_block_t());
                } else {
                    tools.query_all_account_data(account, false, xdb_check_data_func_unit_state_t());
                }
            }else {
                usage();
                return -1;
            }
        }
    } else if (function_name == "check_off_data") {
        auto const table_account_vec = xdb_export_tools_t::get_table_accounts();
        tools.query_all_account_data(table_account_vec, true, xdb_check_data_func_off_data_t());
    } else if (function_name == "check_mpt") {
        auto const table_account_vec = xdb_export_tools_t::get_table_accounts();
        tools.query_all_table_mpt(table_account_vec);
    } else if (function_name == "check_performance") {
        tools.set_outfile_folder("performance_result/");
        auto const table_account_vec = tools.get_table_accounts();
        tools.query_all_table_performance(table_account_vec);
    } else if (function_name == "check_tx_info") {
        uint32_t thread_num = 0;
        uint32_t start_timestamp = 0;
        uint32_t end_timestamp = UINT_MAX;
        char * start_time_str = nullptr;
        char * end_time_str = nullptr;
        if (argc == 4) {
            thread_num = std::stoi(argv[3]);
        } else if (argc == 5) {
            start_time_str = argv[3];
            end_time_str = argv[4];
        } else if (argc == 6) {
            start_time_str = argv[3];
            end_time_str = argv[4];
            thread_num = std::stoi(argv[5]);
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
        }

        tools.set_outfile_folder("all_table_tx_info/");
        auto const account_vec = xdb_export_tools_t::get_table_accounts();
        tools.read_external_tx_firestamp();
        tools.query_tx_info(account_vec, thread_num, start_timestamp, end_timestamp);
    } else if (function_name == "check_block_exist") {
        if (argc < 5) {
            usage();
            return -1;
        }
        tools.query_block_exist(argv[3], std::stoi(argv[4]));
    }else if (function_name == "check_block_info") {
        if (argc < 5) {
            usage();
            return -1;
        }
        tools.query_block_info(argv[3], argv[4]);
    } else if (function_name == "check_block_basic") {
        if (argc < 3 || argc > 5) {
            usage();
            return -1;
        }
        tools.set_outfile_folder("all_block_basic_info/");
        if (argc == 3) {
            auto const unit_account_vec = tools.get_db_unit_accounts();
            tools.query_block_basic(unit_account_vec, "all");
        } else if (argc == 4) {
            tools.query_block_basic(argv[3], "all");
        } else if (argc == 5) {
            tools.query_block_basic(argv[3], argv[4]);
        }
    } else if (function_name == "check_state_basic") {
        if (argc < 3 || argc > 5) {
            usage();
            return -1;
        }
        tools.set_outfile_folder("all_state_basic_info/");
        if (argc == 3) {
            auto const unit_account_vec = tools.get_db_unit_accounts();
            tools.query_state_basic(unit_account_vec, "all");
        } else if (argc == 4) {
            tools.query_state_basic(argv[3], "all");
        } else if (argc == 5) {
            tools.query_state_basic(argv[3], argv[4]);
        }
    } else if (function_name == "check_meta") {
        if (argc < 3 || argc > 4) {
            usage();
            return -1;
        }
        if (argc == 3) {
            auto const unit_account_vec = tools.get_table_accounts();
            tools.query_meta(unit_account_vec);
        } else if (argc == 4) {
            tools.query_meta(argv[3]);
        }
    } else if (function_name == "check_latest_fullblock") {
        tools.query_table_latest_fullblock();
    } else if (function_name == "check_contract_property") {
        if (argc < 6) {
            usage();
            return -1;
        }
        tools.query_property(argv[3], argv[4], argv[5]);
    } else if (function_name == "check_balance") {
        tools.query_balance();
    } else if (function_name == "check_archive_db") {
        if (argc != 4) {
            usage();
            return -1;
        }
        std::map<common::xtable_address_t, uint64_t> table_query_criteria = xdbtool_util_t::parse_table_addr_height_list(argv[3]);
        if (table_query_criteria.empty()) {
            usage();
            return -1;            
        }

        auto t1 = base::xtime_utl::time_now_ms();
        tools.query_archive_db(table_query_criteria, 0);
        auto t2 = base::xtime_utl::time_now_ms();
        std::cout << "check_archive_db total time: " << (t2 - t1) / 1000 << "s." << std::endl;
    } else if (function_name == "parse_checkpoint") {
        if (argc != 4) {
            usage();
            return -1;
        }
        auto t1 = base::xtime_utl::time_now_ms();
        tools.query_checkpoint(std::stoi(argv[3]));
        auto t2 = base::xtime_utl::time_now_ms();
        std::cout << "parse_checkpoint total time: " << (t2 - t1) / 1000 << "s." << std::endl;
    } else if (function_name == "parse_db") {
        tools.set_outfile_folder("parse_db_result/");
        auto const table_account_vec = tools.get_table_accounts();
        auto t1 = base::xtime_utl::time_now_ms();
        tools.query_table_unit_info(table_account_vec);
        auto t2 = base::xtime_utl::time_now_ms();
        std::cout << "parse_db total time: " << (t2 - t1) / 1000 << "s." << std::endl;
    } else if (function_name == "export") {
        // xdb_export db_path export exported.json table0:height0,table1:height1,... [account0,[account1,...]]

        if (argc < 4) {
            usage();
            return -1;
        }

        auto const & table_query = argv[3];
        std::string account_string;
        if (argc == 5) {
            account_string = argv[4];
        }

        std::vector<std::string> table_info;
        top::base::xstring_utl::split_string(table_query, ',', table_info);
        std::map<common::xtable_address_t, uint64_t> table_query_criteria;
        for (auto const & t : table_info) {
            std::vector<std::string> pair;
            top::base::xstring_utl::split_string(t, ':', pair);

            table_query_criteria.emplace(common::xtable_address_t::build_from(pair[0]), std::stoull(pair[1]));
        }

        std::vector<common::xaccount_address_t> queried_accounts;
        if (!account_string.empty()) {
            std::vector<std::string> accounts;
            base::xstring_utl::split_string(account_string, ',', accounts);
            std::transform(std::begin(accounts), std::end(accounts), std::back_inserter(queried_accounts), [](std::string const & acc) { return common::xaccount_address_t::build_from(acc);
            });
        }

        auto genesis_loader = std::make_shared<loader::xconfig_genesis_loader_t>("");
        data::xrootblock_para_t rootblock_para;
        genesis_loader->extract_genesis_para(rootblock_para);
        data::xrootblock_t::init(rootblock_para);
        auto const & genesis_data = rootblock_para.m_account_balances;
        std::unordered_map<common::xaccount_address_t, evm_common::u256> genesis_accounts_data;
        for (auto const & genesis_account_datum : genesis_data) {
            genesis_accounts_data.emplace(common::xaccount_address_t::build_from(top::get<std::string const>(genesis_account_datum)), top::get<uint64_t>(genesis_account_datum));
        }

#pragma omp parallel for
        for (uint i = 0; i < table_query_criteria.size(); ++i) {
            auto it = std::next(std::begin(table_query_criteria), i);
            auto const & tbl = *it;

            std::error_code ec;

            auto const & table_address = top::get<common::xtable_address_t const>(tbl);
            auto const table_height = top::get<uint64_t>(tbl);

            std::string json_file_name = table_address.to_string() + '_' + std::to_string(table_height) + ".json";

            auto units = tools.get_unit_accounts(table_address, table_height, queried_accounts, ec);
            if (ec) {
                std::cerr << "get_unit_accounts on table " << table_address.to_string() << " at height " << table_height << " failed with error code " << ec.value() << " msg "
                          << ec.message() << std::endl;
                continue;
            }

            auto const result = tools.get_account_data(units,
                                                       genesis_accounts_data,
                                                       {common::xtoken_id_t::top, common::xtoken_id_t::eth, common::xtoken_id_t::usdt, common::xtoken_id_t::usdc},
                                                       std::unordered_map<std::string, bool>{},
                                                       table_address,
                                                       ec);
            tools.export_to_json(table_address, table_height, result, json_file_name, account_string.empty() ? std::ios_base::trunc : std::ios_base::app, ec);
        }
    } else {
        usage();
    }

    return 0;
}
