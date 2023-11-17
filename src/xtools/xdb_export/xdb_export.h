#pragma once

#include "xbasic/xtimer_driver.h"
#include "xdb_util.h"
#include "xgrpcservice/xgrpc_service.h"
#include "xmbus/xmessage_bus.h"
#include "xdbstore/xstore_face.h"
#include "xtxstore/xtxstore_face.h"
#include "xvledger/xvcnode.h"
#include "xvledger/xvtxindex.h"
#include "xdata/xlightunit_info.h"
#include "xdata/xblock.h"
#include "xcommon/xtoken_metadata.h"
#include "xgenesis/xgenesis_manager.h"

NS_BEG2(top, db_export)

class xdb_check_data_func_face_t {
public:
    virtual bool is_data_exist(base::xvblockstore_t * blockstore, std::string const & account, uint64_t height) const = 0;
    virtual std::string data_type() const = 0;
};

class xdb_check_data_func_block_t : public xdb_check_data_func_face_t {
    virtual bool is_data_exist(base::xvblockstore_t * blockstore, std::string const & account, uint64_t height) const override;
    virtual std::string data_type() const override;
};

class xdb_check_data_func_table_state_t : public xdb_check_data_func_face_t {
    virtual bool is_data_exist(base::xvblockstore_t * blockstore, std::string const & account, uint64_t height) const override;
    virtual std::string data_type() const override;
};

class xdb_check_data_func_unit_state_t : public xdb_check_data_func_face_t {
    virtual bool is_data_exist(base::xvblockstore_t * blockstore, std::string const & account, uint64_t height) const override;
    virtual std::string data_type() const override;
};

class xdb_check_data_func_off_data_t : public xdb_check_data_func_face_t {
    virtual bool is_data_exist(base::xvblockstore_t * blockstore, std::string const & account, uint64_t height) const override;
    virtual std::string data_type() const override;
};

struct xdb_archive_check_info_t {
    uint64_t total_tables{0};
    uint64_t total_units{0};
    uint64_t total_txindexs{0};
};

class xdb_export_tools_t {
public:
    xdb_export_tools_t(std::string const & db_path);
    ~xdb_export_tools_t();
    static std::vector<std::string> get_system_contract_accounts();
    static std::vector<std::string> get_table_accounts();
    std::vector<std::string> get_db_unit_accounts();
    void query_all_account_data(std::vector<std::string> const & accounts_vec, bool is_table, const xdb_check_data_func_face_t & func);
    void query_all_table_mpt(std::vector<std::string> const & accounts_vec);
    void query_table_latest_fullblock();
    void read_external_tx_firestamp();
    void query_tx_info(std::vector<std::string> const & tables, const uint32_t thread_num, const uint32_t start_timestamp, const uint32_t end_timestamp);
    // query if a specific block is exist(include num)
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
    void query_archive_db(std::map<common::xtable_address_t, uint64_t> const& table_query_criteria);
    // query checkpoint
    void query_checkpoint(const uint64_t clock);
    // set folder
    void set_outfile_folder(std::string const & folder);
    void compact_db();
    static bool  db_scan_key_callback(const std::string& key, const std::string& value,void*cookie);
    bool   db_scan_key_callback(const std::string& key, const std::string& value);
    void   db_parse_type_size(const std::string &fileName);
    std::string get_account_key_string(const std::string & key);
    void   prune_db();
    void   query_all_table_performance(std::vector<std::string> const & accounts_vec);
    void execute_all_table_blocks(std::vector<std::string> const & table_vec);    

    std::unordered_map<common::xaccount_address_t, base::xaccount_index_t> get_unit_accounts(common::xtable_address_t const & table_address,
                                                                                             std::uint64_t table_height,
                                                                                             std::vector<common::xaccount_address_t> const & designated,
                                                                                             std::error_code & ec) const;

    struct exported_account_data {
        common::xaccount_address_t account_address;
        std::array<std::unordered_map<std::string, evm_common::u256>, 2> assets;    // index 0: TOP, index 1: TEP1
        std::unordered_map<std::string, xbytes_t> binary_properties;
        std::unordered_map<std::string, std::string> text_properties;
        uint64_t unit_height;
    };

