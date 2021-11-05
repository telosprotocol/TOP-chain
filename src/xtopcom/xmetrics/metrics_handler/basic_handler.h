// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xmetrics/xmetrics_event.h"
#include "xmetrics/xmetrics_unit.h"
#include "xmetrics/xmetrics_config.h"
#include "nlohmann/json.hpp"
#include "nlohmann/fifo_map.hpp"
#include "xbase/xlog.h"

#include <iomanip>
#include <iostream>
#include <sstream>

NS_BEG3(top, metrics, handler)

// A workaround to give to use fifo_map as map, we are just ignoring the 'less' compare
template<class K, class V, class dummy_compare, class A>
using my_workaround_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;

using unordered_json = nlohmann::basic_json<my_workaround_fifo_map>;
using json = unordered_json;

inline std::string get_category(std::string const & str) {
    return str.substr(0, str.find("_"));
}
inline std::string get_tag(std::string const & str) {
    return str.substr(str.find("_") + 1);
}

using namespace top::base;
void metrics_log_init(std::string log_path);
void metrics_log_close();

//extern top::base::xlogger_t *g_metrics_log_instance;

class xtop_basic_handler {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_basic_handler);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_basic_handler);
    XDECLARE_DEFAULTED_VIRTULA_DESTRUCTOR(xtop_basic_handler);

    virtual int calc_dump_width(std::size_t sz) {
        return (int)(sz / 10 + 1) * 10;
    }

    virtual void dump(std::string const & str, bool is_updated);
 

    virtual metrics_variant_ptr init_new_metrics(event_message const & msg) = 0;
    virtual void dump_metrics_info(metrics_variant_ptr const & metrics_ptr) = 0;
    virtual void process_message_event(metrics_variant_ptr & metrics_ptr, event_message const & msg) = 0;
};

using xbasic_handler_t = xtop_basic_handler;

NS_END3