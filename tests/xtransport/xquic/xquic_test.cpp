#include "gtest/gtest.h"
#include "xtransport/xquic_node/xquic_node.h"

#include <atomic>
#include <random>

// same as DEFAULT_QUIC_SERVER_PORT_DETLA in xquic_node.cpp
#if defined(DEBUG) || defined(XBUILD_CI) || defined(XBUILD_DEV)
static const std::size_t DEFAULT_QUIC_SERVER_PORT_DETLA = 1000;  // quic_port is greater than p2p_port;
#else
static const std::size_t DEFAULT_QUIC_SERVER_PORT_DETLA = 1;  // quic_port is greater than p2p_port;
#endif

NS_BEG4(top, transport, quic, tests)

#define TEST_NODE_COUNT 20
#define EACH_THREAD_SEND_COUNT 1000

std::atomic<std::size_t> recv_cnt{0};

static void recv_callback(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
    // xinfo("recv %s", message.data().c_str());
    recv_cnt++;
}

std::shared_ptr<xquic_node_t> create_new_quic_node(std::size_t port) {
    auto quic_node = std::make_shared<xquic_node_t>(port);
    quic_node->register_on_receive_callback(std::bind(recv_callback, std::placeholders::_1, std::placeholders::_2));
    quic_node->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return quic_node;
}

std::size_t rand_num(std::size_t min, std::size_t max) {
    std::random_device rd;
    std::default_random_engine random(rd());
    std::size_t temp = max - min + 1;
    return (random() % temp) + min;
}

void randomly_send_thread(std::vector<std::shared_ptr<xquic_node_t>> const & nodes) {
    transport::protobuf::RoutingMessage proto_message;
    proto_message.set_data("message");
    std::string send_data_str = proto_message.SerializeAsString();
    for (std::size_t send_cnt = 0; send_cnt < EACH_THREAD_SEND_COUNT; ++send_cnt) {
        std::size_t send_index = rand_num(0, TEST_NODE_COUNT);
        std::size_t recv_index = rand_num(0, TEST_NODE_COUNT);
        nodes[send_index]->send_data(send_data_str, "127.0.0.1", 1000 + recv_index + DEFAULT_QUIC_SERVER_PORT_DETLA);
    }
}

TEST(test_xquic_node, multi_thread_send) {
    std::vector<std::shared_ptr<xquic_node_t>> nodes;
    for (std::size_t node_cnt = 0; node_cnt <= TEST_NODE_COUNT; ++node_cnt) {
        nodes.push_back(create_new_quic_node(1000 + node_cnt));
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    xinfo("quic nodes create finish");

    // randomly_send_thread(nodes);
    std::thread t1 = std::thread(&randomly_send_thread, nodes);
    std::thread t2 = std::thread(&randomly_send_thread, nodes);

    // std::this_thread::sleep_for(std::chrono::seconds(3));

    std::thread t3 = std::thread(&randomly_send_thread, nodes);
    std::thread t4 = std::thread(&randomly_send_thread, nodes);

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    // wait quic engine finish.
    std::this_thread::sleep_for(std::chrono::seconds(3));

    for (std::size_t node_cnt; node_cnt <= TEST_NODE_COUNT; ++node_cnt) {
        nodes[node_cnt]->stop();
    }
    xinfo("recv sum: %zu", recv_cnt.load());
    EXPECT_EQ(recv_cnt.load(), 4 * EACH_THREAD_SEND_COUNT);
}

NS_END4