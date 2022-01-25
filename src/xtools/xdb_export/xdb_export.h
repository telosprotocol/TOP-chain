#pragma once

#include "xbasic/xtimer_driver.h"
#include "xdb_util.h"
#include "xgrpcservice/xgrpc_service.h"
#include "xmbus/xmessage_bus.h"
#include "xstore/xstore_face.h"
#include "xtxstore/xtxstore_face.h"
#include "xvledger/xvcnode.h"

NS_BEG2(top, db_export)
class xdb_export_tools_t {
public:
    enum enum_query_account_type { query_account_table = 0, query_account_unit, query_account_system};

    xdb_export_tools_t(std::string const & db_path);

    static std::vector<std::string> get_system_contract_accounts();
    static std::vector<std::string> get_table_accounts();
    std::vector<std::string> get_db_unit_accounts();
    void query_all_sync_result(std::vector<std::string> const & accounts_vec, bool is_table);
    void query_table_latest_fullblock();
    void query_table_tx_info(std::vector<std::string> const & address_vec, const uint32_t start_timestamp, const uint32_t end_timestamp);
    // query if a specific block is exist(include num)
    void query_block_exist(std::string const & address, const uint64_t height);
    // query block detailed info(use grpc interface)
    void query_block_info(std::string const & account, std::string const & param);
    // query block basic info
    void query_block_basic(std::string const & account, std::string const & param);
    void query_block_basic(std::vector<std::string> const & account_vec, std::string const & param);
    // query state basic info
    void query_state_basic(std::string const & account, std::string const & param);
    void query_state_basic(std::vector<std::string> const & account_vec, std::string const & param);
    // query meta info
    void query_meta(std::string const & account);
    void query_meta(std::vector<std::string> const & account_vec);
    // query table state and its all units state
    void query_table_unit_info(std::string const & table);
    void query_table_unit_info(std::vector<std::string> const & table_vec);
    // query contract property(use contract manager interface)
    void query_property(std::string const & account, std::string const & prop_name, std::string const & param);
    // query balance info
    void query_balance();
    // query archive db integrity and continuity
    void query_archive_db();
    // set folder
    void set_outfile_folder(std::string const & folder);
    void compact_db();
    void read_meta(std::string const & address);

    static bool  db_scan_key_callback(const std::string& key, const std::string& value,void*cookie);
    bool  db_scan_key_callback(const std::string& key, const std::string& value);
    void parse_all(const std::string &fileName);
    std::string get_account_key_string(const std::string & key);
private:
    struct tx_ext_t {
        std::string hash{""};
        int32_t tableid{-1};
        uint64_t height{0};
        uint64_t timestamp{0};
        std::string src{""};
        std::string target{""};
        uint64_t unit_height{0};
        uint8_t phase{0};
        uint64_t fire_timestamp{0}; // origin tx fire timestamp
    };

    struct xdbtool_table_info_t {
        int empty_table_block_num{0};
        int light_table_block_num{0};
        int full_table_block_num{0};
        int missing_table_block_num{0};

        int empty_unit_block_num{0};
        int light_unit_block_num{0};
        int full_unit_block_num{0};
        int total_unit_block_num{0};

        int sendtx_num{0};
        int recvtx_num{0};
        int confirmtx_num{0};
        int selftx_num{0};
        int confirmedtx_num{0};

        uint32_t tx_v1_num{0};
        uint64_t tx_v1_total_size{0};
        uint32_t tx_v2_num{0};
        uint64_t tx_v2_total_size{0};

        uint64_t total_confirm_time_from_send{0};
        uint64_t total_confirm_time_from_fire{0};
        uint64_t max_confirm_time_from_send{0};
        uint64_t max_confirm_time_from_fire{0};
    };

      struct xdbtool_dbsize_key_type_info_t {
        size_t key_count{0};
        size_t key_value_size{0};
    };


// old
    struct xdbtool_dbsize_t {
        size_t all_key_count{0};
        size_t all_key_value_size{0};
        size_t all_key_size{0};
        std::map<int, xdbtool_dbsize_key_type_info_t>   key_type_info;

        size_t unit_key_count{0};
        size_t unit_value_size{0};
        size_t table_key_count{0};
        size_t table_value_size{0};

        void add_key_type(const std::string & key, base::enum_xdbkey_type type, size_t key_size, size_t value_size) {
            all_key_count++;
            all_key_size += key_size;
            all_key_value_size += value_size;

            auto iter = key_type_info.find(type);
            if (iter != key_type_info.end()) {
                iter->second.key_value_size += value_size;
                iter->second.key_count++;
            } else {
                xdbtool_dbsize_key_type_info_t info;
                info.key_count = 1;
                info.key_value_size = value_size;
                key_type_info[type] = info;
            }
            if (type == base::enum_xdbkey_type_block_object
                || type == base::enum_xdbkey_type_block_input_resource
                || type == base::enum_xdbkey_type_block_output_resource
                || type == base::enum_xdbkey_type_unit_proof
                || type == base::enum_xdbkey_type_block_index
                || type == base::enum_xdbkey_type_state_object) {
                if (key.find("Ta") != std::string::npos) {
                    add_table_key_type(value_size);
                } else {
                    add_unit_key_type(value_size);
                }
            }
        }
        void add_unit_key_type(size_t value_size) {
            unit_key_count++;
            unit_value_size += value_size;
        }
        void add_table_key_type(size_t value_size) {
            table_key_count++;
            table_value_size += value_size;
        }

