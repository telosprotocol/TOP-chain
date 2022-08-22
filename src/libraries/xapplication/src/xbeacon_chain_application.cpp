// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xapplication/xbeacon_chain_application.h"

#include "xapplication/xapplication.h"
#include "xapplication/xerror/xerror.h"
#include "xbasic/xerror/xerror.h"
#include "xbasic/xscope_executer.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xcommon/xip.h"
#include "xdata/xblocktool.h"
#include "xdata/xcodec/xmsgpack/xelection/xelection_result_store_codec.hpp"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xnative_contract_address.h"

#include <cinttypes>

NS_BEG2(top, application)

xtop_beacon_chain_application::xtop_beacon_chain_application(observer_ptr<xapplication_t> const & application,
                                                             xobject_ptr_t<base::xvblockstore_t> & blockstore,
                                                             xobject_ptr_t<base::xvnodesrv_t> & nodesvr_ptr,
                                                             xobject_ptr_t<base::xvcertauth_t> & cert_ptr,
                                                             observer_ptr<base::xiothread_t> const & grpc_thread,
                                                             observer_ptr<base::xiothread_t> const & sync_thread,
                                                             std::vector<observer_ptr<base::xiothread_t>> const & sync_account_thread_pool,
                                                             std::vector<observer_ptr<base::xiothread_t>> const & sync_handler_thread_pool)
  : xbase_t{application,
            common::xnetwork_id_t{top::config::to_chainid(XGET_CONFIG(chain_name))},
            blockstore,
            nodesvr_ptr,
            cert_ptr,
            grpc_thread,
            sync_thread,
            sync_account_thread_pool,
            sync_handler_thread_pool} {
}

/// @brief Load election data by the specified block height @p block_height_upper_limit. If block at the @p block_height_upper_limit doesn't exist, go ahead.
/// @param blockstore Block store instance for querying the blocks.
/// @param contract_address Election contract to be queried.
/// @param block_height_upper_limit Load block from this height, if block isn't seen at this height, go ahead.
/// @param property_name Property name to be queried.
/// @param returned_data_block_height The height of the block contains the return election data.
/// @param ec Log the error code in the operation.
/// @return The election data.
static top::data::election::xelection_result_store_t load_election_data(observer_ptr<data::xvblockstore_t> const & blockstore,
                                                                        common::xaccount_address_t const & contract_address,
                                                                        uint64_t const block_height_upper_limit,
                                                                        std::string const & property_name,
                                                                        uint64_t & returned_data_block_height,
                                                                        std::error_code & ec) {
    assert(!ec);

    std::string result;
    xobject_ptr_t<data::xblock_t> block{nullptr};
    auto block_height = block_height_upper_limit;

    do {
        // get committed lightunit from a specified height. the height of the returned block won't be higher than the specified height.
        xobject_ptr_t<data::xvblock_t> const vblock = data::xblocktool_t::get_committed_state_changed_block(blockstore.get(), contract_address.value(), block_height);
        if (vblock == nullptr) {
            ec = error::xerrc_t::load_election_data_missing_block;

            xwarn("load_election_data failed: category %s; msg: %s; contract address: %s; property %s; from height %" PRIu64,
                  ec.category().name(),
                  ec.message().c_str(),
                  contract_address.c_str(),
                  property_name.c_str(),
                  block_height_upper_limit);
            break;
        }
        assert(vblock->get_height() <= block_height);

        block = dynamic_xobject_ptr_cast<data::xblock_t>(vblock);
        if (block == nullptr) {
            ec = error::xerrc_t::load_election_data_block_type_mismatch;

            xwarn("load_election_data failed (block is null): category %s; msg: %s; contract address: %s; property %s; from height %" PRIu64,
                   ec.category().name(),
                   ec.message().c_str(),
                   contract_address.c_str(),
                   property_name.c_str(),
                   block_height_upper_limit);
            break;
        }

        base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(vblock.get(), metrics::statestore_access_from_application_load_election);
        if (bstate == nullptr) {
            ec = error::xerrc_t::load_election_data_missing_state;

            xwarn("load_election_data failed (state is null): category %s; msg: %s; contract address: %s; property %s; from height %" PRIu64,
                   ec.category().name(),
                   ec.message().c_str(),
                   contract_address.c_str(),
                   property_name.c_str(),
                   block_height_upper_limit);

            break;
        }

        data::xunit_bstate_t unitstate(bstate.get());
        if (xsuccess == unitstate.string_get(property_name, result)) {
            block_height = block->get_height();
            break;
        }

        auto min_height = std::min(block_height, block->get_height());
        if (min_height == 0) {
            ec = error::xerrc_t::load_election_data_missing_property;

            xwarn("load_election_data failed (genesis state empty): category %s; msg: %s; contract address: %s; property %s; from height %" PRIu64,
                   ec.category().name(),
                   ec.message().c_str(),
                   contract_address.c_str(),
                   property_name.c_str(),
                   block_height_upper_limit);
            // we fail to read election data even in the genesis block.
            break;
        } else {
            block_height = min_height - 1;
        }
    } while (true);

    if (ec) {
        xwarn("load_election_data failed");
        return {};
    }

    if (result.empty()) {
        ec = error::xerrc_t::load_election_data_property_empty;

        xwarn("load_election_data failed (property empty): category %s; msg: %s; contract address: %s; property %s; from height %" PRIu64,
               ec.category().name(),
               ec.message().c_str(),
               contract_address.c_str(),
               property_name.c_str(),
               block_height_upper_limit);
        return {};
    }

    returned_data_block_height = block_height;
    return codec::msgpack_decode<data::election::xelection_result_store_t>({ std::begin(result), std::end(result) });
}

