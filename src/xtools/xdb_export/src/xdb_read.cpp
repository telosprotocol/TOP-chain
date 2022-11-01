#include "../xdb_read.h"
#include "../xdbtool_util.h"
#include "xdbstore/xstore.h"
#include "json/json.h"

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

void xdb_read_tools_t::db_read_block(std::string const & address, const uint64_t height) {
    std::cout << "db_read_block start: " << std::endl;
    if (m_xvblockdb_ptr != nullptr) {
        std::vector<base::xvbindex_t*> vector_index = m_xvblockdb_ptr->load_index_from_db(address, height);
        for (auto it = vector_index.begin(); it != vector_index.end(); ++it) {
            base::xvbindex_t* index = (base::xvbindex_t*)*it;
            std::cout << index->dump().c_str() << ", block_hash=" << base::xstring_utl::to_hex(index->get_block_hash()) << \
            ", last_block_hash=" << base::xstring_utl::to_hex(index->get_last_block_hash()) <<  ", block_clock=" << index->get_this_block()->get_clock() << \
            ", block_class=" <<  index->get_block_class() << std::endl;
            (*it)->release_ref();   //release ptr that reference added by read_index_from_db
        }
    }

}

void xdb_read_tools_t::db_read_meta(std::string const & address) {
    std::cout << "db_read_meta start: " << std::endl;
    if (m_xvdb_ptr != nullptr) {
        base::xvaccount_t _vaddr{address};
        std::string new_meta_key = base::xvdbkey_t::create_account_meta_key(_vaddr);
        std::string value = m_xvdb_ptr->get_value(new_meta_key);
        base::xvactmeta_t* _meta = new base::xvactmeta_t(_vaddr);  // create empty meta default
        if (!value.empty()) {
            if (_meta->serialize_from_string(value) <= 0) {
                std::cerr << "meta serialize_from_string fail !!!" << std::endl;
            } else {
                std::cout << "meta serialize_from_string succ,meta=" << _meta->clone_block_meta().ddump() << std::endl;
            }
        } else {
            std::cerr << "meta value empty !!!" << std::endl;
        }    
    }
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