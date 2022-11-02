#pragma once

#include "xdb_util.h"

NS_BEG2(top, db_export)

class xdbtool_util_t {
public:
    static void         generate_json_file(std::string const & filename, json const & j);
    static void         generate_common_file(std::string const & filename, std::string const & data);
};

NS_END2