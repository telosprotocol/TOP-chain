// Copyright (c) 2018-Present Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xutl.h"
#include "xunitstore.h"
#include "xvledger/xvbindex.h"
#include "xdata/xblocktool.h"

namespace top
{
    namespace store
    {
        xunitstore_t::xunitstore_t(xvblockdb_t* blockdb)
        : m_blockdb_ptr(blockdb) {
        }

        bool xunitstore_t::store_units(base::xvblock_t* table_block, std::vector<base::xvblock_ptr_t> const& units) {
            assert(table_block->check_block_flag(base::enum_xvblock_flag_committed));
            // TODO(jimmy)  bindex serialize size optimize
            xinfo("xunitstore_t::store_units tps_key enter.%s,units_kvs=%zu", table_block->dump().c_str(), units.size());
            std::map<std::string, std::string> units_kvs;            
            for (auto & unit : units) {
                base::xauto_ptr<base::xvbindex_t > new_idx = new base::xvbindex_t(*unit.get());
                new_idx->set_block_flag(base::enum_xvblock_flag_authenticated);
                new_idx->set_block_flag(base::enum_xvblock_flag_locked);
                new_idx->set_block_flag(base::enum_xvblock_flag_committed);
                new_idx->set_store_flag(base::enum_index_store_flag_main_entry);
                new_idx->set_block_flag(base::enum_xvblock_flag_stored);// TODO(jimmy)
                new_idx->set_unitblock_on_index(unit.get());

                const std::string key_path = base::xvdbkey_t::create_prunable_block_index_key(*new_idx, new_idx->get_height());
                std::string index_bin;
                new_idx->serialize_to(index_bin);
                units_kvs[key_path] = index_bin;
                xdbg_info("xunitstore_t::store_units store_block,done unit=%s,size=%zu",unit->dump().c_str(),index_bin.size());
            }
            xinfo("xunitstore_t::store_units tps_key finish kvs %s,units_kvs=%zu", table_block->dump().c_str(), units_kvs.size());
            bool ret = m_blockdb_ptr->get_xdbstore()->set_values(units_kvs);
            xinfo("xunitstore_t::store_units tps_key finish store %s,units_kvs=%zu", table_block->dump().c_str(), units_kvs.size());
            return ret;
        }
        bool xunitstore_t::store_units(base::xvblock_t* table_block) {
            xassert(table_block->check_block_flag(base::enum_xvblock_flag_committed));
            std::vector<xobject_ptr_t<base::xvblock_t>> sub_blocks;  
            if (table_block->get_block_class() != base::enum_xvblock_class_light) {
                return true;
            }
            if (false == table_block->extract_sub_blocks(sub_blocks)) {
                xerror("xunitstore_t::store_units,fail-extract_sub_blocks for table block(%s)", table_block->dump().c_str());
                return false;                   
            }
            return store_units(table_block, sub_blocks);
        }
        bool xunitstore_t::store_unit(const base::xvaccount_t & account,base::xvblock_t* unit) {
            xassert(unit->get_height() == 0);
            xassert(account.is_unit_address());
            base::xauto_ptr<base::xvbindex_t > new_idx = new base::xvbindex_t(*unit);
            new_idx->set_block_flag(base::enum_xvblock_flag_authenticated);
            new_idx->set_block_flag(base::enum_xvblock_flag_locked);
            new_idx->set_block_flag(base::enum_xvblock_flag_committed);
            new_idx->set_store_flag(base::enum_index_store_flag_main_entry);
            new_idx->set_block_flag(base::enum_xvblock_flag_stored);// TODO(jimmy)
            new_idx->set_unitblock_on_index(unit);

            const std::string key_path = base::xvdbkey_t::create_prunable_block_index_key(account, new_idx->get_height());
            std::string index_bin;
            new_idx->serialize_to(index_bin);            
            bool ret = m_blockdb_ptr->get_xdbstore()->set_value(key_path, index_bin);
            xinfo("xunitstore_t::store_unit store_block,done %s,size=%zu", unit->dump().c_str(),index_bin.size());
            return ret;
        }
        bool xunitstore_t::exist_unit(const base::xvaccount_t & account) const {
            std::vector<base::xvbindex_t*> _indexes = m_blockdb_ptr->read_index_from_db(account, 0);
            for (auto & _index : _indexes) {
                _index->release_ref();
            }
            return _indexes.size() > 0;
        }

