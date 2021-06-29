#pragma once

#include "xmbus/xevent.h"
#include "xdata/xblock.h"
#include "xvnetwork/xaddress.h"

NS_BEG2(top, mbus)

class xevent_blockfetcher_t : public xevent_t {
public:
    enum _minor_type_ {
        none,
        newblock,
        newblockhash,
    };

    xevent_blockfetcher_t(_minor_type_ type, 
            const vnetwork::xvnode_address_t &_network_self, 
            const vnetwork::xvnode_address_t &_from_address)
    : xevent_t(xevent_major_type_blockfetcher, type, to_listener, true),
    network_self(_network_self),
    from_address(_from_address) {
    }

    vnetwork::xvnode_address_t network_self;
    vnetwork::xvnode_address_t from_address;
};

using xevent_blockfetcher_ptr_t = xobject_ptr_t<xevent_blockfetcher_t>;

class xevent_blockfetcher_block_t : public xevent_blockfetcher_t {
public:
    xevent_blockfetcher_block_t(
            const data::xblock_ptr_t &_block,
            const vnetwork::xvnode_address_t &_network_self,
            const vnetwork::xvnode_address_t &_from_address):
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
                const vnetwork::xvnode_address_t &_network_self,
                const vnetwork::xvnode_address_t &_from_address):
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
