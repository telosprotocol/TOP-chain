#include <string>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <json/json.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include "xnet.h"
#include "xckey.h"
#include "xconfig.hpp"
#include <stdlib.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netdb.h>

using namespace top;
using namespace top::utl;

int32_t xconfig::load_config_file(const std::string & config_file)
{
    int fd = open(config_file.c_str(), O_RDONLY);
    if(fd < 0){
        std::cout << "open config file failed, " << strerror(errno) << std::endl;
        return -1;
    }
    char buf[1024] = {0};
    std::string content;
    int ret = 0;
    while(true){
        bzero(buf, 1024);
        ret = read(fd, buf, 1024);
        if(ret < 0){
            std::cout << "read config file failed, " << strerror(errno) << std::endl;
            close(fd);
            return -1;
            break;
        }
        if(ret == 0){
            break;
        }
        content.append(buf, ret);
    }

    xJson::Reader reader;
    xJson::Value root;
    if(reader.parse(content, root)){
        m_rpc_port = root["rpc_port"].asInt();
        m_bootstrap_ip = root["bootstrap_ip"].asString();
        m_local_addr.m_port = root["msg_port"].asInt();

        load_testnet_config(root);

        m_log_path = root["log_path"].asString();
        m_log_level = root["log_level"].asInt();
	    m_db_path = root["db_path"].asString();

    } else {
        std::cout << "parse config file " << config_file << " failed" << std::endl;
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}

void xconfig::load_local_host() {
    if (m_local_addr.m_host.empty())
    {
        do
        {
            struct ifaddrs *ifaddr;
            char host[NI_MAXHOST] = { 0 };

            if (getifaddrs(&ifaddr) == -1)
            {
               break;
            }

            for (auto ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
            {
                if (ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family != AF_INET)
                {
                    continue;
                }

                if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST))
                {
                    continue;
                }

                if (std::strcmp("127.0.0.1", host) == 0)
                {
                    std::memset(host, 0, sizeof(host));
                    continue;
                }

                m_local_addr.m_host = host;
                break;
            }

            freeifaddrs(ifaddr);

        } while (false);
    }
}

void xconfig::load_free_node_config() {
    m_rpc_port = 8051;
    m_local_addr.m_port = 8052;
    m_bootstrap_ip = "104.248.78.80,128.199.70.212";
    m_free_node = true;
    m_zone_id = -1;
    m_shard_id = -1;
    m_node_type = elect::enum_node_type_consensus;
    m_elect_cycle = 30;
    m_log_path = "/tmp/xtop.log";
    m_log_level = 0;
    m_db_path = "/usr/local/topchain/" + DB_PATH;
    ::printf("load free node config\n");

    load_shard_info_from_config();
}

xconfig::xconfig()
{
    load_local_host();
    if (load_account() != 0) {
        generate_account(0);
    } else {
        ::printf("found old account: %s\n", m_account.c_str());
    }
}

int32_t xconfig::save_data_elem(int fd, std::string & value)
{
    int len = value.length();
    int ret = write(fd, (const char *)&len, sizeof(len));
    if(ret != sizeof(len)){
        std::cout << "save account failed" << std::endl;
        return 1;
    }
    ret = write(fd, value.c_str(), value.length());
    if(ret != value.length()){
        std::cout << "save account failed" << std::endl;
        return 1;
    }
    return 0;
}

int32_t xconfig::save_account()
{
    if(access(m_account_file.c_str(), O_RDONLY) == 0){
        // std::cout << "the account file is already exist" << std::endl;
        // return 1;
        ::remove(m_account_file.c_str());
    }

    int fd = open(m_account_file.c_str(), O_RDWR | O_CREAT, 00644);
    if(fd < 0){
        std::cout << "create account file failed" << std::endl;
        return 1;
    }
    save_data_elem(fd, m_pub_key);
    save_data_elem(fd, m_pri_key);
    save_data_elem(fd, m_account);

    close(fd);
    return 0;
}
int32_t xconfig::load_data_elem(int fd, std::string & value)
{
    const int buf_size = 4096;
    int len = 0;
    int ret = read(fd, (char *)&len, 4);
    if(ret != 4 || len > buf_size){
        return 1;
    }
    char buf[buf_size] = {0};
    ret = read(fd, buf, len);
    if(ret != len){
        return 1;
    }
    value.append(buf, len);
    return 0;
}

int32_t xconfig::load_account()
{
    if(access(m_account_file.c_str(), O_RDONLY) != 0){
        std::cout << "the account file is not exist" << std::endl;
        return 1;
    }
    int fd = open(m_account_file.c_str(), O_RDONLY);
    if(fd < 0){
        return 1;
    }
    do {
        if(0 != load_data_elem(fd, m_pub_key)){
            break;
        }
        if(0 != load_data_elem(fd, m_pri_key)){
            break;
        }
        if(0 != load_data_elem(fd, m_account)){
            break;
        }
        m_local_addr.m_account = m_account;
        return 0;
    } while(0);

    std::cout << "Load account file failed" << std::endl;
    close(fd);
    return 1;
}

