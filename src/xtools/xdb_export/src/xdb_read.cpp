#include "../xdb_read.h"
#include "../xdbtool_util.h"
#include "xdbstore/xstore.h"
#include "json/json.h"
#include "xdata/xblockextract.h"
#include "xdata/xblocktool.h"
#include "xbasic/xhex.h"
#include "xdata/xnative_contract_address.h"
#include "xsync/xsync_store_shadow.h"

NS_BEG2(top, db_export)

xdb_read_tools_t::xdb_read_tools_t(std::string const & db_path) {
    int dst_db_kind = 0;
    std::vector<db::xdb_path_t> db_data_paths {};
    base::xvchain_t::instance().get_db_config_custom(db_data_paths, dst_db_kind);
    dst_db_kind |= top::db::xdb_kind_readonly;
    std::shared_ptr<db::xdb_face_t> db = top::db::xdb_factory_t::create(dst_db_kind, db_path, db_data_paths);
    m_store = store::xstore_factory::create_store_with_static_kvdb(db);
    m_xvdb_ptr = m_store.get();
    m_xvblockdb_ptr = new store::xvblockdb_t(m_xvdb_ptr);
}

xdb_read_tools_t::~xdb_read_tools_t() {
    if (m_xvdb_ptr != nullptr) {
        m_xvdb_ptr->close();
    }

    if (m_xvblockdb_ptr != nullptr) {
        m_xvblockdb_ptr->release_ref();
    }
}

bool xdb_read_tools_t::is_match_function_name(std::string const & func_name) {
    static std::vector<std::string> names = {
        "db_read_block",
        "db_read_txindex",
        "db_read_meta",
        "db_data_parse",
        "db_read_all_table_height_lists",
        "db_read_span_account_height",
        "db_read_span_account"
    };

    for (auto & v : names) {
        if (v == func_name) {
            return true;
        }
    }
    return false;
}

bool xdb_read_tools_t::process_function(std::string const & func_name, int argc, char ** argv) {
    if (func_name == "db_read_meta") {
        if (argc != 4) return false;
        std::string address = argv[3];
        db_read_meta(address);
    } else if (func_name == "db_data_parse") {
        if (argc != 3) return false;
        db_data_parse();
    } else if (func_name == "db_read_block") {
        if (argc != 5) return false;
        db_read_block(argv[3], std::stoi(argv[4]));
    } else if (func_name == "db_read_txindex") {
        if (argc != 4) return false;
        db_read_txindex(argv[3]);
    } else if (func_name == "db_read_all_table_height_lists") {
        if (argc != 5) return false;
        db_read_all_table_height_lists(argv[3], std::stoi(argv[4]));
    } else if (func_name == "db_read_span_account_height") {
        if (argc != 4) return false;
        db_read_span_account_height(argv[3]);
    } else if (func_name == "db_read_span_account") {
        if (argc != 5) return false;
        db_read_span_account(argv[3], std::stoi(argv[4]));
    } else {
        xassert(false);
        return false;
    }
    return true;
}

void xdb_read_tools_t::db_read_span_account_height(std::string const & account) {

    uint64_t height{0};
    const std::string key_path = base::xvdbkey_t::create_account_span_genesis_height_key(account);
    std::string account_height_bin = m_xvdb_ptr->get_value(key_path);

    if (!account_height_bin.empty()) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)account_height_bin.c_str(), account_height_bin.size());
        stream >> height;
    }
    std::cout << "db_read_span_account_height account: %s " << account << " height:%lu " << height << std::endl;
}

void xdb_read_tools_t::db_read_span_account(std::string const& account, const uint64_t height)
{
    top::sync::xcommon_span_t span(height);
    const std::string key_path = base::xvdbkey_t::create_account_span_key(account, span.index());
    std::string span_bin = m_xvdb_ptr->get_value(key_path);

    std::cout << "db_read_span_account account: %s " << account << " height:%lu " << height;
    if (!span_bin.empty()) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)span_bin.c_str(), span_bin.size());
        span.serialize_from(stream);
        span.connect();

        if (span.height_exist(height)) {
            std::cout << " exist." << std::endl;
        } else {
            std::cout << " not exist." << std::endl;
        }
    } else {
        std::cout << " span not exist" << std::endl;
    }
}

