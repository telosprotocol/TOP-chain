#include "xbase/xcxx_config.h"
#if defined(XCXX20)
#include "xtransport/proto/ubuntu/transport.pb.h"
#else
#include "xtransport/proto/centos/transport.pb.h"
#endif
#include "xbase/xcontext.h"

#include <gtest/gtest.h>
#if 0
using top::transport::protobuf::RoutingMessage;

#define COUT_SIZE(name, packet) std::cout << name << " size : " << packet.get_size() << std::endl;

TEST(test_,msg_size_1){
    RoutingMessage message;
    message.set_ack_id(UINT32_MAX); // 2byte-6byte  which is 0-4byte(depends on its value )+ 2byte
    message.set_src_node_id("ffffff76760b4a36aaf062b6e8fce02600000000a835e127245768f2e87bba7d92fd8273"); // 72byte + 2byte
    message.set_des_node_id("ffffff76760b4a36aaf062b6e8fce02600000000a835e127245768f2e87bba7d92fd8273"); // 72byte + 2byte
    message.set_type(0); // 2byte-6byte same. 0-4byte depends on value,
    message.set_broadcast(true); // 3bytes 1bytes + 2bytes
    top::base::xpacket_t packet(top::base::xcontext_t::instance());
    _xip2_header xip2_header; // 56 byte
    memset(&xip2_header, 0, sizeof(xip2_header));
    std::cout << "xip2_header" << sizeof(xip2_header) << std::endl;
    xip2_header.ver_protocol = 1;
    std::string header((const char*)&xip2_header, sizeof(xip2_header));

    uint32_t vhash = top::base::xhash32_t::digest("12345");
    std::string header_hash = std::to_string(vhash);
    std::cout << header_hash << " size : " << header_hash.size() << std::endl;
    message.mutable_gossip(); // 156->159 3byte
    message.mutable_gossip()->set_header_hash(header_hash); // 159->171 12byte  10byte size + 2byte

    std::string xbody;
    message.SerializeToString(&xbody);
    std::cout << "message_size : " << xbody.size() << std::endl;


    std::string xdata = header + xbody;

    packet.reset();
    COUT_SIZE("invalid_packet", packet);
    packet.get_body().push_back((uint8_t *)xdata.data(), xdata.size());
    COUT_SIZE("empty_packet", packet);
    packet.set_to_ip_addr("127.0.0.1");
    packet.set_to_ip_port(1234);
    
    COUT_SIZE("set_ip_port", packet);

    EXPECT_EQ(1,1);

}

void message_to_packet_size(RoutingMessage message){
    
    top::base::xpacket_t packet(top::base::xcontext_t::instance());
    _xip2_header xip2_header; // 56 byte
    memset(&xip2_header, 0, sizeof(xip2_header));
    xip2_header.ver_protocol = 1;
    std::string header((const char*)&xip2_header, sizeof(xip2_header));

    std::string xbody;
    message.SerializeToString(&xbody);
    
    std::string xdata = header + xbody;
    packet.reset();

    packet.get_body().push_back((uint8_t *)xdata.data(), xdata.size());

    packet.set_to_ip_addr("127.0.0.1");
    packet.set_to_ip_port(8080);

    std::cout << "// message body size: " << xbody.size() << " packet size: " << packet.get_size() << std::endl;
}

TEST(test_,msg_size){
    RoutingMessage message;

    #define SIZE message_to_packet_size(message);

    SIZE;                                                             // message body size: 0 packet size: 56
    message.set_src_node_id("ffffff76760b4a36aaf062b6e8fce0260000");  // 36byte + 2byte
    SIZE;                                                             // message body size: 38 packet size: 94
    message.set_des_node_id("ffffff76760b4a36aaf062b6e8fce0260000");  // 36byte + 2byte
    SIZE;                                                             // message body size: 76 packet size: 132
    message.mutable_gossip();                                         // 1byte + 2byte
    SIZE;                                                             // message body size: 79 packet size: 135
    message.mutable_gossip()->set_neighber_count(3);                  // 2~6byte : 0-4byte(depends on its value)+ 2byte
    SIZE;                                                             // message body size: 81 packet size: 137
    message.mutable_gossip()->set_stop_times(3);
    message.mutable_gossip()->set_gossip_type(1);
    message.mutable_gossip()->set_max_hop_num(10);
    message.mutable_gossip()->set_switch_layer_hop_num(3);
    message.mutable_gossip()->set_ign_bloomfilter_level(1);
    message.set_hop_num(5);
    message.set_type(1160);
    SIZE;                         // message body size: 97 packet size: 153
    message.set_is_root(true);    // 3bytes 1bytes + 2bytes
    message.set_broadcast(true);  // 3bytes 1bytes + 2bytes
    SIZE;                         // message body size: 103 packet size: 159

    message.mutable_gossip()->set_header_hash("0123456789");  // 10bytes + 2bytes
    SIZE;                                                     // message body size: 115 packet size: 171

    message.add_bloomfilter((uint64_t)0);
    SIZE;                                           // message body size: 118 packet size: 174
    message.add_bloomfilter((uint64_t)UINT64_MAX);  // 3~12bytes?
    SIZE;                                           // message body size: 130 packet size: 186

    for (auto index = 0; index < 10; ++index) {
        message.add_bloomfilter((uint64_t)23456789012345 + index);
    }
    SIZE;  // message body size: 220 packet size: 276
}


TEST(test_,msg_size_simple_hash){
    RoutingMessage message;

    #define SIZE message_to_packet_size(message);
    SIZE;                                     
    message.mutable_gossip();                                      
    SIZE;                                                             
    message.mutable_gossip()->set_neighber_count(3);                 
    SIZE;                                                            
    message.mutable_gossip()->set_switch_layer_hop_num(3);
    message.mutable_gossip()->set_stop_times(3);
    message.mutable_gossip()->set_gossip_type(8);
    message.mutable_gossip()->set_max_hop_num(10);
    message.mutable_gossip()->set_ign_bloomfilter_level(1);

    message.mutable_gossip()->clear_msg_hash();
    message.mutable_gossip()->clear_block();
    message.clear_data();
    message.clear_bloomfilter();
    message.clear_src_node_id();
    message.clear_des_node_id();

    message.set_hop_num(5);
    message.set_type(1160);
    SIZE;                        
    message.set_is_root(true);    
    message.set_broadcast(true);  
    SIZE;                        

    message.mutable_gossip()->set_header_hash("2684374495");  
    SIZE;        // message body size: 39 packet size: 95                                             
    
    message.set_id(123456678);
    SIZE;
    message.set_priority(0 << 12);
    SIZE;
    message.mutable_gossip()->set_evil_rate(0);
    message.mutable_gossip()->set_left_overlap(0);
    message.mutable_gossip()->set_right_overlap(0);
    SIZE;
    message.mutable_gossip()->set_msg_hash(12341556);
    SIZE;
}
#endif