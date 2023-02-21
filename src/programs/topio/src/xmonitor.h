#pragma once

#include <stdint.h>

#include <string>

class xmonitor_t {
public:
    bool monitor_cpu_and_network();
    std::string get_default_eth_device();
    void dump_cpu_mem();
    void dump_net();

private:
    // cpu
    int m_sys_cpu_load{0};  // percent

    // cpu temp
    uint64_t m_last_cpu_used_since_boot{0};
    uint64_t m_last_cpu_idle_since_boot{0};

    // mem
    int m_sys_mem_load{0};  // percent

    // net
    int32_t m_sys_net_in_speed{0};       // kbps
    int32_t m_sys_net_in_throughout{0};  // pps
    int32_t m_sys_net_in_drop{0};        // pps
    int64_t m_sys_net_in_bytes_of_month{0};
    int64_t m_sys_net_in_packets_of_month{0};

    int32_t m_sys_net_out_speed{0};       // kbps
    int32_t m_sys_net_out_throughout{0};  // pps
    int32_t m_sys_net_out_drop{0};        // pps
    int64_t m_sys_net_out_bytes_of_month{0};
    int64_t m_sys_net_out_packets_of_month{0};

    // net temp
    uint64_t m_last_scan_timestamp{0};
    std::string default_eth_device;
    uint64_t m_last_sys_rx_bytes;         // received bytes
    uint64_t m_last_sys_rx_packets;       // received packets
    uint64_t m_last_sys_rx_drop_packets;  // error or dropped packets when recv
    uint64_t m_last_sys_tx_bytes;         // sent bytes
    uint64_t m_last_sys_tx_packets;       // sent packets
    uint64_t m_last_sys_tx_drop_packets;  // error or dropped packets when send
};
