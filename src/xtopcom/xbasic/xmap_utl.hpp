#pragma once

#include "xbasic/xns_macro.h"

NS_BEG2(top, basic)
        
class xmap_utl_t {
    public:
        /**
         * Extract map from string. for example:
         * name:jack;age:18; -> 
         * {
         *      name:jack,
         *      age:18
         * }
         * 
         * @param str IN string which contains map data
         * @param first_sp IN first separator
         * @param second_sp IN second separator
         * @param map OUT map result
         */
        static void extract_map(std::string& str, const char first_sp, const char second_sp, 
                std::map<std::string, std::string>& map) {
            std::vector<std::string> params;
            std::string key, value;
            base::xstring_utl::split_string(str, first_sp, params);

            for (auto& param : params) {
                if (!extract_key_value(param, key, value, second_sp) ||
                        key.empty() || value.empty()) {
                    continue;
                }
                map[key] = value;
            }
        }

        static bool extract_key_value(const std::string& param, std::string& key, std::string& value, const char sp) {
            size_t index = param.find(sp);
            if(index == std::string::npos) {
                return false;
            }
            if(index == param.size() - 1) {
                key = param.substr(0, param.size()-1);
                value = "";
            } else if(index == 0) {
                key = "";
                value = param.substr(1, param.size()-1);
            } else {
                key = param.substr(0, index);
                value = param.substr(index+1, param.size()-index);
            }
            return true;
        }
};
        
NS_END2

