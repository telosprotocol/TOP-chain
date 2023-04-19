#pragma once

#include "xmbus/xevent.h"
#include "xdata/xblock.h"
#include "xcommon/xaddress.h"
#include "xstatistic/xbasic_size.hpp"
#include "xstatistic/xstatistic.h"

NS_BEG2(top, mbus)

class xevent_blockfetcher_t : public xbus_event_t, public xstatistic::xstatistic_obj_face_t {
public:
    enum _minor_type_ {
        none,
        newblock,
        newblockhash,
    };

    xevent_blockfetcher_t(_minor_type_ type, 
            const common::xnode_address_t &_network_self, 
            const common::xnode_address_t &_from_address)
    : xbus_event_t(xevent_major_type_blockfetcher, type, to_listener, true),
    xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_event_blockfetcher),
    network_self(_network_self),
    from_address(_from_address) {
    }
    ~xevent_blockfetcher_t() {statistic_del();}

    common::xnode_address_t network_self;
    common::xnode_address_t from_address;

    virtual int32_t get_class_type() const override {return xstatistic::enum_statistic_event_blockfetcher;}
private:
    virtual size_t get_object_size_real() const override {
        return sizeof(*this) + get_size(get_result_data()) + get_size(network_self.account_election_address().account_address().base_address().to_string()) + get_size(from_address.account_election_address().account_address().base_address().to_string());
    }
};

using xevent_blockfetcher_ptr_t = xobject_ptr_t<xevent_blockfetcher_t>;

class xevent_blockfetcher_block_t : public xevent_blockfetcher_t {
public:
    xevent_blockfetcher_block_t(
            const data::xblock_ptr_t &_block,
            const common::xnode_address_t &_network_self,
            const common::xnode_address_t &_from_address):
    xevent_blockfetcher_t(newblock, _network_self, _from_address),
    block(_block) {
    }

    data::xblock_ptr_t block;
};

class xevent_blockfetcher_blockhash_t : public xevent_blockfetcher_t {
public:
    xevent_blockfetcher_blockhash_t(
                const std::string &_address,
                uint64_t _height,
                const std::string &_hash,
                const common::xnode_address_t &_network_self,
                const common::xnode_address_t &_from_address):
    xevent_blockfetcher_t(newblockhash, _network_self, _from_address),
    address(_address),
    height(_height),
    hash(_hash) {
    }

    std::string address;
    uint64_t height;
    std::string hash;
};

NS_END2