void xdb_read_tools_t::db_read_block(std::string const & address, const uint64_t height) {
    std::cout << "db_read_block start: " << std::endl;
    if (m_xvblockdb_ptr != nullptr) {
        std::vector<base::xvbindex_t*> vector_index = m_xvblockdb_ptr->load_index_from_db(address, height);
        for (auto it = vector_index.begin(); it != vector_index.end(); ++it) {
            base::xvbindex_t* index = (base::xvbindex_t*)*it;
            base::xvblock_t*  block = index->get_this_block();
            if (block == nullptr) {
                std::cout << "fail load block object.index=" << index->dump() << std::endl;
                continue;
            }

            std::cout << index->dump() << ", block_hash=" << base::xstring_utl::to_hex(index->get_block_hash()) << \
            ",last_block_hash=" << base::xstring_utl::to_hex(index->get_last_block_hash()) <<  
            ",clock=" << index->get_this_block()->get_clock() <<
            ",class=" <<  index->get_this_block()->get_block_class() << 
            ",ver=" << index->get_this_block()->get_block_version() << std::endl;

            if (index->get_this_block()->get_block_class() != base::enum_xvblock_class_nil) {
                if (false == m_xvblockdb_ptr->load_block_input(index)) {
                    std::cout << "fail load block input" << std::endl;
                    continue;
                }
                if (false == m_xvblockdb_ptr->load_block_output(index)) {
                    std::cout << "fail load block output" << std::endl;
                    continue;
                }
                if (false == m_xvblockdb_ptr->load_block_output_offdata(index, index->get_this_block())) {
                    std::cout << "fail load block output_offdata" << std::endl;
                    continue;
                }

                std::vector<xobject_ptr_t<base::xvtxindex_t>> sub_txs;
                if (false == index->get_this_block()->extract_sub_txs(sub_txs)) {
                    std::cout << "fail extract sub txs" << std::endl;
                    continue;
                }
                std::cout << "sub txs count " << sub_txs.size() << std::endl;
                for (auto & v : sub_txs) {
                    base::enum_txindex_type txindex_type = base::xvtxkey_t::transaction_subtype_to_txindex_type(v->get_tx_phase_type());
                    const std::string tx_key = base::xvdbkey_t::create_tx_index_key(v->get_tx_hash(), txindex_type);
                    std::string tx_bin = m_xvdb_ptr->get_value(tx_key);
                    if (tx_bin.empty()) {
                        std::cout << "fail load txindex " << base::xvtxkey_t::transaction_hash_subtype_to_string(v->get_tx_hash(), v->get_tx_phase_type()) << std::endl;
                        continue;
                    } else {
                        std::cout << "load txindex " << base::xvtxkey_t::transaction_hash_subtype_to_string(v->get_tx_hash(), v->get_tx_phase_type()) << std::endl;
                    }
                }
            }

            (*it)->release_ref();   //release ptr that reference added by read_index_from_db
        }
    }

}

