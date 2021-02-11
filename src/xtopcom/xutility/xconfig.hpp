#ifndef xconfig_hpp
#define xconfig_hpp

#include <string>
#include <errno.h>
#include <string.h>
#include <json/json.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include "../xnetwork/xtop_node.h"
#include "../xelect/include/xelect_intf.h"

#include <mutex>

namespace top
{
    struct xconfig
    {
    public:
        static xconfig & get_instance()
        {
            static xconfig c;
            return c;
        }
        int32_t load_config_file(const std::string & config_file);
        void load_elect_config(const xJson::Value &root);
        void load_free_node_config();

    public:
        mutable std::mutex m_local_addr_mutex{};
        xtop_node m_local_addr;
        std::string m_rpc_host{"0.0.0.0"};
        uint16_t m_rpc_port;
        std::string m_bootstrap_ip;

        std::string m_log_path;
        int m_log_level;
        std::string m_db_path;

        std::string m_pub_key;
        std::string m_pri_key;
        std::string m_account;
        const std::string m_account_file = "account.top";

        // for testnet
        bool m_free_node; // false if testnet_zone_id is configed
        int32_t m_zone_id; // "testnet_zone_id"
        int32_t m_shard_id; // "testnet_shard_id"
        elect::enum_node_type m_node_type; // "testnet_node_type"
        int m_elect_cycle{60}; // "testnet_elect_cycle", unit: minutes, must be one of 10/20/30/60

        xtop_node
        local_node() const;

        elect::xshard_info
        local_node_shard_info() const noexcept;

        void
        update_shard_info(std::uint32_t const zone_id, std::uint32_t const shard_id);

        void
        load_shard_info_from_config();

    private: 
        xconfig();
        int32_t save_data_elem(int fd, std::string & value);
        int32_t load_data_elem(int fd, std::string & value);
        int32_t save_account();
        int32_t load_account();

    private:
        void load_testnet_config(const xJson::Value &root);
        void generate_account(int i); // return shard_i
        void load_local_host();
    };

    int32_t get_shard_id(const std::string &account);
    int32_t get_zone_id(const std::string &account);
}
#endif
