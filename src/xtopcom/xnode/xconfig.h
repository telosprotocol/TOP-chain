#pragma once

#include <string>

#include "json/json.h"



namespace top {
    namespace topio {

    class xconfig {
    public:

        static xconfig & get_instance() {
            static xconfig c;
            return c;
        }
        std::string get_file_content(const std::string& filepath) const;
        int32_t load_config_file(const std::string & config_file, const std::string & config_file2="");
        bool save(const std::string & config_file,
                    std::unordered_map<std::string, std::string>& map);
        std::string get_string(const std::string & item) const;
        bool get_string(const std::string & item, std::string& value) const;
        bool get_json(const std::string & item, xJson::Value& value) const;
        void set_option_param(std::string& destination, const std::string& item);
        void set_option_param(uint32_t& destination, const std::string& item);
        void set_option_param(uint16_t& destination, const std::string& item);
        void set_option_param(int32_t& destination, const std::string& item);
        void set_option_param(
            bool& destination,
            const std::string& item,
            const std::string& sub_item);
        void set_option_param(
            std::string& destination,
            const std::string& item,
            const std::string& sub_item);
        void set_option_param(
            uint32_t& destination,
            const std::string& item,
            const std::string& sub_item);
        void set_option_param(
            uint64_t& destination,
            const std::string& item,
            const std::string& sub_item);
        void set_option_param(
            uint16_t& destination,
            const std::string& item,
            const std::string& sub_item);
        void set_option_param(
            int32_t& destination,
            const std::string& item,
            const std::string& sub_item);
        int get_int(const std::string & item) const;
        int get_int_empty(const std::string & item) const;
        template<typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
        std::pair<T, T> get_pair(const std::string & item) {
            if(m_root[item].empty() || !m_root[item].isArray() || m_root[item].size() != 2) {
                return {0, 0};
            }
            return {m_root[item][0].asInt(), m_root[item][1].asInt()};
        }

        template<typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
        void set_option_pair(std::pair<T, T>& destination, const std::string & item) {
            if(!m_root[item].empty() && m_root[item].isArray() && m_root[item].size() == 2) {
                destination = {m_root[item][0].asInt(), m_root[item][1].asInt()};
            }
        }

        bool get_bool(const std::string & item) const;
        void fetch_all(std::unordered_map<std::string, std::string>& map);

    private:
        void extract(xJson::Value& arr, std::unordered_map<std::string, std::string>& map);
        void merge_config(xJson::Value& root, const xJson::Value& root_extra);

    private:
        xJson::Value m_root;
    };


    using xtopio_config_t = xconfig;

    }
}
