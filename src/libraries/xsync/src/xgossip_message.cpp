// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <map>
#include "xbase/xmem.h"
#include "xsync/xgossip_message.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xdatautil.h"

NS_BEG2(top, sync)

xbyte_buffer_t xgossip_message_t::create_payload(
        std::vector<xgossip_chain_info_ptr_t>& info_list,
        xbyte_buffer_t& bloom_data,
        bool include_bloom_data) {

    base::xstream_t stream(base::xcontext_t::instance());

    uint32_t version = 1;
    stream << version;

    std::vector<account_id_t>       main_list;
    std::vector<blockchain_info_t>  second_list;

    std::map<account_id_t, uint32_t> map;
    for (auto& ptr : info_list) {
        std::string addr;
        uint32_t table_id{0};

        common::xaccount_address_t account_addr{ptr->owner};
        if (data::is_sys_contract_address(account_addr) || data::is_table_address(account_addr)) {
            if (!data::xdatautil::extract_parts(ptr->owner, addr, table_id)) {
                xwarn("[xgossip_message_t::create_message] wrong sys contract address %s", ptr->owner.c_str());
                continue;
            }
        } else {
            continue;
        }

        uint32_t index;
        auto it = map.find(addr);
        if (it == map.end()) {
            index = map.size();
            map[addr] = index;
        } else {
            index = it->second;
        }

        blockchain_info_t info;
        info.height = ptr->max_height;
        info.index = (uint16_t) index;
        info.suffix = (uint16_t) table_id;
        second_list.push_back(info);

        //xdbg("xgossip_message_t send %s %lu %u %u", addr.c_str(), info.height, info.index, info.suffix);
    }

    main_list.resize(map.size());
    for(auto& pair : map) {
        auto& id = main_list[pair.second];
        id.address = pair.first.address;
    }

#define WRITE_ARRAY(arr)            \
    {                               \
        uint32_t size = arr.size(); \
        stream << size;             \
        for(auto& v : arr) {        \
            v.do_write(stream);     \
        }                           \
    }

    WRITE_ARRAY(main_list);
    WRITE_ARRAY(second_list);
    if (include_bloom_data)
        stream << bloom_data;

    return { stream.data(), stream.data() + stream.size() };
}

void xgossip_message_t::parse_payload(const xbyte_buffer_t& msg,
        std::vector<xgossip_chain_info_ptr_t>& info_list,
        xbyte_buffer_t& bloom_data,
        bool include_bloom_data) {

    try {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*) msg.data(), msg.size());

        // version, don't add to account_id_t and blockchain_info_t for size case
        uint32_t version;
        stream >> version;

        std::vector<account_id_t>       main_list;  // list of sys contracts, first part: N charactors
        std::vector<blockchain_info_t>  second_list;

#define READ_ARRAY(type, array)         \
    {                                   \
        uint32_t size;                  \
        stream >> size;                 \
        type t;                         \
        for(uint32_t i=0;i<size;i++) {  \
            t.do_read(version, stream); \
            array.push_back(t);         \
        }                               \
    }
        // main list
        READ_ARRAY(account_id_t, main_list);
        // second list
        READ_ARRAY(blockchain_info_t, second_list);
        // bloom data
        if (include_bloom_data)
            stream >> bloom_data;

        for(auto& info : second_list) {
            assert(info.index < main_list.size());
            if(info.index < main_list.size()) {
                auto& id = main_list[info.index];
                if(!id.valid) continue;

                std::string address;
                // verify data

                address = data::xdatautil::serialize_owner_str(id.address, info.suffix);

                xgossip_chain_info_ptr_t ptr = std::make_shared<xgossip_chain_info_t>();
                ptr->owner = address;
                ptr->max_height = info.height;
                info_list.push_back(ptr);

                //xdbg("xgossip_message_t recv %s %lu %u %u", address.c_str(), info.height, info.index, info.suffix);
            }
        }
    } catch(...) {
        xwarn("[xgossip_message_t::parse_message_payload] error format of gossip message");
    }
}

bool xgossip_message_t::update(const std::vector<std::string>& list, std::string& str) {
    for(auto& s : list) {
        if(s.find(str) == 0) {
            str = s;
            return true;
        }
    }
    return false;
}

NS_END2
