#pragma once

#include "xdb_util.h"

#include "xtxstore/xtxstore_face.h"
#include "xdb/xdb.h"
#include "xdb/xdb_factory.h"
#include "xdb/xdb_face.h"
#include "xdbstore/xstore_face.h"
#include "xblockstore/src/xvblockdb.h"

NS_BEG2(top, db_export)

struct xdb_key_basic_info_t {
    size_t key_count{0};
    size_t key_total_size{0};
    size_t value_total_size{0};
};

class xdb_read_data_parse_t {
public:
    void            add_key_value(std::string const& key, std::string const& value);
    size_t          get_total_key_count() const {return m_total_info.key_count;}
    void            to_json(json & root) const;
private:
    void            add_type_info(std::string const& type, std::string const& key, std::string const& value);
    void            add_detail_type_info(std::string const& owner, std::string const& detail_type, std::string const& key, std::string const& value);
    uint64_t        calc_percent(xdb_key_basic_info_t const& value) const;

private:
    xdb_key_basic_info_t    m_total_info; 
    std::map<std::string, xdb_key_basic_info_t>   m_type_info;
    std::map<std::string, std::map<std::string, xdb_key_basic_info_t>>  m_detail_type_info;
};

class xdb_read_tools_t {
public:
    xdb_read_tools_t(std::string const & db_path);
    ~xdb_read_tools_t();

    static bool is_match_function_name(std::string const & func_name);
    bool process_function(std::string const & func_name, int argc, char ** argv);

protected:
    void db_read_unit(std::string const & address, const uint64_t height);
    void db_read_block(std::string const & address, const uint64_t height);
    base::xauto_ptr<base::xvactmeta_t> db_read_meta(std::string const & address);    
    void db_data_parse();
    void db_read_txindex(std::string const & hex_txhash);
    void db_read_txindex(std::string const & hex_txhash, base::enum_txindex_type txindex_type);
    void db_read_all_table_height_lists(std::string const & mode, uint64_t redundancy);
    void db_read_span_account_height(std::string const & address);
    void db_read_span_account(std::string const& address, const uint64_t height);

private:
    bool db_scan_key_callback(const std::string& key, const std::string& value);
    static bool db_scan_key_callback(const std::string& key, const std::string& value,void *cookie);
    
private:
    xobject_ptr_t<store::xstore_face_t> m_store { nullptr };
    base::xvdbstore_t*  m_xvdb_ptr { nullptr };
    store::xvblockdb_t* m_xvblockdb_ptr { nullptr };

    xdb_read_data_parse_t   m_db_data_parse;
};

NS_END2
