#include <gtest/gtest.h>
#include "xsync/xblock_fetcher.h"
#include "xsync/xsync_sender.h"
#include "tests/xvnetwork/xdummy_vhost.h"
#include "xdata/tests/test_blockutl.hpp"
#include "xsync/xsync_util.h"
#include "xsync/xchain_info.h"

using namespace top;
using namespace top::sync;
using namespace top::mbus;
using namespace top::data;

class xmock_vhost_t : public top::tests::vnetwork::xtop_dummy_vhost {

};
#if 0
TEST(xblock_fetcher, test) {

    std::string address = xdatautil::serialize_owner_str(sys_contract_beacon_table_block_addr, 0);
    xmock_store_t store;
    std::shared_ptr<xmessage_bus_t> mbus = std::make_shared<top::mbus::xmessage_bus_t>(true, 1000);
    xmock_vhost_t vhost;
    xsync_sender_t sync_sender("", &vhost, nullptr);

    xchain_info_t chain_info;
    chain_info.address = address;

    xobject_ptr_t<base::xiothread_t> thread = make_object_ptr<base::xiothread_t>();

    std::shared_ptr<sync::xblock_fetcher_t> block_fetcher = std::make_shared<sync::xblock_fetcher_t>("", make_observer(thread), make_observer(mbus), &store, nullptr, &sync_sender);


    std::vector<base::xvblock_t*> block_vector;
    base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_table(address);

    base::xvblock_t* prev_block = genesis_block;
    block_vector.push_back(prev_block);

    for (uint64_t i=1; i<=5; i++) {
        prev_block = test_blocktuil::create_next_emptyblock(prev_block);
        block_vector.push_back(prev_block);
    }

    store.current_block = genesis_block;

    base::xvblock_t* vblock = block_vector[5];

    top::common::xnode_address_t from_address;
    top::common::xnode_address_t network_self;

    block_fetcher->handle_newblockhash(address, vblock->get_height(), vblock->get_viewid(), from_address, network_self);
}

#endif