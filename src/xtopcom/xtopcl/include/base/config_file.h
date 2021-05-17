#pragma once

#include <json/value.h>
#include <string>
#include <map>

#include "singleton.h"
#include "json/json.h"

namespace xChainSDK {

    constexpr char net_setting[] = "net_setting";
    constexpr char http_host[] = "http_host";
    constexpr char ws_host[] = "ws_host";
    constexpr char http_mode[] = "http_mode";


    class config_file : public Singleton <config_file> {

    public:
        config_file();
        ~config_file();

        bool load(const std::string& path);

        std::string get_string(const std::string& section, const std::string& key);
        int get_int(const std::string& section, const std::string& key);

    private:
        xJson::Value root_;
    };
}
