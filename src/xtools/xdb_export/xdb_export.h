#pragma once

#include "xbase/xobject_ptr.h"
#include "xdb_util.h"
#include "xdepends/include/json/config.h"
#include "xdepends/include/json/value.h"
#include "xgrpcservice/xgrpc_service.h"
#include "xmbus/xmessage_bus.h"
#include "xstore/xstore_face.h"
#include "xvledger/xvcnode.h"

NS_BEG2(top, db_export)

class xdb_export_tools_t {
public:
    xdb_export_tools_t(std::string const & db_path);
    ~xdb_export_tools_t() = default;

    static std::vector<std::string> get_unit_contract_accounts();
    static std::vector<std::string> get_table_contract_accounts();
    std::vector<std::string> get_db_unit_accounts();
    void query_all_sync_result(std::vector<std::string> const & accounts_vec, bool is_table);
    void query_table_latest_fullblock();
    void query_table_tx_info(std::vector<std::string> const & address_vec, const uint32_t start_timestamp, const uint32_t end_timestamp);
    // not used
    void query_block_num();
    // query if a specific block is exist(include num)
    void query_block_exist(std::string const & address, const uint64_t height);
    // query block detailed info(use grpc interface)
    void query_block_info(std::string const & account, std::string const & param);
    // query block basic info
    void query_block_basic(std::string const & account, std::string const & param);
    // query state basic info
    void query_state_basic(std::string const & account, std::string const & param);
    // query table state and its all units state
    void query_table_unit_state(std::string const & table);
    // query contract property(use contract manager interface)
    void query_contract_property(std::string const & account, std::string const & prop_name, std::string const & param);
    // query balance info
    void query_balance();

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
    void generate_db_unit_accounts_file_v2(std::set<std::string> & accounts_set);
    void query_unit_account(std::string const & account, std::set<std::string> & accounts_set);
    void query_unit_account2(std::string const & account, std::set<std::string> & accounts_set);
    void query_sync_result(std::string const & account, const uint64_t h_s, const uint64_t h_e, std::string & result, int init_s = -1, int init_e = -1);
    void query_sync_result(std::string const & account, json & result_json);
    void query_table_latest_fullblock(std::string const & account, json & j);
    void query_table_tx_info(std::string const & account, const uint32_t start_timestamp, const uint32_t end_timestamp, json & result_json);
    void query_block_info(std::string const & account, const uint64_t h, xJson::Value & root);
    void query_block_basic(std::string const & account, const uint64_t h, json & result);
    void query_state_basic(std::string const & account, const uint64_t h, json & result);
    void query_table_unit_state(std::string const & table, json & result);
    void query_contract_property(std::string const & account, std::string const & prop_name, uint64_t height, xJson::Value & jph);
    void query_balance(std::string const & table, json & j_unit, json & j_table);

    void read_info_from_table_block(const data::xblock_t * block, xdbtool_table_info_t & table_info, std::vector<tx_ext_t> & txinfos);
    void set_txinfo_to_json(json & j, const tx_ext_t & txinfo);
    void set_txinfos_to_json(json & j, const std::map<std::string, tx_ext_t> & txinfos);
    void set_txinfos_to_json(json & j, const std::vector<tx_ext_t> & txinfos);
    void set_confirmed_txinfo_to_json(json & j, const tx_ext_t & send_txinfo, const tx_ext_t & confirm_txinfo);
    void set_table_txdelay_time(xdbtool_table_info_t & table_info, const tx_ext_t & send_txinfo, const tx_ext_t & confirm_txinfo);

    xobject_ptr_t<mbus::xmessage_bus_face_t> m_bus;
    xobject_ptr_t<store::xstore_face_t> m_store;
    xobject_ptr_t<base::xvblockstore_t> m_blockstore;
    xobject_ptr_t<base::xvnodesrv_t> m_nodesvr_ptr;
    std::shared_ptr<rpc::xrpc_handle_face_t> m_getblock;
};

NS_END2