#include "xmonitor.h"

#include <xbase/xutl.h>
#include <xbase/xcontext.h>

#include "xpbase/base/top_log.h"

using namespace top::base;

// trigger at main-loop /////////////////////////////////////

//return true if the event is already handled
bool xmonitor_t::monitor_cpu_and_network()
{
    const auto cur_time_ms = xtime_utl::time_now_ms();

    //scan system resource
    m_sys_cpu_load = xsys_utl::get_cpu_load(m_last_cpu_used_since_boot, m_last_cpu_idle_since_boot);
    int64_t system_free_memory_size;
    m_sys_mem_load = xsys_utl::get_memory_load(system_free_memory_size);
    dump_cpu_mem();

    if(0 == m_last_scan_timestamp)  // first time
    {
        m_last_scan_timestamp = cur_time_ms;
        default_eth_device = get_default_eth_device();
        xsys_utl::get_sys_net_info(default_eth_device,
                m_last_sys_rx_bytes, m_last_sys_rx_packets, m_last_sys_rx_drop_packets,
                m_last_sys_tx_bytes, m_last_sys_tx_packets, m_last_sys_tx_drop_packets);
    }
    else
    {
        const int64_t duration_ms = cur_time_ms - m_last_scan_timestamp;
        if(duration_ms > xcontext_t::enum_system_scan_interval)
        {
            m_last_scan_timestamp = cur_time_ms;
            uint64_t    now_sys_rx_bytes   = 0;      //received bytes
            uint64_t    now_sys_rx_packets = 0;      //received packets
            uint64_t    now_sys_rx_drop_packets = 0; //error or dropped packets when recv
            uint64_t    now_sys_tx_bytes = 0;        //sent bytes
            uint64_t    now_sys_tx_packets = 0;      //sent packets
            uint64_t    now_sys_tx_drop_packets = 0; //error or dropped packets when send
            if(xsys_utl::get_sys_net_info(default_eth_device,
                    now_sys_rx_bytes, now_sys_rx_packets, now_sys_rx_drop_packets,
                    now_sys_tx_bytes, now_sys_tx_packets, now_sys_tx_drop_packets))
            {
                // (bytes * 8bit) / (ms/1000) / 1024kbits
                m_sys_net_in_speed = (int32_t)(((now_sys_rx_bytes - m_last_sys_rx_bytes) << 3) / duration_ms);
                // packets / (ms/1000)
                m_sys_net_in_throughout = (int32_t)(((now_sys_rx_packets - m_last_sys_rx_packets) << 10) / duration_ms);
                m_sys_net_in_drop = (int32_t)(((now_sys_rx_drop_packets - m_last_sys_rx_drop_packets) << 10) / duration_ms);
                m_sys_net_in_bytes_of_month += (now_sys_rx_bytes - m_last_sys_rx_bytes);
                m_sys_net_in_packets_of_month += (now_sys_rx_packets - m_last_sys_rx_packets);

                // (bytes * 8bit) / (ms/1000) / 1024kbits
                m_sys_net_out_speed = (int32_t)(((now_sys_tx_bytes - m_last_sys_tx_bytes) << 3) / duration_ms);
                // packets / (ms/1000)
                m_sys_net_out_throughout = (int32_t)(((now_sys_tx_packets - m_last_sys_tx_packets) << 10) / duration_ms);
                m_sys_net_out_drop = (int32_t)(((now_sys_tx_drop_packets - m_last_sys_tx_drop_packets) << 10) / duration_ms);
                m_sys_net_out_bytes_of_month += (now_sys_tx_bytes - m_last_sys_tx_bytes);
                m_sys_net_out_packets_of_month += (now_sys_tx_packets - m_last_sys_tx_packets);

                //////assign back
                m_last_sys_rx_bytes = now_sys_rx_bytes;
                m_last_sys_rx_packets = now_sys_rx_packets;
                m_last_sys_rx_drop_packets = now_sys_rx_drop_packets;
                
                m_last_sys_tx_bytes = now_sys_tx_bytes;
                m_last_sys_tx_packets = now_sys_tx_packets;
                m_last_sys_tx_drop_packets = now_sys_tx_drop_packets;

                dump_net();
            }
        }
    }

    return true;
}

#include <stdio.h>

static bool get_default_net_device(std::string& default_net_device) {
	FILE * pfd = fopen("/proc/net/route", "r");
	if (NULL == pfd) {
		return false;
	}

    char buf[256];
	auto buf_len = sizeof(buf);
    char iface[256];
    char destination[256];
	while (fgets(buf, buf_len, pfd)) {
		sscanf(buf, "%s %s %*s %*s %*s %*s %*s %*s %*s %*s %*s\n", iface, destination);
		if (destination == std::string("00000000")) {
			default_net_device = iface;
            fclose(pfd);
            return true;
		}
	}

	fclose(pfd);
	return false;
}

std::string xmonitor_t::get_default_eth_device() {
    std::string default_net_device;
    get_default_net_device(default_net_device);
    return default_net_device;
}

void xmonitor_t::dump_cpu_mem() {
    if (m_sys_cpu_load >= 80)
    {
        TOP_FATAL("cpu load: %d%%", m_sys_cpu_load);
    }

    if (m_sys_mem_load >= 80)
    {
        TOP_FATAL("mem load: %d%%", m_sys_mem_load);
    }
}



void xmonitor_t::dump_net() {
    TOP_INFO("%5s | %10s %12s %8s %11s %13s", default_eth_device.c_str(),
            "speed/kbps", "throuout/pps", "drop/pps", "bytes/month", "packets/month");
    TOP_INFO("%5s | %10ld %12ld %8ld %11ld %13ld", "in",
            (long)m_sys_net_in_speed, (long)m_sys_net_in_throughout, (long)m_sys_net_in_drop,
            (long)m_sys_net_in_bytes_of_month, (long)m_sys_net_in_packets_of_month);
    TOP_INFO("%5s | %10ld %12ld %8ld %11ld %13ld", "out",
            (long)m_sys_net_out_speed, (long)m_sys_net_out_throughout, (long)m_sys_net_out_drop,
            (long)m_sys_net_out_bytes_of_month, (long)m_sys_net_out_packets_of_month);
}