void xdb_read_tools_t::db_read_txindex(std::string const & hex_txhash, base::enum_txindex_type txindex_type) {
    std::string txhash = top::to_string(top::from_hex(hex_txhash));
    const std::string tx_key = base::xvdbkey_t::create_tx_index_key(txhash, txindex_type);
    std::string tx_idx_bin = m_xvdb_ptr->get_value(tx_key);
    if (tx_idx_bin.empty()) {
        std::cout << "fail load txindex " << hex_txhash << std::endl;
        return;
    }

    base::xauto_ptr<base::xvtxindex_t> txindex(new base::xvtxindex_t());
    if (txindex->serialize_from_string(tx_idx_bin) <= 0) {
        std::cout << "fail unserialize from send txindex " << hex_txhash << std::endl;
        return;
    }
    txindex->set_tx_hash(txhash);
    std::cout << "load txindex " << txindex->dump() << std::endl;

    std::vector<base::xvbindex_t*> vector_index = m_xvblockdb_ptr->load_index_from_db(txindex->get_block_addr(), txindex->get_block_height());
    for (auto & bindex : vector_index) {
        if (bindex->check_block_flag(base::enum_xvblock_flag_committed)) {
            if (bindex->get_block_hash() != txindex->get_block_hash()) {                
                std::cout << "wrong blockhash txindex " << txindex->dump() << std::endl;                 
            }
        }

        bindex->release_ref();   //release ptr that reference added by read_index_from_db
    }    
}

void xdb_read_tools_t::db_read_txindex(std::string const & hex_txhash) {
    db_read_txindex(hex_txhash, base::enum_txindex_type_send);
    db_read_txindex(hex_txhash, base::enum_txindex_type_receive);
    db_read_txindex(hex_txhash, base::enum_txindex_type_confirm);
}

base::xauto_ptr<base::xvactmeta_t> xdb_read_tools_t::db_read_meta(std::string const & address) {
    base::xvaccount_t _vaddr{address};
    std::string new_meta_key = base::xvdbkey_t::create_account_meta_key(_vaddr);
    std::string value = m_xvdb_ptr->get_value(new_meta_key);
    base::xauto_ptr<base::xvactmeta_t> _meta = new base::xvactmeta_t(_vaddr);  // create empty meta default
    if (!value.empty()) {
        if (_meta->serialize_from_string(value) <= 0) {
            std::cerr << "address=" << address << " meta serialize_from_string fail !!!" << std::endl;
        } else {
            std::cout << "address=" << address << " meta=" << _meta->clone_block_meta().ddump() << std::endl;
            return _meta;
        }
    } else {
        std::cerr << "address=" << address << " meta value empty !!!" << std::endl;
    }
    return nullptr;
}

void xdb_read_tools_t::db_read_all_table_height_lists(std::string const & mode, uint64_t redundancy) {
    auto tables = data::xblocktool_t::make_all_table_addresses();
    tables.push_back(sys_drand_addr);

    std::string table_height_lists;
    for (auto & table : tables) {
        base::xauto_ptr<base::xvactmeta_t> _meta = db_read_meta(table);
        uint64_t height = 0;
        if (nullptr != _meta) {            
            if (mode == "commit_mode" || mode.empty()) {
                uint64_t commit_height = _meta->clone_block_meta()._highest_commit_block_height;
                height = commit_height > redundancy ? commit_height - redundancy : commit_height;
            } else {
                xassert(false);
            }
        }
        std::string table_height = table + ":" + std::to_string(height) + ",";
        table_height_lists += table_height;
    }

    std::cout << std::endl;
    std::cout << "table_height_lists:" << table_height_lists << std::endl;
}

void xdb_read_data_parse_t::add_key_value(std::string const& key, std::string const& value) {
    base::enum_xdbkey_type db_key_type = base::xvdbkey_t::get_dbkey_type(key);
    std::string type_name = base::xvdbkey_t::get_dbkey_type_name(db_key_type);

    add_type_info(type_name, key, value);

    std::string owner;
    switch (db_key_type) {
        case base::enum_xdbkey_type_block_object:
        case base::enum_xdbkey_type_block_input_resource:
        case base::enum_xdbkey_type_block_output_resource:
        case base::enum_xdbkey_type_block_index:
        case base::enum_xdbkey_type_state_object:
        case base::enum_xdbkey_type_block_out_offdata:
        case base::enum_xdbkey_type_unit_proof:
        case base::enum_xdbkey_type_unitstate_v2: {    
            base::xvaccount_t address(base::xvdbkey_t::get_account_address_from_key(key));
            if (address.is_table_address()) {
                owner = "Table";
            } else if (address.is_drand_address()) {
                owner = "Drand";
            } else if (address.is_timer_address()) {
                owner = "Timer";
            } else if (address.is_contract_address()) {
                owner = "Contract";
            } else {
                owner = "User";
            }
            add_detail_type_info(owner, type_name, key, value);

            // add every table detail info
            if (address.is_table_address()) {
                owner = address.get_account();
                add_detail_type_info(owner, type_name, key, value);
            }           
        }
        default:
            break;
    }
}

