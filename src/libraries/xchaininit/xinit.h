#pragma once

#include <string>
#include <map>
#include "xbase/xcontext.h"

namespace top {

using namespace base;

xcontext_t::enum_global_object_key const enum_xtop_global_object_base{ xcontext_t::enum_global_object_key::enum_global_max_xbase_object_key };
xcontext_t::enum_global_object_key const enum_xtop_global_object_store{ static_cast<xcontext_t::enum_global_object_key>(enum_xtop_global_object_base + 1) };
xcontext_t::enum_global_object_key const enum_xtop_global_object_sync{  static_cast<xcontext_t::enum_global_object_key>(enum_xtop_global_object_base + 2) };
xcontext_t::enum_global_object_key const enum_xtop_global_object_vhost{ static_cast<xcontext_t::enum_global_object_key>(enum_xtop_global_object_base + 3) };
xcontext_t::enum_global_object_key const enum_xtop_global_object_consensus_service{ static_cast<xcontext_t::enum_global_object_key>(enum_xtop_global_object_base + 4) };
xcontext_t::enum_global_object_key const enum_xtop_global_object_unit_service{ static_cast<xcontext_t::enum_global_object_key>(enum_xtop_global_object_base + 5) };

int topchain_init(const std::string& config_file, const std::string& config_extra);
int topchain_noparams_init(const std::string& pub_key, const std::string& pri_key, const std::string& node_id, const std::string& datadir, const std::string& config_extra);
bool check_miner_info(const std::string &pub_key, const std::string &node_id, std::string& miner_type);
int topchain_start(const std::string& datadir, const std::string& config_file);
}
