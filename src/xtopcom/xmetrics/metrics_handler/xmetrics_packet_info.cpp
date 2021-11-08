#include "xmetrics_packet_info.h"

NS_BEG3(top, metrics, handler)

// using val_unit = Variant<std::string, int64_t>;
#define VAL_UNIT_TYPE_STR 1
#define VAL_UNIT_TYPE_INT64 2

void metrics_packet_impl(metrics_pack_unit & unit) {
    json res, cont;
    res["category"] = get_category(unit.name);
    res["tag"] = get_tag(unit.name);
    res["type"] = unit.type;
    for (auto const & p : unit.pack_content) {
        if (p.second.GetType() == VAL_UNIT_TYPE_STR) {
            cont[p.first] = p.second.GetConstRef<std::string>();
        } else if (p.second.GetType() == VAL_UNIT_TYPE_INT64) {
            cont[p.first] = p.second.GetConstRef<int64_t>();
        }
    }
    res["content"] = cont;
    std::stringstream ss;
    ss << res;
   if (g_metrics_log_instance) {
         g_metrics_log_instance->kinfo("[metrics]%s", ss.str().c_str());
    }
    else{
        xkinfo("[metrics]%s", ss.str().c_str());
    }
    
  
}

NS_END3