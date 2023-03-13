#include <string>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdlib.h>

#include <json/value.h>
#include "xpbase/base/top_log.h"
#include "xloader/xconfig_offchain_loader.h"

NS_BEG2(top, loader)

xconfig_offchain_loader_t::xconfig_offchain_loader_t(const std::string& config_file, const std::string & config_extra_file)
 : m_config_file(config_file), m_config_extra_file(config_extra_file) {
}

void xconfig_offchain_loader_t::start() {

}

void xconfig_offchain_loader_t::stop() {

}

bool xconfig_offchain_loader_t::save_conf(const std::map<std::string, std::string>& map) {
#if 0
    Json::Value root;
    for(auto& enty : map) {
        root[enty.first.c_str()] = enty.second.c_str();
    }

    Json::FastWriter writer;
    std::string str = writer.write(root);
    std::ofstream ofs;
    ofs.open(out_file.c_str());
    if(ofs.is_open()) {
        ofs << str;
        ofs.close();
        return true;
    }
    return false;
#endif
    return true;
}

bool xconfig_offchain_loader_t::fetch_all(std::map<std::string, std::string>& map) {
    Json::Value json_root;
    if (!m_config_file.empty()) {
        Json::Reader reader;

        auto config_content = get_file_content(m_config_file);
        if (config_content.empty()) {
            std::cout << "config file " << m_config_file << " empty" << std::endl;
            return false;
        }
        bool ret = reader.parse(config_content, json_root);
        if (!ret) {
            std::cout << "parse config file " << m_config_file << " failed" << std::endl;
            return false;
        }
    }

    if (!m_config_extra_file.empty()) {
        Json::Reader reader_extra;
        Json::Value root_extra;
        auto config_extra_content = get_file_content(m_config_extra_file);
        bool ret = reader_extra.parse(config_extra_content, root_extra);
        if (!ret) {
            std::cout << "parse config file " << config_extra_content << " failed" << std::endl;
            return -1;
        }

        for (auto& name : root_extra.getMemberNames()) {
            json_root[name] = root_extra[name];
            TOP_INFO("merge extra[%s]", name.c_str());
        }
    }
    extract("", json_root, map);

    return true;
}

std::string xconfig_offchain_loader_t::get_file_content(const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) {
        std::cerr << "can't open file: " << filepath << std::endl;

        assert(false);
        return "";
    }
    std::ostringstream tmp;
    tmp << in.rdbuf();
    in.close();
    return std::move(tmp.str());
}

void xconfig_offchain_loader_t::extract(const std::string& prefix, const Json::Value& root, std::map<std::string, std::string>& params) {
    const auto members = root.getMemberNames();
    for(const auto& member : members) {
        std::string key = member + "_";
        if (!prefix.empty()) {
            key = prefix + member + "_";
        }
        if(root[member].isArray()) {
            for (unsigned int i = 0; i < root[member].size(); ++i) {
                if (root[member][i].isObject()) {
                    extract(key, root[member][i], params);
                } else {
                    Json::FastWriter writer;
                    params[key] = writer.write(root[member]);
                }
            }
        } else if(root[member].isObject()) {
            extract(key, root[member], params);
        } else {
            if (prefix.empty()) {
                params[member] = root[member].asString();
            } else {
                std::string param = prefix + member;
                params[param] = root[member].asString();
            }
        }
    }
}

NS_END2