static void dump_config_and_exit(const std::string &info, const xJson::Value &root) {
    xJson::StyledWriter writer;
    const auto str = writer.write(root);
    ::printf("[config] %s: %s\n", info.c_str(), str.c_str());
    ::exit(1);
}

void xconfig::load_testnet_config(const xJson::Value &root) {
    if (root.isMember("testnet_elect_cycle")) {
        m_elect_cycle = root["testnet_elect_cycle"].asInt();
        if (m_elect_cycle < 5 || m_elect_cycle > 60 || 60 % m_elect_cycle != 0)
            dump_config_and_exit("testnet_elect_cycle should belong to {5,10,12,20,30,60}", root);
    }
    ::printf("elect cycle: %d minutes\n", m_elect_cycle);

    // node type
    const auto node_type = root["testnet_node_type"].asString();
    if (node_type == "rpc") {
        m_free_node = false;
        m_zone_id = -1;
        m_shard_id = -1;
        m_node_type = elect::enum_node_type_rpc;
        ::printf("edge node\n");

        load_shard_info_from_config();
        return;
    }

    // consensus need zone/shard
    m_node_type = elect::enum_node_type_consensus;
    if (!root.isMember("testnet_zone_id")) { // free node
        ::printf("this is a free node\n");
        m_free_node = true;
        m_zone_id = -1;
        m_shard_id = -1;

        load_shard_info_from_config();
        return;
    }

    m_free_node = false;
    m_zone_id = root["testnet_zone_id"].asInt();
    if (m_zone_id < 0 || m_zone_id > 2)
        dump_config_and_exit("testnet_zone_id should belong to {0,1,2}", root);

    if (!root.isMember("testnet_shard_id")) { // just config zone_id
        ::printf("configed zs info: (%d, x)\n", (int)m_zone_id);
        for (int i = 1; ; ++i) { // make sure zone_id
            if (get_zone_id(m_account) == m_zone_id)
                break;
            generate_account(i);
        }
        m_shard_id = get_shard_id(m_account);
    } else { // config both zone_id and shard_id
        m_shard_id = root["testnet_shard_id"].asInt();
        if (m_shard_id < 0 || m_shard_id > 1)
            dump_config_and_exit("testnet_shard_id should belong to {0,1}", root);
        ::printf("configed zs info: (%d, %d)\n", (int)m_zone_id, (int)m_shard_id);

        for (int i = 1; ; ++i) {
            if (get_shard_id(m_account) == m_shard_id && get_zone_id(m_account) == m_zone_id)
                break;
            generate_account(i);
        }
    }

    ::printf("configed shard info: (%d, %d)\n", (int)m_zone_id, (int)m_shard_id);
    ::printf("zond_id: %d, node_type: %d\n", m_zone_id, m_node_type);

    load_shard_info_from_config();
}

void xconfig::generate_account(int n) {
    xecprikey_t pri_key;
    m_pri_key.assign((const char *)pri_key.data(), pri_key.size());
    xecpubkey_t pub_key = pri_key.get_public_key();
    m_pub_key.assign((const char *)pub_key.data(), pub_key.size());
    m_account = pub_key.to_address();
    m_local_addr.m_account = m_account;

    save_account();
    ::printf("<%d>: generated account: %s belong to (%d, %d)\n", n, m_account.c_str()
        , (int)get_zone_id(m_account), (int)get_shard_id(m_account));
}

xtop_node
xconfig::local_node() const
{
    std::lock_guard<std::mutex> lock{ m_local_addr_mutex };
    return m_local_addr;
}

void
xconfig::update_shard_info(std::uint32_t const zone_id, std::uint32_t const shard_id)
{
    std::lock_guard<std::mutex> lock{ m_local_addr_mutex };
    m_local_addr.m_zone_id = zone_id;
    m_local_addr.m_shard_id = shard_id;
}

void xconfig::load_shard_info_from_config()
{
    std::lock_guard<std::mutex> lock{ m_local_addr_mutex };
    m_local_addr.m_zone_id = m_zone_id;
    m_local_addr.m_shard_id = m_shard_id;
}

elect::xshard_info
xconfig::local_node_shard_info() const noexcept
{
    std::lock_guard<std::mutex> lock{ m_local_addr_mutex };
    return { m_local_addr.m_zone_id, m_local_addr.m_shard_id };
}


namespace top {
int32_t get_shard_id(const std::string &account) {
    const auto n_mid = account.size() / 2;
    const char ch_shard = account[n_mid]; // get the middle char avoid account's last char
    return ch_shard % 2;
}

int32_t get_zone_id(const std::string &account) {
    const auto n_mid = account.size() / 2;
    const char ch_zone = account[n_mid - 1];
    return ch_zone % 3;
}
}
