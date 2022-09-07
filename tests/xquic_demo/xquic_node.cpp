#include "xquic_node.h"

#include "test_data.h"
#include "xbasic/xthreading/xbackend_thread.hpp"

#include <cassert>

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

void xquic_node_t::send(std::string addr, uint32_t port, top::xbytes_t const & data) {
    auto addr_port = addr + std::to_string(port);

    // erase closed connection:
    if (m_conn_map.find(addr_port) != m_conn_map.end() && m_conn_map.at(addr_port)->cid.cid_len == 0) {
        m_conn_map.erase(addr_port);
    }

    if (m_conn_map.find(addr_port) == m_conn_map.end()) {
        cli_user_conn_t * new_conn = m_client.connect((char *)addr.c_str(), port);
        m_conn_map.insert({addr_port, new_conn});
    }
    cli_user_conn_t * conn = m_conn_map.at(addr_port);
    if (conn == nullptr) {
        // todo add ec return;
        return;
    }
    assert(conn != nullptr);
    __TIME_RECORD_BEGIN(inner_send);
    m_client.send(conn, data);
    __TIME_RECORD_END(inner_send, g_send_2);
}

static std::size_t global_recv_cnt = 0;

void xquic_node_t::on_quic_message_ready(top::xbytes_t const & bytes) {
    global_recv_cnt++;
    // printf("quic node recv: %s \n", std::string{bytes.begin(), bytes.end()}.c_str());
    printf(BOLDBLUE "quic node recv: %zu num:%zu at % " PRIu64 " \n" RESET, bytes.size(), global_recv_cnt, xqc_now());
}

int main(int argc, char * argv[]) {
    // xquic_node_t node;
    // std::shared_ptr<xquic_node_t> node_ptr = std::make_shared<xquic_node_t>(9999);
    // node_ptr->start();

    // std::string data{"Message from client to server in demo"};
    // top::xbytes_t send_data{data.begin(), data.end()};

    // node_ptr->send("127.0.0.1", 9999, send_data);

    // std::this_thread::sleep_for(std::chrono::seconds(1));

    int ch = 0;
    uint32_t server_port = 9999;
    uint32_t send_port = 9999;
    std::size_t send_cnt = 1;
    std::size_t wait_us = 1;
    std::size_t data_sz = 10;
    std::shared_ptr<xquic_node_t> node_ptr;
    while ((ch = getopt(argc, argv, "p:t:w:s:z:")) != -1) {
        switch (ch) {
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

        case 'z': {
            data_sz = atoi(optarg);
            std::string data{"1111111111111111"};
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

            for (auto i = 0; i < send_cnt; ++i) {
                // auto sdata = data + std::to_string(i);
                // auto sdata = data;
                top::xbytes_t send_data{data.begin(), data.end()};
                __TIME_RECORD_BEGIN(out_send);
                node_ptr->send("127.0.0.1", send_port, send_data);
                __TIME_RECORD_END(out_send, g_send_1);
                __TIME_RECORD_BEGIN(sleep_cnt);
                
                std::chrono::microseconds sleep_duration{wait_us};
                auto now = std::chrono::high_resolution_clock::now();
                while (true) {
                    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - now);
                    if (elapsed > sleep_duration)
                        break;
                }

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
