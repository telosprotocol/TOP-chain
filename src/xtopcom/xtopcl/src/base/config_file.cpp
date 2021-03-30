#include "config_file.h"
#include <fstream>
#include <iostream>
#include "log.h"


namespace xChainSDK {


    config_file::config_file()
    {

    }

    config_file::~config_file()
    {

    }

    bool config_file::load(const std::string& path) {

        std::ifstream cfg(path, std::ofstream::in);
        if (!cfg.is_open())
            return false;

        std::string json_str;
        std::string str;
        while (!cfg.eof()) {
            cfg >> str;
            json_str += str;
        }
        cfg.close();

        xJson::Reader reader;
        xJson::Value root;
        try {
            if (!reader.parse(json_str, root_)) {
                return false;
            }
        }
        catch (...) {
            std::cout << "config file parse json error. " << std::endl;
            return false;
        }

        return true;
    }

    std::string config_file::get_string(const std::string& section, const std::string& key) {
        if (root_[section][key].isString()) {
            return root_[section][key].asString();
        }
        else {
            // std::cout << "Not find config string section: " << section << " key: " << key << std::endl;
            return std::string("");
        }
    }

    int config_file::get_int(const std::string& section, const std::string& key) {
        if (root_[section][key].isInt()) {
            return root_[section][key].asInt();
        }
        else {
            // std::cout << "Not find config int section: " << section << " key: " << key << std::endl;
            return 0;
        }
    }

}