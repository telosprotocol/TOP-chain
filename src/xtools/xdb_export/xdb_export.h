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
};

NS_END2
