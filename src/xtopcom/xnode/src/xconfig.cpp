#include <string>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>

#include "xnode/xconfig.h"
#include "json/value.h"
#include "xpbase/base/top_log.h"

namespace top{
    namespace topio {

std::string xconfig::get_file_content(const std::string& filepath) const {
    std::ifstream in(filepath);
    if (!in.is_open()) {
        std::cerr << "can't open file: " << filepath << std::endl;

        assert(false);
        return "";
    }
    std::ostringstream tmp;
    tmp << in.rdbuf();
    in.close();
    return tmp.str();
}

int32_t xconfig::load_config_file(const std::string & config_file, const std::string & config_extra) {
    std::string content = get_file_content(config_file);
    xJson::Reader reader;
    bool ret = reader.parse(content, m_root);
    if (!ret) {
        std::cout << "parse config file " << config_file << " failed" << std::endl;
        return -1;
    }

    if (!config_extra.empty()) {
        xJson::Reader reader_extra;
        xJson::Value root_extra;
        std::string content_extra = get_file_content(config_extra);
        bool ret = reader_extra.parse(content_extra, root_extra);
        if (!ret) {
            std::cout << "parse config file " << config_extra << " failed" << std::endl;
            return -1;
        }
        merge_config(m_root, root_extra);
    }
    return 0;
}

void xconfig::merge_config(xJson::Value& root, const xJson::Value& root_extra) {
    if (!root_extra.isObject()) {
        return;
    }

    for (auto& name : root_extra.getMemberNames()) {
        root[name] = root_extra[name];
        TOP_FATAL("merge extra[%s]", name.c_str());
    }
}

bool xconfig::save(const std::string & config_file,
        std::unordered_map<std::string, std::string>& map) {
    xJson::Value root;
    for(auto& enty : map) {
        root[enty.first.c_str()] = enty.second.c_str();
    }

    xJson::FastWriter writer;
    std::string str = writer.write(root);
    std::ofstream ofs;
    ofs.open(config_file.c_str());
    if(ofs.is_open()) {
        ofs << str;
        ofs.close();
        return true;
    }
    return false;
}

void xconfig::fetch_all(std::unordered_map<std::string, std::string>& map) {
    extract(m_root, map);
}

void xconfig::extract(xJson::Value& arr, std::unordered_map<std::string, std::string>& map) {
    auto mem = arr.getMemberNames();
    for(auto it = mem.begin(); it != mem.end(); ++it) {
        if(arr[*it].isArray()) {
            // extract(arr[*it], map);
            continue;
        } else {
            map[*it] = arr[*it].asString();
        }
    }
}

std::string xconfig::get_string(const std::string & item) const {
    if(m_root[item].empty())
    {
        std::cout << "config item " << item << " is empty" << std::endl;
    }
    assert(false == m_root[item].empty());
    // std::cout << "config " << item << " " << m_root[item].asString() << std::endl;
    return m_root[item].asString();
}

bool xconfig::get_string(const std::string & item, std::string& value) const {
    if (!m_root[item].empty() && m_root[item].isString()) {
        value = m_root[item].asString();
        return true;
    }
    return false;
}

bool xconfig::get_json(const std::string& item, xJson::Value& value) const {
    if (m_root.isMember(item)) {
        value = m_root[item];
        return true;
    }

    return false;
}

void xconfig::set_option_param(std::string& destination, const std::string& item)
{
    if (!m_root[item].empty() && m_root[item].isString()) {
        destination = m_root[item].asString();
    }
}

void xconfig::set_option_param(uint32_t& destination, const std::string& item)
{
    if (!m_root[item].empty() && m_root[item].isUInt()) {
        destination = m_root[item].asUInt();
    }
}

void xconfig::set_option_param(uint16_t& destination, const std::string& item)
{
    if (!m_root[item].empty() && m_root[item].isUInt()) {
        destination = m_root[item].asUInt();
    }
}

void xconfig::set_option_param(int32_t& destination, const std::string& item)
{
    if (!m_root[item].empty() && m_root[item].isInt()) {
        destination = m_root[item].asInt();
    }
}

void xconfig::set_option_param(
        bool& destination,
        const std::string& item,
        const std::string& sub_item)
{
    if (!m_root[item].empty() && !m_root[item][sub_item].empty() && m_root[item][sub_item].isBool()) {
        destination = m_root[item][sub_item].asBool();
    }
}

void xconfig::set_option_param(
        std::string& destination,
        const std::string& item,
        const std::string& sub_item)
{
    if (!m_root[item].empty() && !m_root[item][sub_item].empty() && m_root[item][sub_item].isString()) {
        destination = m_root[item][sub_item].asString();
    }
}

void xconfig::set_option_param(
    uint32_t& destination,
    const std::string& item,
    const std::string& sub_item)
{
    if (!m_root[item].empty() && !m_root[item][sub_item].empty() && m_root[item][sub_item].isUInt()) {
        destination = m_root[item][sub_item].asUInt();
    }
}

void xconfig::set_option_param(
    uint64_t& destination,
    const std::string& item,
    const std::string& sub_item)
{
    if (!m_root[item].empty() && !m_root[item][sub_item].empty() && m_root[item][sub_item].isUInt64()) {
        destination = m_root[item][sub_item].asUInt64();
    }
}

void xconfig::set_option_param(
    uint16_t& destination,
    const std::string& item,
    const std::string& sub_item)
{
    if (!m_root[item].empty() && !m_root[item][sub_item].empty() && m_root[item][sub_item].isUInt()) {
        destination = m_root[item][sub_item].asUInt();
    }
}

void xconfig::set_option_param(
    int32_t& destination,
    const std::string& item,
    const std::string& sub_item)
{
    if (!m_root[item].empty() && !m_root[item][sub_item].empty() && m_root[item][sub_item].isInt()) {
        destination = m_root[item][sub_item].asInt();
    }
}

int xconfig::get_int(const std::string & item) const {
    if(m_root[item].empty())
    {
        std::cout << "config item " << item << " is empty" << std::endl;
    }
    assert(false == m_root[item].empty());
    std::cout << "config " << item << " " << m_root[item].asInt() << std::endl;
    return m_root[item].asInt();
}

int xconfig::get_int_empty(const std::string & item) const {
    if(m_root[item].empty()) {
        std::cout << "config item " << item << " empty, use default" << std::endl;
        return 0;
    }
    std::cout << "config " << item << " " << m_root[item].asInt() << std::endl;
    return m_root[item].asInt();
}

bool xconfig::get_bool(const std::string & item) const {
    if(m_root[item].empty())
    {
        std::cout << "config item " << item << " is empty" << std::endl;
    }
    assert(false == m_root[item].empty());
    std::cout << "config " << item << " " << m_root[item].asBool() << std::endl;
    return m_root[item].asBool();
}
    }
}
