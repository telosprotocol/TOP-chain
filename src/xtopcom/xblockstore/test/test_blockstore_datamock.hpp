#pragma once
#include <string>
#include <vector>
#include <iostream>
#include "gtest/gtest.h"
#include "xbase/xmem.h"
#include "xbase/xutl.h"
#include "xbase/xcontext.h"
#include "xdata/xdata_defines.h"
#include "xdata/xblock.h"
#include "xdata/xblockchain.h"
#include "xdata/xlightunit.h"
#include "xdata/xfullunit.h"
#include "xdata/xtransaction.h"
#include "xdata/xdatautil.h"
#include "xdata/xblocktool.h"
#include "xdata/xchain_param.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xconfig/xconfig_register.h"
#include "xcommon/xip.h"
#include "xdata/xcodec/xmsgpack/xelection_result_store_codec.hpp"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xelection/xelection_result.h"
#include "xdata/xelection/xelection_result.h"

#include "xmutisig/xschnorr/xschnorr.h"

#include "xcrypto/xckey.h"
#include "xcrypto/xcrypto_util.h"
#include "xcodec/xmsgpack_codec.hpp"

#include "xstore/xaccount_cmd.h"
#include "xdata/tests/test_blockutl.hpp"
#include "xbasic/xutility.h"
#include "xbase/xobject_ptr.h"
#include "xbasic/xcrypto_key.h"

#include "xvledger/xvledger.h"
#include "xblockstore/xblockstore_face.h"
#include "xvledger/xvblock.h"
#include "xdata/tests/test_blockutl.hpp"

using namespace top;
using namespace top::data;
using namespace top::store;
using namespace top::base;

using xlightunit_input_ptr_t = std::string;  // TODO(jimmy) just for compile
using xlightunit_state_ptr_t = std::string;  // TODO(jimmy) just for compile

class test_blockstore_datamock_t {
 public:
    explicit test_blockstore_datamock_t(store::xstore_face_t* store, base::xvblockstore_t* blockstore):
    m_store(store),
    m_blockstore(blockstore) {
    }

    std::string get_assign_table_address(uint32_t table_id) {
        return get_assign_table_batch_address(table_id, 1)[0];
    }

    std::vector<std::string> get_assign_table_batch_address(uint32_t table_id, uint32_t count) {
        // std::vector<std::string>  vec_address;
        // for (uint32_t i = 0; i < 100000; i++) {
        //     std::string address = top::utl::xcrypto_util::make_address_by_random_key(0, 0);
        //     if (table_id == data::xaccount_mapping::account_to_table_id(address)) {
        //         std::cout << "make new address " << table_id << " " << address << std::endl;
        //         vec_address.push_back(address);
        //         if (vec_address.size() >= count) {
        //             return vec_address;
        //         }
        //     }
        // }
        assert(0);
        return {};
    }

