#include "xconfig/xconfig_face.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xconfig_update_parameter_action.h"

NS_BEG2(top, config)

xconfig_update_parameter_action_t::~xconfig_update_parameter_action_t() {}

bool xconfig_update_parameter_action_t::do_update(const std::map<std::string, std::string>& params) {
    xconfig_register_t::get_instance().update_params(params);
    return true;
}

xconfig_incremental_add_update_parameter_action_t::~xconfig_incremental_add_update_parameter_action_t() {}
bool xconfig_incremental_add_update_parameter_action_t::do_update(const std::map<std::string, std::string>& params) {
    // current only support black/whitelist
    std::map<std::string, std::string> target_param = params;
    for (auto const& item: params) {
        std::string result{""};
        auto res = xconfig_register_t::get_instance().get<std::string>(item.first, result);
        if (!res) {
            xwarn("[xconfig_incremental_add_update_parameter_action_t::do_update] %s read fail, use default value", item.first.c_str());
            #if defined XENABLE_ASSERTION_FAILURE
                assert(false);
            #endif
        }

        target_param[item.first] = xtop_config_utility::incremental_add_bwlist(result, item.second);
        xinfo("[xconfig_incremental_add_update_parameter_action_t::do_update] key is %s, value is %s", item.first.c_str(), target_param[item.first].c_str());
    }


    xconfig_register_t::get_instance().update_params(target_param);
    return true;
}

xconfig_incremental_delete_update_parameter_action_t::~xconfig_incremental_delete_update_parameter_action_t() {}
bool xconfig_incremental_delete_update_parameter_action_t::do_update(const std::map<std::string, std::string>& params) {
    // current only support black/whitelist
    std::map<std::string, std::string> target_param = params;
    for (auto const& item: params) {
        std::string result{""};
        auto res = xconfig_register_t::get_instance().get<std::string>(item.first, result);
        if (!res) {
            xwarn("[xconfig_incremental_delete_update_parameter_action_t::do_update] %s read fail, use default value", item.first.c_str());
            #if defined XENABLE_ASSERTION_FAILURE
                assert(false);
            #endif
        }

        target_param[item.first] = xtop_config_utility::incremental_delete_bwlist(result, item.second);
        xinfo("[xconfig_incremental_delete_update_parameter_action_t::do_update] key is %s, value is %s", item.first.c_str(), target_param[item.first].c_str());
    }

    xconfig_register_t::get_instance().update_params(target_param);
    return true;
}

xconfig_add_parameter_action_t::~xconfig_add_parameter_action_t() {}
bool xconfig_add_parameter_action_t::do_update(const std::map<std::string, std::string>& params) {
    xconfig_register_t::get_instance().add_delete_params(params);
    return true;
}

xconfig_delete_parameter_action_t::~xconfig_delete_parameter_action_t() {}
bool xconfig_delete_parameter_action_t::do_update(const std::map<std::string, std::string>& params) {
    xconfig_register_t::get_instance().add_delete_params(params, false);
    return true;
}

std::string xtop_config_utility::incremental_add_bwlist(std::string const& bwlist, std::string const& value) {

    std::string result_bwlist{bwlist+","};
    std::vector<std::string> vec_bwlist;
    base::xstring_utl::split_string(bwlist, ',', vec_bwlist);

    std::vector<std::string> vec_add_member;
    base::xstring_utl::split_string(value, ',', vec_add_member);
    for (auto const& v: vec_add_member) {
        if (std::find(std::begin(vec_bwlist), std::end(vec_bwlist), v) != std::end(vec_bwlist)) continue;
        result_bwlist += v + ",";
    }

    return result_bwlist.substr(0, result_bwlist.size()-1);
}

std::string  xtop_config_utility::incremental_delete_bwlist(std::string const& bwlist, std::string const& value) {

    std::string result_bwlist{""};
    std::vector<std::string> vec_bwlist;
    base::xstring_utl::split_string(bwlist, ',', vec_bwlist);

    std::vector<std::string> vec_delete_member;
    base::xstring_utl::split_string(value, ',', vec_delete_member);
    for (auto const& v: vec_delete_member) {
        auto target_item = std::find(std::begin(vec_bwlist), std::end(vec_bwlist), v);
        if (std::end(vec_bwlist) == target_item) continue;

        vec_bwlist.erase(target_item);
    }

    for (auto const& v: vec_bwlist) {
        result_bwlist += v + ",";
    }

    return result_bwlist.substr(0, result_bwlist.size()-1);
}


NS_END2