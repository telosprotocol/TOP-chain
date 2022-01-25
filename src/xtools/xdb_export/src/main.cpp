#include "../xdb_export.h"
#include "../xdb_reset.h"
#include "xbase/xhash.h"
#include "xmigrate/xvmigrate.h"
#include "xconfig/xpredefined_configurations.h"

using namespace top;
using namespace top::db_export;

// #define XDB_EXPORT_LOG

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
    std::cout << "        - compact_db new_path" << std::endl;
    std::cout << "        - check_fast_sync <account>" << std::endl;
    std::cout << "        - check_block_exist <account> <height>" << std::endl;
    std::cout << "        - check_block_info <account> <height|last|all>" << std::endl;
    std::cout << "        - check_tx_info [starttime] [endtime] [threads]" << std::endl;
    std::cout << "        - check_latest_fullblock" << std::endl;
    std::cout << "        - check_contract_property <account> <property> <height|last|all>" << std::endl;
    std::cout << "        - check_balance" << std::endl;
    std::cout << "        - check_archive_db [redundancy]" << std::endl;
    std::cout << "        - parse_checkpoint <height>" << std::endl;
    std::cout << "        - parse_db" << std::endl;
    std::cout << "        - read_meta <account>" << std::endl;
    std::cout << "        - parse_type_size [db_path] " << std::endl;
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

    if (function_name == "compact_db") {
        if (argc != 4) {
            xassert(false);
            usage();
            return -1;
        }
        std::string v3_db_path = argv[3];
        xdb_export_tools_t tools_v3{v3_db_path};
        tools_v3.compact_db();
        return 0;
    } else if(function_name == "parse_type_size") {
        if (argc != 4) {
            xassert(false);
            usage();
            return -1;
        }

        std::cout << "parse_type_size start" << std::endl;
        std::string v3_db_path = argv[3];
        xdb_export_tools_t tools_v3{v3_db_path};
        std::string dir{"parse_type_size/"};
        tools_v3.set_outfile_folder(dir);
        std::string  result_file = "result_parse_type_size";
        std::vector<std::string> pathNames;
        base::xstring_utl::split_string(v3_db_path, '/', pathNames);
        if (pathNames.size() > 1) {
            result_file = pathNames.back();
        }
        tools_v3.parse_all(result_file);
        return 0;
    }

    xdb_export_tools_t tools{db_path};
    if (function_name == "check_fast_sync") {
        if (argc == 3) {
            auto const table_account_vec = xdb_export_tools_t::get_table_accounts();
            auto const unit_account_vec = tools.get_db_unit_accounts();
            tools.query_all_sync_result(table_account_vec, true);
            tools.query_all_sync_result(unit_account_vec, false);
        } else if (argc == 4) {
            std::string method_name{argv[3]};
            if (method_name == "table") {
                auto const table_account_vec = xdb_export_tools_t::get_table_accounts();
                tools.query_all_sync_result(table_account_vec, true);
            } else if (method_name == "unit") {
                auto const unit_account_vec = tools.get_db_unit_accounts();
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
        tools.query_tx_info(account_vec, thread_num, start_timestamp, end_timestamp);
    } else if (function_name == "check_block_exist") {
        if (argc < 5) {
            usage();
            return -1;
        }
        tools.query_block_exist(argv[3], std::stoi(argv[4]));
    }  else if (function_name == "read_meta") {
        if (argc < 4) {
            usage();
            return -1;
        }
        std::string address = argv[3];
        tools.read_meta(address);
    } else if (function_name == "check_block_info") {
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
            auto const unit_account_vec = tools.get_db_unit_accounts();
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
        if (argc != 3 && argc != 4) {
            usage();
            return -1;
        }
        uint32_t redundancy = 0;
        if (argc == 4) {
            redundancy = std::stoi(argv[3]);
        }
        auto t1 = base::xtime_utl::time_now_ms();
        tools.query_archive_db(redundancy);
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
    } else {
        usage();
    }

    return 0;
}