        void to_json(json & root) {
            root["all_key_count"] = std::to_string(all_key_count);
            root["all_key_size"] = std::to_string(all_key_size);
            root["all_key_value_size"] = std::to_string(all_key_value_size);
            uint64_t all_key_key_size_avg = all_key_size / all_key_count;
            uint64_t all_key_value_size_avg = all_key_value_size / all_key_count;
            root["all_key_key_size_avg"] = std::to_string(all_key_key_size_avg);
            root["all_key_value_size_avg"] = std::to_string(all_key_value_size_avg);

            root["unit_key_count"] = std::to_string(unit_key_count);
            root["unit_value_size"] = std::to_string(unit_value_size);
            uint64_t unit_value_size_avg = unit_value_size / unit_key_count;
            root["unit_value_size_avg"] = std::to_string(unit_value_size_avg);

            root["table_key_count"] = std::to_string(table_key_count);
            root["table_value_size"] = std::to_string(table_value_size);
            uint64_t table_value_size_avg = table_value_size / table_key_count;
            root["table_value_size_avg"] = std::to_string(table_value_size_avg);

            for (auto & v : key_type_info) {
                std::string key_type_str = base::xvdbkey_t::get_dbkey_type_name((base::enum_xdbkey_type)v.first);
                root["type_key_count_" + key_type_str] = std::to_string(v.second.key_count);
                root["type_key_value_size_" + key_type_str] = std::to_string(v.second.key_value_size);
                root["type_key_value_size_avg" + key_type_str] = std::to_string(v.second.key_value_size / v.second.key_count);
            }
        }
    };

//end 

    std::set<std::string> query_db_unit_accounts();
    std::set<std::string> generate_db_unit_accounts_file();
    std::set<std::string> query_unit_account2(std::string const & account);
    void query_sync_result(std::string const & account, const uint64_t h_s, const uint64_t h_e, std::string & result, int init_s = -1, int init_e = -1);
    void query_sync_result(std::string const & account, json & result_json);
    void query_table_latest_fullblock(std::string const & account, json & j);
    void query_table_tx_info(std::string const & account, const uint32_t start_timestamp, const uint32_t end_timestamp, json & result_json);
    void query_block_info(std::string const & account, const uint64_t h, xJson::Value & root);
    void query_block_state_basic(std::string const & account, const uint64_t h, json & result);
    void query_block_basic(std::string const & account, const uint64_t h, json & result);
    void query_state_basic(std::string const & account, const uint64_t h, json & result);
    void query_meta(std::string const & account, json & result);
    void query_table_unit_state(std::string const & table, json & result);
    void query_property(std::string const & account, std::string const & prop_name, const uint64_t height, json & j);
    void query_balance(std::string const & table, json & j_unit, json & j_table);
    uint32_t query_block_continuity_and_integrity(std::string const & account, enum_query_account_type type, std::ofstream & file);
    uint32_t query_block_continuity(std::string const & account, enum_query_account_type type, std::ofstream & file);
    uint32_t query_cert_continuity(std::string const & account, enum_query_account_type type, std::ofstream & file);

    void read_info_from_table_block(const data::xblock_t * block, xdbtool_table_info_t & table_info, std::vector<tx_ext_t> & txinfos);
    void set_txinfo_to_json(json & j, const tx_ext_t & txinfo);
    void set_txinfos_to_json(json & j, const std::map<std::string, tx_ext_t> & txinfos);
    void set_txinfos_to_json(json & j, const std::vector<tx_ext_t> & txinfos);
    void set_confirmed_txinfo_to_json(json & j, const tx_ext_t & send_txinfo, const tx_ext_t & confirm_txinfo);
    void set_table_txdelay_time(xdbtool_table_info_t & table_info, const tx_ext_t & send_txinfo, const tx_ext_t & confirm_txinfo);

    std::set<std::string> get_special_genesis_accounts();
    std::set<std::string> get_db_unit_accounts_v2();
    void load_db_unit_accounts_info();
    void generate_db_unit_accounts_data_file();
    void generate_account_info_file(std::string const & account, const uint64_t height);
    void generate_json_file(std::string const & filename, json const & j);

    std::unique_ptr<xbase_timer_driver_t> m_timer_driver;
    xobject_ptr_t<mbus::xmessage_bus_face_t> m_bus;
    xobject_ptr_t<store::xstore_face_t> m_store;
    xobject_ptr_t<base::xvblockstore_t> m_blockstore;
    xobject_ptr_t<base::xvtxstore_t> m_txstore;
    xobject_ptr_t<base::xvnodesrv_t> m_nodesvr_ptr;
    std::shared_ptr<rpc::xrpc_handle_face_t> m_getblock;

    std::map<std::string, std::map<std::string, base::xaccount_index_t>> m_db_units_info;
    std::string m_outfile_folder;
    
    
    
    struct xdbtool_parse_info_t {
        uint32_t count;
        uint32_t input_count;
        uint32_t output_count; 
        uint32_t proof_count;
        uint32_t block_count; 
        uint32_t state_count; 
        uint32_t account_number;
        uint64_t index_count;
        uint64_t size;
        uint64_t input_size;
        uint64_t output_size;
        uint64_t proof_size;
        uint64_t block_size;
        uint64_t state_size;
        uint64_t index_size;
    };
    void parse_info_set(xdbtool_parse_info_t &info, int db_key_type,uint64_t value_size);
    std::map<std::string, xdbtool_parse_info_t> m_db_parse_info;
    std::map<std::string, xdbtool_parse_info_t> m_db_sum_info;
    uint64_t  m_info_key_count;
    uint64_t  m_info_account_count;
    xdbtool_dbsize_t m_dbsize_info;
    void vector_to_json(std::map<std::string, xdbtool_parse_info_t> &db_info, json &json_root);
};

NS_END2
