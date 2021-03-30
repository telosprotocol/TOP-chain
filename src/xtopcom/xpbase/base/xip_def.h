#pragma once

//XIP = low_addr(64bit) OF XIP2
/*XIP is 64bit address of the logic & virtual IP at overlay network
XIP definition as total 64bit = [xnetwork_version#:8bit][xnetwork-id: 8-8-8 bit][xhost-id:32bit] =
{
    -[xnetwork_version#:8bit] //allow network upgrading time-on-time and 0 present any. e.g. elect round# at Chain Network
    -[xnetwork-id: 8-8-8 bit] //A-Class,B-Class,C-Class,D-Class,E-Class Network...
    -[zone-id:7bit|cluster-id:7bit|group-id:8bit|node-id:10bit]
}
*/
//pass in uint64_t of xip2.low_addr
#define     get_network_ver_from_xip(xip2_low64bit)     (int32_t)(((xip2_low64bit ) >> 56) & 0xFF)       //8bit
#define     get_network_id_from_xip(xip2_low64bit)      (int32_t)(((xip2_low64bit ) >> 32) & 0xFFFFFF)   //24bit
#define     set_network_ver_to_xip(xip2_low64bit,ver)   ( (xip2_low64bit) | (((uint64_t)(ver) & 0xFF)      << 56) )
#define     set_network_id_to_xip(xip2_low64bit,id)     ( (xip2_low64bit) | (((uint64_t)(id)  & 0xFFFFFF)  << 32) )
#define     reset_network_ver_to_xip(xip2_low64bit)     ( (xip2_low64bit) & (((uint64_t)(  0x00FFFFFFFFFFFFFF))) )
#define     reset_network_id_to_xip(xip2_low64bit)      ( (xip2_low64bit) & (((uint64_t)(  0xFF000000FFFFFFFF))) )


//new XIP 'host-id(32bit)  -[zone-id:7bit|cluster-id:7bit|group-id:8bit|node-id:10bit]
//pass in uint64_t of xip2.low_addr
#define     get_zone_id_from_xip(xip2_low64bit)         (int32_t)(((xip2_low64bit ) >> 25) & 0x7F)            //7bit
#define     get_cluster_id_from_xip(xip2_low64bit)      (int32_t)(((xip2_low64bit ) >> 18) & 0x7F)            //7bit
#define     get_group_id_from_xip(xip2_low64bit)        (int32_t)(((xip2_low64bit ) >> 10) & 0xFF)            //8bit
#define     get_node_id_from_xip(xip2_low64bit)         (int32_t)(((xip2_low64bit )      ) & 0x3FF)           //10bit
#define     get_server_id_from_xip(xip2_low64bit)       (uint32_t)((xip2_low64bit ) & 0xFFFFFFFF)             //32bit
//server id = zone_id:cluster_id:group_id:node_id,any server is valid to handle packet if server_id  is  0

//pass in uint64_t of xip2.low_addr
#define     set_zone_id_to_xip(xip2_low64bit,id)        (  (xip2_low64bit ) | (((uint64_t)(id) & 0x7F)       << 25) )
#define     set_cluster_id_to_xip(xip2_low64bit,id)     (  (xip2_low64bit ) | (((uint64_t)(id) & 0x7F)       << 18) )
#define     set_group_id_to_xip(xip2_low64bit,id)       (  (xip2_low64bit ) | (((uint64_t)(id) & 0xFF)       << 10) )
#define     set_node_id_to_xip(xip2_low64bit,id)        (  (xip2_low64bit ) | (((uint64_t)(id) & 0x3FF)           ) )
#define     set_server_id_to_xip(xip2_low64bit,id)      (  (xip2_low64bit ) | (((uint64_t)(id) & 0xFFFFFFFF)      ) )

//pass in uint64_t of xip2.low_addr
#define     reset_zone_id_to_xip(xip2_low64bit)         (  (xip2_low64bit ) & ((uint64_t) 0xFFFFFFFF01FFFFFF) )
#define     reset_cluster_id_to_xip(xip2_low64bit)      (  (xip2_low64bit ) & ((uint64_t) 0xFFFFFFFFFE03FFFF) )
#define     reset_group_id_to_xip(xip2_low64bit)        (  (xip2_low64bit ) & ((uint64_t) 0xFFFFFFFFFFFC03FF) )
#define     reset_node_id_to_xip(xip2_low64bit)         (  (xip2_low64bit ) & ((uint64_t) 0xFFFFFFFFFFFFFC00) )
#define     reset_server_id_to_xip(xip2_low64bit)       (  (xip2_low64bit ) & ((uint64_t) 0xFFFFFFFF00000000) )

/*
 //XIP2 is 128bit address like IPv6 design on top of XIP
 XIP2 = { [xnetwork-id:24bit] - [xnetwork_version#:8bit] - [xinterface_id:32bit] - [XIP: 64bit] } =
 {
     high 64bit:
         //xnetwork_type   refer enum_xnetwork_type
         //xaddress_domain is    enum_xaddress_domain_xip
         //xaddress_type   is    enum_xip_type
         -[enum_xaddress_domain_xip:1bit | enum_xip_type:2bit | xnetwork_type:5bit]
         -[xinterface_id:32bit]    //unique device id like hash(Mac Address),or elect round, or hash(Account#),or hash(public Key)
         or [xnetwork clock:32bit] //global unique clock height(usally it is time-block'height)
         -[process-id:4bit|router-id:4bit|switch-id:8bit|local-id:8bit]
 
    low  64bit:
 }
 */
    //   -[enum_xaddress_domain_xip:1bit | enum_xip_type:2bit | xnetwork_type:5bit]

