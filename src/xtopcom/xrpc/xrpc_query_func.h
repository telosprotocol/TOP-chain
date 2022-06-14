#pragma once

#include <string>
#include "json/json.h"
#include "xbase/xobject.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection_association_result_store_codec.hpp"
#include "xdata/xelection/xelection_association_result_store.h"
#include "xdata/xelection/xelection_cluster_result.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xelection/xstandby_result_store.h"
#include "xgrpcservice/xgrpc_service.h"
#include "xstore/xstore.h"
#include "xsyncbase/xsync_face.h"
#include "xvledger/xvtxstore.h"
#include "xvledger/xvledger.h"
#include "xtxpool_service_v2/xtxpool_service_face.h"
#include "xrpc/xjson_proc.h"

NS_BEG2(top, xrpc)
enum class xtop_enum_full_node_compatible_mode {
    incompatible,
    compatible,
};
using xfull_node_compatible_mode_t = xtop_enum_full_node_compatible_mode;

class xrpc_query_func {
public:
    xrpc_query_func() {}
    void set_store(observer_ptr<store::xstore_face_t> store) { m_store = store;}
    bool is_prop_name_already_set_property(const std::string & prop_name);
    bool is_prop_name_not_set_property(const std::string & prop_name);
    bool query_special_property(xJson::Value & jph, const std::string & owner, const std::string & prop_name, data::xaccount_ptr_t unitstate, bool compatible_mode);
    void query_account_property_base(xJson::Value & jph, const std::string & owner, const std::string & prop_name, data::xaccount_ptr_t unitstate, bool compatible_mode);
    void query_account_property(xJson::Value & jph, const std::string & owner, const std::string & prop_name, xfull_node_compatible_mode_t compatible_mode);
    void query_account_property(xJson::Value & jph, const std::string & owner, const std::string & prop_name, const uint64_t height, xfull_node_compatible_mode_t compatible_mode);
private:
    observer_ptr<store::xstore_face_t> m_store;
};
NS_END2