        base::xauto_ptr<base::xvblock_t> load_unit_by_condition(xvblockdb_t* blockdb_ptr, const base::xvaccount_t & account, const uint64_t height, std::function<bool(base::xvbindex_t* bindex)> _match_func) {
            std::vector<base::xvbindex_t*> _indexes = blockdb_ptr->read_index_from_db(account, height);
            xobject_ptr_t<base::xvbindex_t> match_index = nullptr;
            for (auto & _index : _indexes) {
                if (match_index == nullptr && _match_func(_index)) {
                    match_index.attach(_index);                    
                } else {
                    _index->release_ref();
                }
            }
            if (match_index != nullptr) {
                xobject_ptr_t<base::xvblock_t> new_block_ptr = nullptr;
                if (match_index->check_store_flag(base::enum_index_store_flag_unit_in_index)) {// unit in bindex mode
                    new_block_ptr = match_index->create_unitblock_on_index();
                } else {
                    if (false == blockdb_ptr->load_block_object(match_index.get())) {
                        return nullptr;
                    }
                    if (false == blockdb_ptr->load_block_input(match_index.get()) || false == blockdb_ptr->load_block_output(match_index.get())) {
                        return nullptr;
                    }
                    match_index->get_this_block()->add_ref();
                    new_block_ptr =  match_index->get_this_block();
                }
                if (nullptr == new_block_ptr) {
                    xerror("xunitstore_t::load_unit_by_condition fail load block by index.%s",match_index->dump().c_str());
                } else {
                    xdbg_info("xunitstore_t::load_unit_by_condition succ %s,height=%ld",account.get_account().c_str(),height);
                }                
                return new_block_ptr;
            }
            xwarn("xunitstore_t::load_unit_by_condition fail %s,height=%ld",account.get_account().c_str(),height);
            return nullptr;
        }

        base::xauto_ptr<base::xvblock_t> xunitstore_t::load_unit(const base::xvaccount_t & account,const uint64_t height,const std::string & blockhash) {
            auto f = [&blockhash](base::xvbindex_t* _index) -> bool { 
                return _index->get_block_hash() == blockhash;
            };
            xdbg("xunitstore_t::load_unit %s,height=%ld,hash=%s",account.get_account().c_str(),height,base::xstring_utl::to_hex(blockhash).c_str());
            return load_unit_by_condition(m_blockdb_ptr, account, height, f);
        }
        base::xauto_ptr<base::xvblock_t> xunitstore_t::load_unit(const base::xvaccount_t & account,const uint64_t height) {
            auto f = [](base::xvbindex_t* _index) -> bool { 
                return _index->check_block_flag(base::enum_xvblock_flag_committed);
            };
            xdbg("xunitstore_t::load_unit %s,height=%ld",account.get_account().c_str(),height);
            return load_unit_by_condition(m_blockdb_ptr, account, height, f);
        }
        base::xauto_ptr<base::xvblock_t> xunitstore_t::load_unit(const base::xvaccount_t & account,const uint64_t height,const uint64_t viewid) {
            auto f = [&viewid](base::xvbindex_t* _index) -> bool { 
                return _index->get_viewid() == viewid;
            };
            xdbg("xunitstore_t::load_unit %s,height=%ld,viewid=%ld",account.get_account().c_str(),height,viewid);
            return load_unit_by_condition(m_blockdb_ptr, account, height, f);
        }
        bool xunitstore_t::delete_unit(const base::xvaccount_t & account,base::xvblock_t* block) {
            std::vector<base::xvbindex_t*> _indexes = m_blockdb_ptr->read_index_from_db(account, block->get_height());
            xobject_ptr_t<base::xvbindex_t> match_index = nullptr;
            for (auto & _index : _indexes) {
                if (match_index == nullptr && _index->get_block_hash() == block->get_block_hash()) {
                    match_index.attach(_index);
                } else {
                    _index->release_ref();
                }
            }
            if (nullptr != match_index) {
                m_blockdb_ptr->delete_block(match_index.get());
            }
            return  true;
        }

    }
}