    xblock_ptr_t create_unit(const std::string & address, const std::map<std::string, std::string> &prop_list, uint64_t timer_height=1, bool need_store = true) {
#if 0
        xblockchain2_t* account = m_store->clone_account(address);
        if (account == nullptr) {
            base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_empty_unit(address);
            assert(m_blockstore->store_block(genesis_block));
            account = m_store->clone_account(address);
        } else {
            if (account->is_state_behind()) {
                assert(0);
                return nullptr;
            }
        }
#else
        xblockchain2_t* account = m_store->clone_account(address);
        if (account == nullptr) {
            base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_empty_unit(address);
            assert(m_store->set_vblock(address, genesis_block));
            account = m_store->clone_account(address);
        } else {
            if (account->is_state_behind()) {
                assert(0);
                return nullptr;
            }
        }
#endif
        auto& config_register = top::config::xconfig_register_t::get_instance();

        xauto_ptr<xvblock_t> latest_block = m_blockstore->get_latest_locked_block(address);
        xvblock_t *prev_block = latest_block.get();

        uint32_t fullunit_num = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num);
        if ((account->get_chain_height() + 1) % fullunit_num == 0) {

            xfullunit_block_para_t block_para;
            block_para.m_account_state = account->get_account_mstate();  // TODO(jimmy) account ptr pass for performance
            block_para.m_first_unit_hash = account->get_last_full_unit_hash();
            block_para.m_first_unit_height = account->get_last_full_unit_height();

            base::xvblock_t* proposal_block = nullptr;

            if (need_store) {
                proposal_block = test_blocktuil::create_next_fullunit_with_consensus(block_para, prev_block, timer_height);
            } else {
                proposal_block = test_blocktuil::create_next_fullunit(block_para, prev_block, timer_height);
            }

            if (need_store) {
                base::xvaccount_t _vaddress(proposal_block->get_account());
                assert(m_blockstore->store_block(_vaddress, proposal_block));
            }


            xblock_t* block = dynamic_cast<xblock_t*>(proposal_block);

            block->add_ref();
            xdataobj_ptr_t data_obj = nullptr;
            data_obj.attach(block);
            xblock_ptr_t blk = dynamic_xobject_ptr_cast<xblock_t>(data_obj);

            return blk;

        } else {
            uint64_t amount = 100;
            std::string to_account = "T-to-xxxxxxxxxxxxxxxxxxxxx";
            xtransaction_ptr_t tx = account->make_transfer_tx(to_account, -amount, 0, 0, 0);

            // create property
            xaccount_cmd accountcmd(account, m_store);
            int prop_ret = create_and_modify_property(accountcmd, address, prop_list);
            xproperty_log_ptr_t bin_log = nullptr;
            if (prop_ret == 0) {
                auto prop_hashs = accountcmd.get_property_hash();
                assert(prop_hashs.size() == 1);
            }

            xlightunit_block_para_t block_para;
            block_para.set_one_input_tx(tx);
            block_para.set_balance_change(amount);
            block_para.set_property_log(accountcmd.get_property_log());
            block_para.set_propertys_change(accountcmd.get_property_hash());

            base::xvblock_t* proposal_block = nullptr;

            if (need_store) {
                proposal_block = test_blocktuil::create_next_lightunit_with_consensus(block_para, prev_block, timer_height);
            } else {
                proposal_block = test_blocktuil::create_next_lightunit(block_para, prev_block, timer_height);
            }

            if (need_store) {
                base::xvaccount_t vaddress(proposal_block->get_account());
                assert(m_blockstore->store_block(vaddress, proposal_block));
            }

            xblock_t* block = dynamic_cast<xblock_t*>(proposal_block);
            block->add_ref();
            xdataobj_ptr_t data_obj = nullptr;
            data_obj.attach(block);
            xblock_ptr_t blk = dynamic_xobject_ptr_cast<xblock_t>(data_obj);

            return blk;
        }

        return nullptr;
    }

    int create_and_modify_property(xaccount_cmd &accountcmd, const std::string &address, const std::map<std::string, std::string> &prop_list) {

        if (prop_list.size() == 0)
            return -1;

        base::xauto_ptr<base::xvblock_t> vblock = m_blockstore->get_latest_committed_block(address);

        uint64_t height = 0;
        if (vblock != nullptr)
            height = vblock->get_height();

        if (height == 0) {
            for (auto &it: prop_list) {
                const std::string & prop_name = it.first;
                const std::string & value = it.second;
                auto ret = accountcmd.list_create(prop_name, true);
                assert(ret == 0);
            }
        } else {
            for (auto &it: prop_list) {
                const std::string & prop_name = it.first;
                const std::string & value = it.second;

                int32_t error_code;
                auto prop_ptr = accountcmd.get_property(prop_name, error_code);

                // property must initiate at first block
                if (prop_ptr == nullptr) {
                    assert(0);
                }
                auto ret = accountcmd.list_push_back(prop_name, value, true);
                assert(ret == 0);
            }
        }

        return 0;
    }

    xblock_ptr_t create_lightunit(const std::string & address, const xlightunit_input_ptr_t & input,
        const xlightunit_state_ptr_t & state, uint64_t timer_height=1) {
    #if 0
        auto account = m_store->clone_account(address);
        if (account == nullptr) {
            account = make_object_ptr<xaccount_t>(address);
        }
        xblock_ptr_t unit = account->create_new_block(enum_xunit_type::xunit_type_lightunit);
        unit->add_transaction(input);
        unit->add_transaction(state);
        unit->get_block_header()->set_timerblock_height(timer_height);
        unit->calc_block_hash();
        return unit;
    #endif
        xassert(0);
        return nullptr;
    }