    std::vector<exported_account_data> get_account_data(std::unordered_map<common::xaccount_address_t, base::xaccount_index_t> const & accounts,
                                                        std::unordered_map<common::xaccount_address_t, evm_common::u256> const & genesis_account_data,
                                                        std::vector<common::xtoken_id_t> const & queried_tokens,
                                                        std::unordered_map<std::string, bool> const & queried_properties,
                                                        common::xtable_address_t const & table_address,
                                                        std::error_code & ec) const;

    void export_to_json(common::xtable_address_t const & table_address,
                        uint64_t table_height,
                        std::vector<exported_account_data> const & data,
                        std::string const & file_path,
                        std::ios_base::openmode open_mode,
                        std::error_code & ec) const;

    struct exported_account_bstate_data {
        common::xaccount_address_t account_address;
        xbytes_t serialized_bstate;
        uint64_t unit_height{0};
    };

    std::vector<exported_account_bstate_data> get_account_bstate_data(std::unordered_map<common::xaccount_address_t, base::xaccount_index_t> const & accounts,
                                                                      std::unordered_set<common::xaccount_address_t> const & genesis_accounts,
                                                                      common::xtable_address_t const & table_address,
                                                                      std::error_code & ec) const;

    void export_to_json(common::xtable_address_t const & table_address,
                        uint64_t table_height,
                        std::vector<exported_account_bstate_data> const & data,
                        std::string const & file_path,
                        std::ios_base::openmode const open_mode,
                        std::error_code & ec) const;

private:
    enum enum_tx_consensus_phase_type : uint8_t {
        enum_tx_consensus_phase_type_unkwown,
        enum_tx_consensus_phase_type_one,
        enum_tx_consensus_phase_type_two,
        enum_tx_consensus_phase_type_three,
        enum_tx_consensus_phase_type_max,
    };

    struct tx_ext_t {
        std::string get_hex_hash() const {return base::xstring_utl::to_hex(hash);}
        std::string hash{};
        enum_tx_consensus_phase_type cons_phase_type{enum_tx_consensus_phase_type_unkwown};
        uint8_t phase{0};
        base::xtable_shortid_t  sendtableid;
        uint64_t block_timestamp{0};
        uint64_t fire_timestamp{0}; // origin tx fire timestamp
    };

    struct tx_ext_sum_t {
        std::string get_hex_hash() const {return base::xstring_utl::to_hex(hash);}
        std::string hash{};
        enum_tx_consensus_phase_type cons_phase_type{enum_tx_consensus_phase_type_max};
        uint8_t actions_count{0};
        base::xtable_shortid_t  sendtableid;
        uint32_t fire_timestamp{0}; // origin tx fire timestamp
        uint64_t send_block_timestamp{0};
        uint64_t recv_block_timestamp{0};
        uint64_t confirm_block_timestamp{0};

        tx_ext_sum_t() {
        }

        tx_ext_sum_t(const tx_ext_t & tx_ext) {
            hash = tx_ext.hash;
            cons_phase_type = tx_ext.cons_phase_type;
            if (cons_phase_type != enum_tx_consensus_phase_type_one && cons_phase_type != enum_tx_consensus_phase_type_two && cons_phase_type != enum_tx_consensus_phase_type_three) {
                std::cout << "tx_ext_sum_t error cons_phase_type " << (uint32_t)cons_phase_type << std::endl;
                std::__throw_bad_exception();
            }

            sendtableid = tx_ext.sendtableid;
            fire_timestamp = tx_ext.fire_timestamp;
            actions_count = 1;
            if (tx_ext.phase == base::enum_transaction_subtype_self || tx_ext.phase == base::enum_transaction_subtype_send) {
                send_block_timestamp = tx_ext.block_timestamp;
            } else if (tx_ext.phase == base::enum_transaction_subtype_recv) {
                recv_block_timestamp = tx_ext.block_timestamp;
            } else {
                confirm_block_timestamp = tx_ext.block_timestamp;
            }
        }

