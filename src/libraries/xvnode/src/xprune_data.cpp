// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvnode/xcomponents/xprune_data/xprune_data.h"

#include <inttypes.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <json/value.h>
#include <json/reader.h>
#include <json/writer.h>
#include "xbase/xutl.h"
#include "xvledger/xvledger.h"

NS_BEG4(top, vnode, components, prune_data)


bool  xprune_data::update_prune_config_file(std::string& prune_enable)
{
    top::base::xstring_utl::tolower_string(prune_enable);
    std::string extra_config = base::xvchain_t::instance().get_data_dir_path() + "/.extra_conf.json";
    Json::Value key_info_js;
    std::ifstream keyfile(extra_config, std::ios::in);
    if (keyfile) {
        std::stringstream buffer;
        buffer << keyfile.rdbuf();
        keyfile.close();
        std::string key_info = buffer.str();
        Json::Reader reader;
        // ignore any error when parse
        reader.parse(key_info, key_info_js);
    }
    
    key_info_js["auto_prune_data"] = prune_enable;
    
    // dump new json to file
    Json::StyledWriter new_sw;
    std::ofstream os;
    os.open(extra_config);
    if (!os.is_open()) {
        return false;
    }
    os << new_sw.write(key_info_js);
    os.close();
    return true;
}


NS_END4