void xtop_beacon_chain_application::load_last_election_data() {
    std::vector<common::xaccount_address_t> sys_addr{rec_elect_rec_contract_address,
                                                     rec_elect_archive_contract_address,
                                                     rec_elect_exchange_contract_address,
                                                     rec_elect_fullnode_contract_address,
                                                     rec_elect_edge_contract_address,
                                                     rec_elect_zec_contract_address,
                                                     zec_elect_consensus_contract_address,
                                                     zec_elect_eth_contract_address,
                                                     relay_make_block_contract_address};

    std::map<common::xaccount_address_t, common::xzone_id_t> addr_to_zone_id{{rec_elect_rec_contract_address, common::xcommittee_zone_id},
                                                                             {rec_elect_zec_contract_address, common::xzec_zone_id},
                                                                             {rec_elect_archive_contract_address, common::xstorage_zone_id},
                                                                             {rec_elect_exchange_contract_address, common::xstorage_zone_id},
                                                                             {rec_elect_fullnode_contract_address, common::xfullnode_zone_id},
                                                                             {rec_elect_edge_contract_address, common::xedge_zone_id},
                                                                             {zec_elect_consensus_contract_address, common::xdefault_zone_id},
                                                                             {zec_elect_eth_contract_address, common::xevm_zone_id},
                                                                             {relay_make_block_contract_address, common::xrelay_zone_id}};
    for (const auto & addr : sys_addr) {
        for (auto const & property : data::election::get_property_name_by_addr(addr)) {
            common::xzone_id_t zone_id = addr_to_zone_id[addr];
            using top::data::election::xelection_result_store_t;
            std::error_code ec;

            xwarn("xbeacon_chain_application::load_last_election_data begin. contract %s; property %s", addr.c_str(), property.c_str());
            xscope_executer_t loading_result_logger{ [&ec, &addr, property] {
                xwarn("xbeacon_chain_application::load_last_election_data end. contract %s; property %s", addr.c_str(), property.c_str());
            } };

            // only use lightunit
            xobject_ptr_t<base::xvblock_t> latest_vblock = data::xblocktool_t::get_latest_connectted_state_changed_block(m_application->blockstore().get(), addr.value());
            if (latest_vblock == nullptr) {
                xerror("xtop_beacon_chain_application::load_last_election_data has no latest lightunit. addr=%s", addr.c_str());
                continue;
            }

            uint64_t block_height = latest_vblock->get_height();
            xinfo("xbeacon_chain_application::load_last_election_data load block.addr=%s,height=%ld", addr.c_str(), block_height);
            auto const & last_election_result_store = load_election_data(m_application->blockstore(),
                                                                         addr,
                                                                         block_height,
                                                                         property,
                                                                         block_height,
                                                                         ec);
            if (ec) {
                if (block_height != 0) {
                    xerror("xbeacon_chain_application::load_last_election_data fail to load election data. addr %s; property %s; from height %" PRIu64,
                           addr.c_str(),
                           property.c_str(),
                           block_height);
                    top::error::throw_error(ec);
                } else {
                    continue;
                }
            }

            if ((addr == rec_elect_rec_contract_address || addr == rec_elect_zec_contract_address || addr == zec_elect_consensus_contract_address ||
                 addr == zec_elect_eth_contract_address || addr == relay_make_block_contract_address) &&
                block_height != 0) {
                uint64_t prev_block_height = block_height - 1;
                auto const & before_last_election_result_store = load_election_data(m_application->blockstore(), addr, prev_block_height, property, prev_block_height, ec);
                if (!ec) {
                    assert(block_height > prev_block_height);
                    on_election_data_updated(before_last_election_result_store, zone_id, prev_block_height);
                } else {
                    xwarn("xbeacon_chain_application::load_last_election_data fail to load prev election data. addr %s; property %s; from height %" PRIu64,
                          addr.c_str(),
                          property.c_str(),
                          prev_block_height);
                }
            }
            on_election_data_updated(last_election_result_store, zone_id, block_height);
        }
    }
}

NS_END2