        void copy_tx_ext(const tx_ext_t & tx_ext) {
            actions_count++;
            if (hash != tx_ext.hash || cons_phase_type != tx_ext.cons_phase_type || sendtableid != tx_ext.sendtableid || tx_ext.block_timestamp == 0) {
                std::cout << "copy_tx_ext error cons_phase_type " << (uint32_t)cons_phase_type << std::endl;
                std::__throw_bad_exception();
            }
            if (tx_ext.phase == base::enum_transaction_subtype_self || tx_ext.phase == base::enum_transaction_subtype_send) {
                send_block_timestamp = tx_ext.block_timestamp;
                fire_timestamp = tx_ext.fire_timestamp;
            } else if (tx_ext.phase == base::enum_transaction_subtype_recv) {
                recv_block_timestamp = tx_ext.block_timestamp;
            } else {
                confirm_block_timestamp = tx_ext.block_timestamp;
            }
        }

        bool is_confirmed() {
            if (cons_phase_type == enum_tx_consensus_phase_type_one) {
                return true;
            } else if (cons_phase_type == enum_tx_consensus_phase_type_two) {
                if (send_block_timestamp != 0 && recv_block_timestamp != 0) {
                    return true;
                }
            } else if (cons_phase_type == enum_tx_consensus_phase_type_three) {
                if (send_block_timestamp != 0 && recv_block_timestamp != 0 && confirm_block_timestamp != 0) {
                    return true;
                }                
            } else {
                std::cout << "is_confirmed error cons_phase_type " << (uint32_t)cons_phase_type << std::endl;
                std::__throw_bad_exception();
            }

            return false;
        }

        uint32_t    get_delay_from_send_to_confirm() const {
            if (cons_phase_type == enum_tx_consensus_phase_type_one) {
                return 0;
            } else if (cons_phase_type == enum_tx_consensus_phase_type_two) {
                return recv_block_timestamp > send_block_timestamp ? (recv_block_timestamp - send_block_timestamp) : 0;
            } else {
                return confirm_block_timestamp > send_block_timestamp ? (confirm_block_timestamp - send_block_timestamp) : 0;              
            }
        }
        uint32_t   get_delay_from_fire_to_confirm() const {
            if (cons_phase_type == enum_tx_consensus_phase_type_one) {
                return send_block_timestamp > fire_timestamp ? send_block_timestamp - fire_timestamp : 0;
            } else if (cons_phase_type == enum_tx_consensus_phase_type_two) {
                return recv_block_timestamp > fire_timestamp ? recv_block_timestamp - fire_timestamp : 0;
            } else {
                return confirm_block_timestamp > fire_timestamp ? confirm_block_timestamp - fire_timestamp : 0;       
            }
        }

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
        int no_firestamptx_num{0};

        uint32_t tx_v1_num{0};
        uint64_t tx_v1_total_size{0};
        uint32_t tx_v2_num{0};
        uint64_t tx_v2_total_size{0};
    };

    struct xdbtool_all_table_info_t {
        std::map<std::string, tx_ext_sum_t> unconfirmed_tx_map;
        uint64_t max_confirm_time_from_send{0};
        uint64_t max_confirm_time_from_fire{0};
        uint64_t confirmed_tx_count{0};
        uint64_t total_confirm_time_from_send{0};
        uint64_t total_confirm_time_from_fire{0};
        uint64_t total_tx_count{0};
        std::mutex m_lock;
        bool all_table_set_txinfo(const tx_ext_t & tx_ext, base::enum_transaction_subtype subtype, tx_ext_sum_t & tx_ext_sum);
        void set_table_txdelay_time(const tx_ext_sum_t & tx_ext_sum);
    };

