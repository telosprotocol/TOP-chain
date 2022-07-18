// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xns_macro.h"
#include "xpbase/base/top_utils.h"
#include "xbase/xutl.h"
#include "xevm_common/xtriehash.h"
#include "xevm_common/xtriecommon.h"
#include "xdata/xrelay_block_store.h"
#include "xdata/xnative_contract_address.h"
#include "xvledger/xvledger.h"
#include "xdata/xblockextract.h"

NS_BEG2(top, data)

using namespace top::evm_common;

bool xrelay_block_store::load_block_hash_from_db(uint64_t load_height, xrelay_block_save_leaf& block_leaf)
{
    bool result = true;

    // load form db directly
    base::xvaccount_t _table_addr(sys_contract_relay_block_addr);
    auto _db_block = base::xvchain_t::instance().get_xblockstore()->load_block_object(_table_addr, load_height, base::enum_xvblock_flag_authenticated, false);
    if (_db_block == nullptr) {
        xwarn("xrelay_block_store::load_block_hash_from_db block height(%d) fail-load", load_height);
        return false;
    }

    std::error_code ec;
    top::data::xrelay_block _db_relay_block;
    data::xblockextract_t::unpack_relayblock_from_wrapblock(_db_block.get(), _db_relay_block, ec);
    if (ec) {
        xerror("xrelay_block_store:load_block_hash_from_db decodeBytes decodeBytes error %s; err msg %s", ec.category().name(), ec.message().c_str());
        return false;
    }

    block_leaf.m_block_hash = _db_relay_block.get_block_hash();
    block_leaf.m_type = _db_relay_block.check_block_type();

    return result;
}

bool xrelay_block_store::get_all_leaf_block_hash_list_from_cache(const xrelay_block& poly_block, std::vector<h256>& leaf_hash_vector, bool include_self)
{
    leaf_hash_vector.clear();
    enum_block_cache_type block_type = poly_block.check_block_type();

    if (block_type != cache_poly_tx_block && block_type != cache_poly_election_block) {
        xwarn("xrelay_block_store::get_all_leaf_block_hash_list_from_cache block height(%d) type(%d) error",
            poly_block.get_block_height(), block_type);
        return false;
    }

    bool check_result = true;
    uint64_t last_height = poly_block.get_block_height();
    if (include_self) {
        //  const h256 _block_hash = poly_block.get_block_hash();
        leaf_hash_vector.insert(leaf_hash_vector.begin(), poly_block.get_block_hash());
    }

    while (last_height > 1) {
        last_height--;
        xrelay_block_save_leaf _block_leaf;
        check_result = load_block_hash_from_db(last_height, _block_leaf);
        if (check_result && (_block_leaf.m_type < block_type)) {
            if (_block_leaf.m_type == cache_tx_block) {
                leaf_hash_vector.insert(leaf_hash_vector.begin(), _block_leaf.m_block_hash);
            }
        } else {
            break;
        }
    }

    if (!check_result) {
        xwarn("xrelay_block_store:get_all_leaf_block_hash_list_from_cache  leaf_hash_vector clear ");
        leaf_hash_vector.clear();
    }
    return check_result;
}

bool xrelay_block_store::get_all_poly_block_hash_list_from_cache(const xrelay_block& tx_block, std::map<uint64_t, evm_common::h256>& block_hash_map)
{
    block_hash_map.clear();
    enum_block_cache_type block_type = tx_block.check_block_type();

    if (block_type != cache_tx_block) {
        xwarn("xrelay_block_store::get_all_poly_block_hash_list_from_cache block height(%d) type(%d) error",
            tx_block.get_block_height(), block_type);
        return false;
    }

    bool check_result = true;
    uint64_t last_height = tx_block.get_block_height();

    while (check_result) {
        last_height++;
        xrelay_block_save_leaf _block_leaf;
        check_result = load_block_hash_from_db(last_height, _block_leaf);
        if (check_result) {
            //
            if (_block_leaf.m_type > block_type) {
                xdbg("xrelay_block_store:get_all_poly_block_hash_list_from_cache  height(%d) poly  height(%d)  type(%d).",
                    tx_block.get_block_height(), last_height, _block_leaf.m_type);
                block_hash_map.insert(std::make_pair(last_height, _block_leaf.m_block_hash));
                block_type = _block_leaf.m_type;
                // chain_id_set |= _block_leaf.m_chain_id;
                if (block_type == cache_poly_election_block) {
                    xinfo("xrelay_block_store:get_all_poly_block_hash_list_from_cache tx_block height(%d) poly election height(%d).",
                        tx_block.get_block_height(), last_height, _block_leaf.m_type);
                    break;
                }
            }
        } else {
            break;
        }
    }

    // tx poly block is build, but election poly block is no, it's ok
    if (block_hash_map.size() > 0) {
        check_result = true;
    }

    return check_result;
}

NS_END2