#if 0
    xtransaction_ptr_t create_tx_create_user_account(const std::string &from, uint16_t expire_time = 100) {
        uint64_t last_nonce = 0;
        uint256_t last_hash;

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_deposit(100000);
        tx->make_tx_create_user_account(from);
        tx->set_last_trans_hash_and_nonce(last_hash, last_nonce);
        tx->set_same_source_target_address(from);
        tx->set_fire_and_expire_time(expire_time);
        tx->set_digest();
        return tx;
    }

    xtransaction_ptr_t create_tx_transfer(const std::string &from, const std::string &to, uint64_t amount) {
        auto account = m_store->clone_account(from);
        if (account == nullptr) {
            account = new xblockchain2_t(from);
        }
        xtransaction_ptr_t tx = account->make_transfer_tx(to, -amount, 0, 0, 0);

        return tx;
    }
#endif
    xblock_ptr_t transfer(const std::string &from, const std::string &to, uint64_t amount) {
#if 0
        xaccount_ptr_t account = m_store->clone_account(from);
        if (account == nullptr) {
            return nullptr;
        }

        xblock_ptr_t unit = account->create_new_block(enum_xunit_type::xunit_type_lightunit);  // TODO(jimmy)

        xtransaction_ptr_t tx = create_tx_transfer(from, to, amount);
        xlightunit_input_ptr_t unit_input = make_object_ptr<xlightunit_input_t>();
        unit_input->add_tx(false, tx);
        unit->add_transaction(unit_input);

        xlightunit_state_ptr_t unit_state = make_object_ptr<xlightunit_state_t>();
        unit_state->m_balance_change = 100;
        unit->add_transaction(unit_state);

        unit->calc_block_hash();

        assert(unit->get_block_type() == data::enum_xblock_type::xblock_type_unit);
        auto ret = m_store->set_block(unit);
        assert(ret == 0);
        ret = m_store->set_transaction(tx);
        assert(ret == 0);
        return unit;
#endif
    xassert(0);
    return nullptr;
    }
#if 0
    xcons_transaction_ptr_t create_cons_transfer_tx(const std::string & from, const std::string & to, uint64_t amount = 100) {
        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        data::xproperty_asset asset(amount);
        tx->make_tx_transfer(asset);
        tx->set_different_source_target_address(from, to);
        tx->set_digest();
        tx->set_tx_subtype(enum_transaction_subtype_send);

        xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(tx.get());
        return cons_tx;
    }
