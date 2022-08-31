#include "xquic_node.h"

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

    m_client.init();

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

void xquic_node_t::send(std::string addr, uint32_t port, top::xbytes_t const & data) {
    user_conn_t * conn = m_client.connect((char *)addr.c_str(), port);
    if (conn == nullptr) {
        // todo add ec return;
    }
    assert(conn != nullptr);
    m_client.send(conn, data);
    m_client.release_connection(conn);
}

void xquic_node_t::on_quic_message_ready(top::xbytes_t const & bytes) {
    printf("quic node recv: %s \n", std::string{bytes.begin(), bytes.end()}.c_str());
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
    std::shared_ptr<xquic_node_t> node_ptr;
    while ((ch = getopt(argc, argv, "p:t:s:")) != -1) {
        switch (ch) {
        case 'p': {
            printf("option port :%s\n", optarg);
            server_port = atoi(optarg);
            node_ptr = std::make_shared<xquic_node_t>(server_port);
            node_ptr->start();
            break;
        }
        case 't': {
            printf("option port :%s\n", optarg);
            send_cnt = atoi(optarg);
            printf("send count: %zu\n", send_cnt);
            break;
        }
        case 's': {
            printf("option port :%s\n", optarg);
            send_port = atoi(optarg);

            std::string data{"Message from client to server in demo "};
            for (auto i = 0; i < send_cnt; ++i) {
                auto sdata = data + std::to_string(i);
                top::xbytes_t send_data{sdata.begin(), sdata.end()};
                node_ptr->send("127.0.0.1", send_port, send_data);
            }
            break;
        }
        default: {
            printf("not support\n");
        }
        }
    }

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
