#include "xquic_node.h"

#include "test_data.h"
#include "xbasic/xthreading/xbackend_thread.hpp"

#include <cassert>

#ifdef LEAK_TRACER
#    include "leaktracer/MemoryTrace.hpp"

#    include <csignal>
#    include <fstream>
#    include <iostream>
#endif

xquic_node_t::xquic_node_t(unsigned int const _server_port) : m_server_port{_server_port} {
}

void xquic_node_t::start() {
    assert(!running());

    auto self = shared_from_this();
    top::threading::xbackend_thread::spawn([this, self] {
#if defined DEBUG
        m_quic_server_thread_id = std::this_thread::get_id();
#endif
        m_server.init(std::bind(&xquic_node_t::on_quic_message_ready, shared_from_this(), std::placeholders::_1), m_server_port);
    });

    top::threading::xbackend_thread::spawn([this, self] { m_client.init(); });

    std::this_thread::sleep_for(std::chrono::seconds(1));

    running(true);
    assert(running());

    //     auto self = shared_from_this();
    //     top::threading::xbackend_thread::spawn([this, self] {
    // #if defined DEBUG
    //         m_quic_node_thread_id = std::this_thread::get_id();
    // #endif
    //         do_handle_quic_node_message();
    //     })
}

void xquic_node_t::stop() {
    assert(running());
    running(false);
    assert(!running());
}

static uint64_t g_send_1{0};
static uint64_t g_send_2{0};
static uint64_t g_sleep{0};