//pass in uint64_t of xip2.high_addr
#define     get_address_domain_from_xip(xip2_high64bit)            (int32_t)(((xip2_high64bit) >> 63) & 0x01)      //1bit
#define     get_address_type_from_xip(xip2_high64bit)              (int32_t)(((xip2_high64bit) >> 61) & 0x03)      //2bit
#define     get_network_type_from_xip(xip2_high64bit)              (int32_t)(((xip2_high64bit) >> 56) & 0x1F)      //5bit
#define     set_address_domain_to_xip(xip2_high64bit,domain)       (  (xip2_high64bit) | (((uint64_t)(domain) & 0x01)   << 63) )
#define     set_address_type_to_xip(xip2_high64bit,type)           (  (xip2_high64bit) | (((uint64_t)(type)   & 0x03)   << 61) )
#define     set_network_type_to_xip(xip2_high64bit,type)           (  (xip2_high64bit) | (((uint64_t)(type)   & 0x1F)   << 56) )
#define     reset_address_domain_to_xip(xip2_high64bit)            (  (xip2_high64bit) & (((uint64_t)   0x7FFFFFFFFFFFFFFF)) )
#define     reset_address_type_to_xip(xip2_high64bit)              (  (xip2_high64bit) & (((uint64_t)   0x9FFFFFFFFFFFFFFF)) )
#define     reset_network_type_to_xip(xip2_high64bit)              (  (xip2_high64bit) & (((uint64_t)   0xE0FFFFFFFFFFFFFF)) )


//-[xinterface_id:32bit]    //unique device id like hash(Mac Address),or elect round, or hash(Account#),or hash(public Key)

//pass in uint64_t of xip2.high_addr
#define     get_interface_id_from_xip(xip2_high64bit)              (uint32_t)(((xip2_high64bit)>> 24) & 0xFFFFFFFF)                 //32bit
#define     set_interface_id_to_xip(xip2_high_64bit,id)            ( (xip2_high_64bit) | (((uint64_t)(id)  & 0xFFFFFFFF)<< 24) )
#define     reset_interface_id_to_xip(xip2_high_64bit)             ( (xip2_high_64bit) & (((uint64_t)(  0xFF00000000FFFFFF))) )

//XIP'endpoint-id(24bit)  -[process-id:4bit|local-router-id:4bit|switch-id:8bit|local-id:8bit]
//any process is valid to handle packet if process_id is 0
//client_id = router_id:switch_id:lcoal_id

//pass in uint64_t of xip2.high_addr
#define     get_process_id_from_xip(xip2_high64bit)                (int32_t)(((xip2_high64bit) >> 20) & 0x0F)              //4bit
#define     get_router_id_from_xip(xip2_high64bit)                 (int32_t)(((xip2_high64bit) >> 16) & 0x0F)              //4bit
#define     get_switch_id_from_xip(xip2_high64bit)                 (int32_t)(((xip2_high64bit) >> 8)  & 0xFF)              //8bit
#define     get_local_id_from_xip(xip2_high64bit)                  (int32_t)(((xip2_high64bit)     )  & 0xFF)              //8bit
#define     get_client_id_from_xip(xip2_high64bit)                 (int32_t)(((xip2_high64bit)   ) & 0xFFFFF)              //20bit


//pass in uint64_t of xip2.high_addr
#define     set_process_id_to_xip(xip2_high64bit,id)               (  (xip2_high64bit) | (((uint64_t)(id) & 0x0F) << 20) )  //4bit
#define     set_router_id_to_xip(xip2_high64bit,id)                (  (xip2_high64bit) | (((uint64_t)(id) & 0x0F) << 16) )  //4bit
#define     set_switch_id_to_xip(xip2_high64bit,id)                (  (xip2_high64bit) | (((uint64_t)(id) & 0xFF) << 8)  )  //8bit
#define     set_local_id_to_xip(xip2_high64bit,id)                 (  (xip2_high64bit) | (((uint64_t)(id) & 0xFF) ) )       //8bit
#define     reset_process_id_to_xip(xip2_high64bit)                (  (xip2_high64bit) & (((uint64_t)0xFFFFFFFFFF0FFFFF)) ) //4bit
#define     reset_router_id_to_xip(xip2_high64bit)                 (  (xip2_high64bit) & (((uint64_t)0xFFFFFFFFFFF0FFFF)) ) //4bit
#define     reset_switch_id_to_xip(xip2_high64bit)                 (  (xip2_high64bit) & (((uint64_t)0xFFFFFFFFFFFF00FF)) ) //8bit
#define     reset_local_id_to_xip(xip2_high64bit)                  (  (xip2_high64bit) & (((uint64_t)0xFFFFFFFFFFFFFF00)) ) //8bit
