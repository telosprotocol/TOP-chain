#pragma once

#include "xmbus/xevent.h"
#include "xdata/xblock.h"
#include "xvnetwork/xaddress.h"

NS_BEG2(top, mbus)

class xevent_blockfetcher_t : public xevent_t {
public:
    enum _minor_type_ {
        none,
        consensus,
        gossip,
        newblock,
        newblockhash,
    };

    xevent_blockfetcher_t(_minor_type_ type, 
            const vnetwork::xvnode_address_t &_network_self, 
            const vnetwork::xvnode_address_t &_target_address)
    : xevent_t(xevent_major_type_blockfetcher, type, to_listener, true),
    network_self(_network_self),
    target_address(_target_address) {
    }

    vnetwork::xvnode_address_t network_self;
    vnetwork::xvnode_address_t target_address;
};

DEFINE_SHARED_PTR(xevent_blockfetcher);

class xevent_blockfetcher_block_t : public xevent_blockfetcher_t {
public:
    xevent_blockfetcher_block_t(
            _minor_type_ _minor_type,
            const data::xblock_ptr_t &_block,
            const vnetwork::xvnode_address_t &_network_self,
            const vnetwork::xvnode_address_t &_target_address):
    xevent_blockfetcher_t(_minor_type, _network_self, _target_address),
    block(_block) {
    }

    data::xblock_ptr_t block;
};

class xevent_blockfetcher_blockinfo_t : public xevent_blockfetcher_t {
public:
    xevent_blockfetcher_blockinfo_t(
                _minor_type_ _minor_type,
                const std::string &_address,
                uint64_t _height,
                uint64_t _view_id,
                const vnetwork::xvnode_address_t &_network_self,
                const vnetwork::xvnode_address_t &_target_address):
    xevent_blockfetcher_t(_minor_type, _network_self, _target_address),
    address(_address),
    height(_height),
    view_id(_view_id) {
    }

    std::string address;
    uint64_t height;
    uint64_t view_id;
};

class xevent_blockfetcher_consensus_t : public xevent_blockfetcher_block_t {
public:
    xevent_blockfetcher_consensus_t(const data::xblock_ptr_t &_block,
            const vnetwork::xvnode_address_t &_network_self,
            const vnetwork::xvnode_address_t &_target_address):
    xevent_blockfetcher_block_t(consensus, _block, _network_self, _target_address) {
    }
};

class xevent_blockfetcher_gossip_t : public xevent_blockfetcher_blockinfo_t {
public:
    xevent_blockfetcher_gossip_t(const std::string &_address,
                uint64_t _height,
                uint64_t _view_id,
                const vnetwork::xvnode_address_t &_network_self,
                const vnetwork::xvnode_address_t &_target_address):
    xevent_blockfetcher_blockinfo_t(gossip, _address, _height, _view_id, _network_self, _target_address) {
    }
};

class xevent_blockfetcher_newblock_t : public xevent_blockfetcher_block_t {
public:
    xevent_blockfetcher_newblock_t(const data::xblock_ptr_t &_block,
            const vnetwork::xvnode_address_t &_network_self,
            const vnetwork::xvnode_address_t &_target_address):
    xevent_blockfetcher_block_t(newblock, _block, _network_self, _target_address) {
    }
};

class xevent_blockfetcher_newblockhash_t : public xevent_blockfetcher_blockinfo_t {
public:
    xevent_blockfetcher_newblockhash_t(const std::string &_address,
                uint64_t _height,
                uint64_t _view_id,
                const vnetwork::xvnode_address_t &_network_self,
                const vnetwork::xvnode_address_t &_target_address):
    xevent_blockfetcher_blockinfo_t(newblockhash, _address, _height, _view_id, _network_self, _target_address) {
    }
};


NS_END2