    struct xdbtool_dbsize_key_type_info_t {
        size_t key_count{0};
        size_t key_value_size{0};
    };
    struct tx_check_info_t {
        uint64_t m_test_timestamp{0};
//        uint64_t m_tx_timestamp{0};
        tx_check_info_t(uint64_t test_timestamp) : m_test_timestamp(test_timestamp) {}
        tx_check_info_t() {}
    };
    struct tx_check_result_info_t {
        uint64_t m_test_timestamp{0};
        uint64_t m_tx_send_timestamp{0};
        uint64_t m_tx_recv_timestamp{0};
        uint64_t m_tx_timestamp{0};
    };

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
                || type == base::enum_xdbkey_type_state_object
                || type == base::enum_xdbkey_type_block_out_offdata) {
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

    void query_account_data(std::string const & account,  const uint64_t h_end, std::string & result, const xdb_check_data_func_face_t & func);
    void query_account_data(std::string const & account, json & result_json, const xdb_check_data_func_face_t & func);
    void query_table_mpt(std::string const & account, json & result_json);
    static bool table_mpt_read_callback(const std::string& key, const std::string& value,void *cookie);
    void query_table_latest_fullblock(std::string const & account, json & j);
    void query_tx_info_internal(std::string const & account, const uint32_t start_timestamp, const uint32_t end_timestamp);
    void query_block_info(std::string const & account, const uint64_t h, Json::Value & root);
    void query_block_state_basic(std::string const & account, const uint64_t h, json & result);
    void query_block_basic(std::string const & account, const uint64_t h, json & result);
    void query_state_basic(std::string const & account, const uint64_t h, json & result);
    void query_meta(std::string const & account, json & result);
    void query_table_unit_state(std::string const & table, json & result);
    void query_property(std::string const & account, std::string const & prop_name, const uint64_t height, json & j);
    void query_balance(std::string const & table, json & j_unit, json & j_table);
    void query_checkpoint_internal(std::string const & table, std::set<std::string> const & genesis_only, const uint64_t clock, json & j_data);
    void query_archive_db_internal(common::xtable_address_t const & table_address, uint64_t check_height, std::ofstream & file, std::shared_ptr<xdb_archive_check_info_t> archive_info);

    json set_confirmed_txinfo_to_json(const tx_ext_sum_t & tx_ext_sum);
    json set_unconfirmed_txinfo_to_json(const tx_ext_sum_t & tx_ext_sum);
    // void set_table_txdelay_time(xdbtool_table_info_t & table_info, const tx_ext_t & send_txinfo, const tx_ext_t & confirm_txinfo);

    bool all_table_set_txinfo(const tx_ext_t & tx_ext, base::enum_transaction_subtype subtype, tx_ext_sum_t & tx_ext_sum);
    void get_txinfo_from_txaction(const data::xlightunit_action_t & txaction, const data::xblock_t * block, const data::xtransaction_ptr_t & tx_ptr, std::vector<tx_ext_t> & batch_tx_exts);
    void print_all_table_txinfo_to_file();

    std::set<std::string> get_special_genesis_accounts();
    std::set<std::string> get_db_unit_accounts_v2();
    void load_db_unit_accounts_info();
    void generate_account_info_file(std::string const & account, const uint64_t height);
    void generate_json_file(std::string const & filename, json const & j);
    void generate_common_file(std::string const & filename, std::string const & data);
    void query_table_performance(std::string const & account);

    std::unique_ptr<xbase_timer_driver_t> m_timer_driver;
    xobject_ptr_t<mbus::xmessage_bus_face_t> m_bus;
    xobject_ptr_t<store::xstore_face_t> m_store;
    xobject_ptr_t<base::xvblockstore_t> m_blockstore;
    xobject_ptr_t<base::xvtxstore_t> m_txstore;
    xobject_ptr_t<base::xvnodesrv_t> m_nodesvr_ptr;
    std::shared_ptr<rpc::xrpc_handle_face_t> m_getblock;
    std::unique_ptr<genesis::xgenesis_manager_t> m_genesis_manager;

    std::map<std::string, std::map<std::string, base::xaccount_index_t>> m_db_units_info;
    std::string m_outfile_folder;
    std::mutex m_write_lock;
    
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

    // xdbtool_all_table_info_t m_all_table_info[32];
    xdbtool_all_table_info_t m_all_table_info[TOTAL_TABLE_NUM];
    std::map<std::string, uint64_t> m_txs_fire_timestamp;
};

NS_END2