#endif

    xblock_ptr_t create_tableblock(const std::set<std::string> &set_account, const std::map<std::string, std::string> &prop_list, uint64_t timer_height=1) {

        std::string table_addr;
        for (auto it: set_account) {
            std::string &address = it;
            std::string tmpowner = account_address_to_block_address(top::common::xaccount_address_t{address});

            if (table_addr.empty())
                table_addr = tmpowner;
            else if (table_addr != tmpowner) {
                xassert(0);
                return nullptr;
            }
        }

        // get units
        xtable_block_para_t table_para;
        for (auto &it: set_account) {
            xblock_ptr_t block_ptr = create_unit(it, prop_list, timer_height, false);
            block_ptr->add_ref();
            table_para.add_unit(block_ptr.get());
        }

        // build table
        base::xauto_ptr<base::xvblock_t> vblock_ptr = m_blockstore->get_latest_committed_block(table_addr);
        base::xvblock_t* prev_block = vblock_ptr.get();
        base::xvblock_t* proposal_block = test_blocktuil::create_next_tableblock(table_para, prev_block, timer_height);
        assert(!proposal_block->get_block_hash().empty());
        
        base::xvaccount_t vaddress(proposal_block->get_account());
        assert(m_blockstore->store_block(vaddress, proposal_block));

        proposal_block->add_ref();
        xdataobj_ptr_t data_obj = nullptr;
        data_obj.attach(proposal_block);
        xblock_ptr_t blk = dynamic_xobject_ptr_cast<xblock_t>(data_obj);

        return blk;
    }

    using adjust_block_func = std::function<void(xblock_ptr_t& block)>;

    xblock_ptr_t create_electblock( std::string const& addr, uint32_t max_nodes,
                                    common::xnode_type_t const& type, common::xcluster_id_t const& cid, common::xgroup_id_t const& gid,
                                    std::vector<top::xmutisig::key_pair_t>& out_key_pairs,
                                    std::vector<top::xmutisig::key_pair_t> const& sign_block_key_pairs,
                                    std::vector<top::xmutisig::key_pair_t> const& sign_block_parent_key_pairs,
                                    adjust_block_func _func) {
        // top::data::election::xelection_result_store_t result_store;
        // prepare_result_store_and_ordered_keys(max_nodes, type, cid, gid, result_store, out_key_pairs);
        // auto state = make_object_ptr<xlightunit_state_t>();
        // auto result = codec::msgpack_encode<data::election::xelection_result_store_t>(result_store);
        // state->m_native_property.native_string_set(data::XPROPERTY_CONTRACT_ELECTION_RESULT_KEY, std::string((char*) result.data(), result.size()));

        // xblock_ptr_t block;
        // m_store->create_new_block(data::enum_xblock_type::xblock_type_unit, addr, data::enum_xunit_type::xunit_type_lightunit, block);
        // int64_t now = base::xtime_utl::gmttime_ms();
        // block->set_timestamp((uint64_t)now);
        // block->add_transaction(state);
        // _func(block);
        // block->calc_block_hash();
        xassert(0);

        // top::consensus::test::xmock_sign::sign_block(block, sign_block_key_pairs, sign_block_parent_key_pairs);

        // if(m_store->set_block(block) == 0) return block;
        return nullptr;
    }

    void prepare_result_store_and_ordered_keys(uint32_t max_nodes,common::xnode_type_t const& type, common::xcluster_id_t const& cid, common::xgroup_id_t const& gid,
                                    top::data::election::xelection_result_store_t& result_store,
                                    std::vector<top::xmutisig::key_pair_t>& out_key_pairs) {
#if 0
        // elect data
        std::vector<top::xmutisig::key_pair_t> list;
        auto& nodes = result_store.result_of(type).result_of(cid).result_of(gid);
        nodes.start_time(0);
        for(uint32_t i=0;i<max_nodes;i++) {
            top::xmutisig::key_pair_t keypair = top::xmutisig::xschnorr::instance()->generate_key_pair();
            list.push_back(keypair);

            top::data::election::xelection_info_bundle_t election_info_bundle{};
            common::xnode_id_t node_id{std::string("test_node_id_with_") + std::to_string(i)};
            election_info_bundle.node_id(node_id);
            top::data::election::xelection_info_t info;
            info.standby_info.pubkey = top::xcrypto_key_t<top::pub>(keypair.second.get_serialize_str());
            election_info_bundle.election_info(info);
            nodes.insert(std::move(election_info_bundle));
        }
        // ordered key pairs
        std::vector<std::string> ordered_pubkeys;
        get_ordered_keys(result_store, ordered_pubkeys);
        for(auto& pk : ordered_pubkeys) {
            for(auto it=list.begin();it != list.end();++it) {
                if(it->second.get_serialize_str() == pk) {
                    out_key_pairs.push_back(*it);
                    break;
                }
            }
        }
#endif
    xassert(0);
    }

    void get_ordered_keys(top::data::election::xelection_result_store_t const& result_store, std::vector<std::string>& pubkeys) {
#if 0
        for (auto const & election_result_info : result_store) {
            auto const & election_result = top::get<data::election::xelection_result_t>(election_result_info);
            for (auto const & cluster_result_info : election_result) {
                auto const & cluster_result = top::get<data::election::xelection_cluster_result_t>(cluster_result_info);
                for (auto const & group_result_info : cluster_result) {
                    auto const & group_id = top::get<common::xgroup_id_t const>(group_result_info);
                    auto const & group_result = top::get<data::election::xelection_group_result_t>(group_result_info);
                    for (auto const & node_info : group_result) {
                        auto const & election_info = top::get<data::election::xelection_info_bundle_t>(node_info).election_info();
                        pubkeys.push_back(election_info.standby_info.pubkey.to_string());
                    }
                }
            }
        }
#endif
    }

    xblock_ptr_t create_normal_signed_block(std::string const& addr,
                                    std::vector<top::xmutisig::key_pair_t> const& sign_block_key_pairs,
                                    std::vector<top::xmutisig::key_pair_t> const& sign_block_parent_key_pairs,
                                    adjust_block_func _func) {
#if 0
        xblock_ptr_t block;
        m_store->create_new_block(data::enum_xblock_type::xblock_type_unit, addr, data::enum_xunit_type::xunit_type_lightunit, block);
        int64_t now = base::xtime_utl::gmttime_ms();
        block->set_timestamp((uint64_t)now);
        _func(block);
        block->calc_block_hash();

        top::consensus::test::xmock_sign::sign_block(block, sign_block_key_pairs, sign_block_parent_key_pairs);

        return block;
#endif
    xassert(0);
    return nullptr;
    }

    store::xstore_face_t* m_store;
    base::xvblockstore_t* m_blockstore{};
};
