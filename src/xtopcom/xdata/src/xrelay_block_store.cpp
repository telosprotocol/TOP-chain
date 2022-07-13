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


bool  xrelay_block_store::set_block_merkle_root_from_store(xrelay_block &next_block)
{
    bool check_result = true;
    h256  block_root_hash{0};
    enum_block_cache_type block_type = next_block.check_block_type();

    if (cache_error_block == block_type) {
        return false;
    } else if (block_type == cache_tx_block) {
        next_block.set_block_merkle_root_hash(block_root_hash);
        return true;
    }
    
    std::vector<h256> _leaf_hash_vector;
    uint64_t chain_id_set = 0x0;
    check_result = get_all_leaf_block_hash_list_from_cache(next_block, _leaf_hash_vector, false);

    if (check_result) {
        if (_leaf_hash_vector.size() > 0) {
            std::vector<bytes> _blocks_hash_poly;
            for (auto &block_hash : _leaf_hash_vector) {
                _blocks_hash_poly.push_back(block_hash.to_bytes());
            }
            block_root_hash = orderedTrieRoot(_blocks_hash_poly);
            xinfo("set_block_merkle_root_from_store poly block height(%d) root hash[%s]",next_block.get_block_height(), block_root_hash.hex().c_str());
        }
        next_block.set_block_merkle_root_hash(block_root_hash);
        //next_block.set_chain_id(chain_id_set);
    }  
   
    return check_result;
}

bool xrelay_block_store::load_block_hash_from_cache(uint64_t load_height, xrelay_block_save_leaf &block_leaf)
{
    bool result = true;
    //if (!m_tx_block_map.get(load_height, block_leaf)) {
        //not found, load form db
    base::xvaccount_t _table_addr(sys_contract_relay_block_addr);
    auto _db_block = base::xvchain_t::instance().get_xblockstore()->load_block_object(_table_addr, load_height, base::enum_xvblock_flag_authenticated, false);
    if (_db_block == nullptr) {
        xwarn("xrelay_block_store::load_block_hash_from_cache block height(%d) fail-load", load_height);
        return false;
    }

    std::error_code ec;
    top::data::xrelay_block  _db_relay_block;
    data::xblockextract_t::unpack_relayblock(_db_block.get(), false, _db_relay_block, ec);    
    if (ec) {
        xerror("xrelay_block_store:load_block_hash_from_cache decodeBytes decodeBytes error %s; err msg %s", ec.category().name(), ec.message().c_str());
        return false;
    }
    
    block_leaf.m_block_hash = _db_relay_block.get_block_hash();
    block_leaf.m_type = _db_relay_block.check_block_type();

       /* result = save_block_hash_to_store_cache(_db_relay_block);
        if (!result) {
            return false;
        }*/
        
        //result = m_tx_block_map.get(load_height, block_leaf);
   // }
    return result;
}


bool xrelay_block_store::get_all_leaf_block_hash_list_from_cache(const xrelay_block &poly_block, std::vector<h256>  &leaf_hash_vector, bool include_self)
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
        leaf_hash_vector.insert(leaf_hash_vector.begin(),  poly_block.get_block_hash());
    }
    
    while (last_height > 1) {
        last_height--;
        xrelay_block_save_leaf _block_leaf;
        check_result = load_block_hash_from_cache(last_height, _block_leaf);
        if (check_result) {
            if (_block_leaf.m_type < block_type) {
                leaf_hash_vector.insert(leaf_hash_vector.begin(), _block_leaf.m_block_hash);
                //chain_id_set |= _block_leaf.m_chain_id;
            } else {
                break;
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

bool xrelay_block_store::get_all_poly_block_hash_list_from_cache(const xrelay_block &tx_block, std::map<uint64_t, evm_common::h256> &block_hash_map)
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
        check_result = load_block_hash_from_cache(last_height, _block_leaf);
        if (check_result) {
            //
            if (_block_leaf.m_type > block_type) {
                xdbg("xrelay_block_store:get_all_poly_block_hash_list_from_cache  height(%d) poly  height(%d)  type(%d).",
                      tx_block.get_block_height(), last_height, _block_leaf.m_type);
                block_hash_map.insert(std::make_pair(last_height, _block_leaf.m_block_hash));
                block_type = _block_leaf.m_type;
                //chain_id_set |= _block_leaf.m_chain_id;
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
    if (block_hash_map.size() > 0 ) {
        check_result = true;
    }
    
    return check_result;
}


bool  xrelay_block_store::save_block_hash_to_store_cache(xrelay_block &next_block)
{
    enum_block_cache_type block_typ = next_block.check_block_type();
    if (cache_error_block == block_typ) {
        xwarn("xrelay_block_store::save_block_hash_to_store_cache height(%d) check_block_type type error", next_block.get_block_height());
        return false;
    }
    xdbg("xrelay_block_store::save_block_hash_to_store_cache height(%d) check_block_type type(%d)", next_block.get_block_height(), block_typ);
    return save_tx_block_hash_to_tx_map(next_block, block_typ);
}

//ok
bool  xrelay_block_store::save_tx_block_hash_to_tx_map(xrelay_block &next_block, enum_block_cache_type block_typ)
{
    xrelay_block_save_leaf _block_leaf;
    if (m_tx_block_map.get(next_block.get_block_height(), _block_leaf)) {
        if (_block_leaf.m_block_hash != next_block.get_block_hash()) {
            xwarn("xrelay_block_store:save_tx_block_hash_to_tx_map  block height(%ld) , save hash(%s)  and  block hash (%s) not compare",
                next_block.get_block_height(), _block_leaf.m_block_hash.hex().c_str(), 
                next_block.get_block_hash().hex().c_str()) ;
            return false;
        }
        return true;
    }
    //_block_leaf.m_chain_id =  next_block.get_chain_id();
    _block_leaf.m_block_hash = next_block.get_block_hash();
    _block_leaf.m_type = block_typ;
    m_tx_block_map.put(next_block.get_block_height(), _block_leaf);
    
    return true;    
}





bool xrelay_block_store::check_tx_block_validity(const xrelay_block &next_block)
{
    if (0 == next_block.get_block_height()) {
         return true;
    }
    
    if (!next_block.get_elections_sets().empty()) {
        xwarn("xrelay_block_store:check_tx_block_validity tx block has elections szie(%d) is error",
            next_block.get_elections_sets().size());
        return false;
    }
    
    if (next_block.get_all_transactions().empty() || next_block.get_all_receipts().empty()) {
        xwarn("xrelay_block_store:check_tx_block_validity tx block has tx_size(%d) and receipt_size(%d) is error",
            next_block.get_all_transactions().size(), next_block.get_all_receipts().size());
        return false;
    }
    return true;
}

void xrelay_block_store::clear_cache()
{
    m_tx_block_map.clear();
}

NS_END2