void xdb_read_data_parse_t::add_type_info(std::string const& type, std::string const& key, std::string const& value) {
    m_total_info.key_count += 1;
    m_total_info.key_total_size += key.size();
    m_total_info.value_total_size += value.size();

    m_type_info[type].key_count += 1;
    m_type_info[type].key_total_size += key.size();
    m_type_info[type].value_total_size += value.size();
}

void xdb_read_data_parse_t::add_detail_type_info(std::string const& owner, std::string const& detail_type, std::string const& key, std::string const& value) {
    m_detail_type_info[owner][detail_type].key_count += 1;
    m_detail_type_info[owner][detail_type].key_total_size += key.size();
    m_detail_type_info[owner][detail_type].value_total_size += value.size();
}

uint64_t xdb_read_data_parse_t::calc_percent(xdb_key_basic_info_t const& value) const {
    return (value.key_total_size+value.value_total_size)*100/(m_total_info.key_total_size+m_total_info.value_total_size);
}

void xdb_read_data_parse_t::to_json(json & root) const {
    {
        json info;
        info["count"] = m_total_info.key_count;
        info["size"] = m_total_info.key_total_size;
        info["valuesize"] = m_total_info.value_total_size;
        root["total"] = info;
    }
    {
        json type_info;
        for (auto & v : m_type_info) {
            json info;
            info["count"] = v.second.key_count;
            info["keysize"] = v.second.key_total_size;
            info["valuesize"] = v.second.value_total_size;
            info["percent"] = calc_percent(v.second);
            type_info[v.first] = info;
        }
        root["type"] = type_info;
    }
    {
        json detail_type_info;
        for (auto & p : m_detail_type_info) {
            json type_info;
            for (auto & v : p.second) {
                json info;
                info["count"] = v.second.key_count;
                info["keysize"] = v.second.key_total_size;
                info["valuesize"] = v.second.value_total_size;
                info["percent"] = calc_percent(v.second);
                type_info[v.first] = info;
            }
            detail_type_info[p.first] = type_info;
        }   
        root["detail_type"] = detail_type_info;
    }
}

bool xdb_read_tools_t::db_scan_key_callback(const std::string& key, const std::string& value)
{
    m_db_data_parse.add_key_value(key, value);
    if (m_db_data_parse.get_total_key_count() % 1000000 == 0) {
        std::cout << "db scan key total = " << m_db_data_parse.get_total_key_count() << std::endl;
    }
    return true;
}

bool  xdb_read_tools_t::db_scan_key_callback(const std::string& key, const std::string& value,void *cookie)
{
    bool ret = false;
    if (NULL != cookie) {
        xdb_read_tools_t *p_xdb_read_tools_t = (xdb_read_tools_t*)cookie;
        ret = p_xdb_read_tools_t->db_scan_key_callback(key, value);
    }
    return ret;
}

void xdb_read_tools_t::db_data_parse() {
    top::store::xstore * pxstore = dynamic_cast< top::store::xstore*>(m_xvdb_ptr);

    pxstore->read_range_callback("",  db_scan_key_callback, this);

    // output to json future
    json root;
    m_db_data_parse.to_json(root);
    xdbtool_util_t::generate_json_file("./db_data_parse.json" ,root);
}

NS_END2