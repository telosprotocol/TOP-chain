// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <memory>
#include <list>
#include "xvnetwork/xmessage.h"
#include "xbasic/xbyte_buffer.h"
#include "xdata/xdata_common.h"
#include "xdata/xblock.h"

NS_BEG2(top, sync)

/**
 * main list:
 *  0: T-x-2BxfTxM5
 *  1: T-a-gRD2qVpp
 *  2: T-s-oNVNkY6z
 *  3: T-z-rSxFzh2E
 *
 * second list：
 *  0,0,12
 *  1,0,32
 *  2,1,30
 *  2,2,20
 *  ......
 *  2,256,30
 *  3,0,100
 */

struct xgossip_chain_info_t {
    std::string owner;
    uint64_t max_height;
};

using xgossip_chain_info_ptr_t = std::shared_ptr<xgossip_chain_info_t>;

class xgossip_message_t {
public:
    struct account_id_t {
        std::string address;
        bool valid{true};
        std::string key;

        account_id_t(const std::string& addr) :
        address(addr) {
            key = addr;
        }

        account_id_t() = default;
        account_id_t(const account_id_t&) = default;

        void do_read(uint32_t version, base::xstream_t& stream) {
            stream >> address;
        }

        void do_write(base::xstream_t& stream) {
            stream << address;
        }

        bool operator< (const account_id_t& id) const {
            return key < id.key;
        }
    };
    struct blockchain_info_t {
        uint16_t index{};   // index of main list
        uint16_t suffix{};  // can be book id or table id, for other sys contract, 0
        uint64_t height{};  // block hight of blockchain
        uint64_t viewid{};

        void do_read(uint32_t version, base::xstream_t& stream) {
            stream >> index;
            stream >> suffix;
            stream >> height;
            stream >> viewid;
        }

        void do_write(base::xstream_t& stream) {
            stream << index;
            stream << suffix;
            stream << height;
            stream << viewid;
        }
    };

    virtual ~xgossip_message_t() {
    }

    xbyte_buffer_t create_payload(
            std::vector<xgossip_chain_info_ptr_t>& info_list,
            xbyte_buffer_t& bloom_data, bool include_bloom_data = true);

    void parse_payload(const xbyte_buffer_t& msg,
            std::vector<xgossip_chain_info_ptr_t>& info_list,
            xbyte_buffer_t& bloom_data, bool include_bloom_data = true);

protected:
    bool update(const std::vector<std::string>& list, std::string& str);
};

using xgossip_message_ptr_t = std::shared_ptr<xgossip_message_t>;

NS_END2