#define __TIME_RECORD_BEGIN(name) auto st_bg_##name = xqc_now();
#define __TIME_RECORD_END(name, g)                                                                                                                                                 \
    auto st_ed_##name = xqc_now();                                                                                                                                                 \
    g += (st_ed_##name - st_bg_##name);

bool xquic_node_t::send(std::string addr, uint32_t port, top::xbytes_t data) {
    auto addr_port = addr + std::to_string(port);

    // erase closed connection:
    if (m_conn_map.find(addr_port) != m_conn_map.end() && m_conn_map.at(addr_port)->conn_status == cli_conn_status_t::after_connected) {
        delete m_conn_map.at(addr_port);
        printf("erase closed connections\n");
        m_conn_map.erase(addr_port);
    }

    if (m_conn_map.find(addr_port) == m_conn_map.end()) {
        cli_user_conn_t * new_conn = m_client.connect((char *)addr.c_str(), port);
        if (new_conn == nullptr) {
            // todo add ec return;
            printf("try connection error\n");
            return false;
        }
        assert(new_conn);
        // printf("new cli_user_conn: (%p) , stream: (%p)\n", new_conn, new_conn->cli_user_stream);
        m_conn_map.insert({addr_port, new_conn});
    }
    cli_user_conn_t * conn = m_conn_map.at(addr_port);
    if (conn == nullptr) {
        assert(false);
        // todo add ec return;
        return false;
    }
    if (conn->conn_status == cli_conn_status_t::before_connected && xqc_now() - conn->conn_create_time > BEFORE_WELL_CONNECTED_KEEP_MSG_TIMER) {
        // printf("not well establish\n");
        return false;
    }
    if (m_client.send_queue_full()) {
        // todo add ec return;
        // printf("send_queue_full,  \n");
        return false;
    }

    assert(conn != nullptr);
    __TIME_RECORD_BEGIN(inner_send);
    m_client.send(conn, std::move(data));
    __TIME_RECORD_END(inner_send, g_send_2);

    return true;
}

static std::size_t global_recv_cnt = 0;

void xquic_node_t::on_quic_message_ready(top::xbytes_t const & bytes) {
    global_recv_cnt++;
    if (global_recv_cnt % 1000 == 0) {
        // printf("quic node recv: %s \n", std::string{bytes.begin(), bytes.end()}.c_str());
        printf(BOLDBLUE "quic node recv: %zu num:%zu at % " PRIu64 " \n" RESET, bytes.size(), global_recv_cnt, xqc_now());
    }
}

#ifdef LEAK_TRACER
void export_mem_trace(int signal) {
    leaktracer::MemoryTrace::GetInstance().stopMonitoringAllocations();
    leaktracer::MemoryTrace::GetInstance().stopAllMonitoring();

    std::ofstream oleaks;

    oleaks.open("leaks.out", std::ios_base::out);
    if (oleaks.is_open())
        leaktracer::MemoryTrace::GetInstance().writeLeaks(oleaks);
    else
        std::cerr << "Failed to write to \"leaks.out\"\n";

    oleaks.close();
}
#endif
int main(int argc, char * argv[]) {
    // xquic_node_t node;
    // std::shared_ptr<xquic_node_t> node_ptr = std::make_shared<xquic_node_t>(9999);
    // node_ptr->start();

    // std::string data{"Message from client to server in demo"};
    // top::xbytes_t send_data{data.begin(), data.end()};

    // node_ptr->send("127.0.0.1", 9999, send_data);

    // std::this_thread::sleep_for(std::chrono::seconds(1));

#ifdef LEAK_TRACER
    std::signal(SIGUSR1, export_mem_trace);
    leaktracer::MemoryTrace::GetInstance().startMonitoringAllThreads();
#endif
    int ch = 0;
    char server_addr[64] = "127.0.0.1";
    uint32_t server_port = 9999;
    uint32_t send_port = 9999;
    std::size_t send_cnt = 1;
    std::size_t dest_cnt = 1;
    std::size_t wait_us = 1;
    std::size_t data_sz = 10;
    std::string data{"1111111111111111"};
    std::shared_ptr<xquic_node_t> node_ptr;
    while ((ch = getopt(argc, argv, "a:p:t:w:s:r:z:")) != -1) {
        switch (ch) {
        case 'a': {
            snprintf(server_addr, sizeof(server_addr), optarg);
            break;
        }
        case 'p': {
            // printf("option port :%s\n", optarg);
            server_port = atoi(optarg);
            node_ptr = std::make_shared<xquic_node_t>(server_port);
            node_ptr->start();
            break;
        }
        case 't': {
            send_cnt = atoi(optarg);
            printf("send count: %zu\n", send_cnt);
            break;
        }
        case 'w': {
            wait_us = atoi(optarg);
            printf("wait us: %zu\n", wait_us);
            break;
        }
        case 's': {
            // printf("option port :%s\n", optarg);
            send_port = atoi(optarg);
            break;
        }
        case 'r': {
            // send_port_range:
            dest_cnt = atoi(optarg);
            break;
        }

        case 'z': {
            data_sz = atoi(optarg);
            switch (data_sz) {
            case 0:
                data = _500B_data;
                break;
            case 1:
                data = _1KB_data;
                break;
            case 10:
                data = _10KB_data;
                break;
            case 100:
                data = _100KB_data;
                break;
            case 1000:
                data = _1000KB_data;
                break;
            default:
                break;
            }
            for (auto i = 0; i < send_cnt;) {
                // auto sdata = data + std::to_string(i);
                // auto sdata = data;
                // top::xbytes_t send_data{data.begin(), data.end()};
                bool res{false};
                __TIME_RECORD_BEGIN(out_send);
                do {
                    for (std::size_t port_detal = 0; port_detal < dest_cnt; ++port_detal) {
                        res = node_ptr->send(server_addr, send_port + port_detal, {data.begin(), data.end()});
                        if (res == false)
                            break;
                    }
                    send_cnt++;
                } while (res);
                send_cnt--;
                __TIME_RECORD_END(out_send, g_send_1);
                __TIME_RECORD_BEGIN(sleep_cnt);
                std::this_thread::sleep_for(std::chrono::microseconds(wait_us));

                // use too much cpu.
                // std::chrono::microseconds sleep_duration{wait_us};
                // auto now = std::chrono::high_resolution_clock::now();
                // while (true) {
                //     auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - now);
                //     if (elapsed > sleep_duration)
                //         break;
                // }

                /// resolution is low. 10us minimum
                // std::this_thread::sleep_for(std::chrono::microseconds{wait_us});  // test timeouted connection
                __TIME_RECORD_END(sleep_cnt, g_sleep);
            }
            break;
        }
        default: {
            printf("not support\n");
        }
        }
    }

    printf("g_static: g_send_1 : %" PRIu64 " \n", g_send_1);
    printf("g_static: g_send_2 : %" PRIu64 " \n", g_send_2);
    printf("g_static: g_sleep : %" PRIu64 " \n", g_sleep);
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
