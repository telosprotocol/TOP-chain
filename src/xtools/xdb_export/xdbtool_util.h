#pragma once

#include "xdb_util.h"
#include "xcommon/xaccount_address.h"

NS_BEG2(top, db_export)

class xdbtool_util_t {
public:
    static void         generate_json_file(std::string const & filename, json const & j);
    static void         generate_common_file(std::string const & filename, std::string const & data);
    static std::map<common::xtable_address_t, uint64_t>     parse_table_addr_height_list(std::string const & para);
};

NS_END2