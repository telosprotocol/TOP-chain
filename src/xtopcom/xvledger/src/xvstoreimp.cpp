// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "../xvdbstore.h"
#include "../xvblockstore.h"
#include "../xvcontractstore.h"
#include "../xvtxstore.h"
#include "../xveventbus.h"
#include "../xvledger.h"
#include "../xvdbkey.h"
#include "../xvcontract.h"
#include "xmetrics/xmetrics.h"

namespace top
{
    namespace base
    {
        //----------------------------------------xvdbstore_t----------------------------------------//
        xvdbstore_t::xvdbstore_t()
            :xobject_t((enum_xobject_type)enum_xobject_type_vxdbstore)
        {
        }

        xvdbstore_t::~xvdbstore_t()
        {
        };

        //caller need to cast (void*) to related ptr
        void*   xvdbstore_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(_enum_xobject_type_ == enum_xobject_type_vxdbstore)
                return this;

            return xobject_t::query_interface(_enum_xobject_type_);
        }

        //----------------------------------------xvtxstore_t----------------------------------------//
        xvtxstore_t::xvtxstore_t()
            :xobject_t((enum_xobject_type)enum_xobject_type_vtxstore)
        {
        }

        xvtxstore_t::~xvtxstore_t()
        {
        };

        //caller need to cast (void*) to related ptr
        void*   xvtxstore_t::query_interface(const int32_t _enum_xobject_type_)
        {
            xassert(false);
            if(_enum_xobject_type_ == enum_xobject_type_vtxstore)
                return this;

            return xobject_t::query_interface(_enum_xobject_type_);
        }

        bool  xvtxstore_t::store_tx_bin(const std::string & raw_tx_hash,const std::string & raw_tx_bin)
        {
            xassert(false);
            xassert(raw_tx_hash.empty() == false);
            xassert(raw_tx_bin.empty() == false);
            if(raw_tx_hash.empty() || raw_tx_bin.empty())
                return false;

            const std::string raw_tx_key = xvdbkey_t::create_tx_key(raw_tx_hash);
            const std::string existing_value = xvchain_t::instance().get_xdbstore()->get_value(raw_tx_key) ;
            if(existing_value == raw_tx_bin) //has stored already
                return true;

            //replace by new one
            XMETRICS_GAUGE(metrics::store_tx_origin, 1);
            return xvchain_t::instance().get_xdbstore()->set_value(raw_tx_key,raw_tx_bin);
        }

        const std::string xvtxstore_t::load_tx_bin(const std::string & raw_tx_hash)
        {
            xassert(false);
            xassert(raw_tx_hash.empty() == false);
            if(raw_tx_hash.empty())
                return std::string();

            const std::string raw_tx_key = xvdbkey_t::create_tx_key(raw_tx_hash);
            xdbg("xvtxstore_t::load_tx_bin, to load raw tx bin for (%s)",base::xstring_utl::to_hex(raw_tx_key).c_str());
            return xvchain_t::instance().get_xdbstore()->get_value(raw_tx_key);
        }

        bool   xvtxstore_t::store_tx_obj(const std::string & raw_tx_hash,xdataunit_t * raw_tx_obj)
        {
            xassert(false);
            xassert(raw_tx_hash.empty() == false);
            if( raw_tx_hash.empty() || (raw_tx_obj == NULL) ) {
                xdbg("xvtxstore_t::store_tx_obj, null tx hash or tx obj %s",base::xstring_utl::to_hex(raw_tx_hash).c_str());
                return false;
            }

            //check whether has stored
            const std::string raw_tx_key = xvdbkey_t::create_tx_key(raw_tx_hash);
            std::string raw_tx_bin;
            raw_tx_obj->serialize_to_string(raw_tx_bin);

            const std::string existing_value = xvchain_t::instance().get_xdbstore()->get_value(raw_tx_key) ;
            if(existing_value == raw_tx_bin) //nothing changed
                return true;

            xdbg("xvtxstore_t::store_tx_obj,%s",base::xstring_utl::to_hex(raw_tx_key).c_str());
            //replace by new one
            XMETRICS_GAUGE(metrics::store_tx_origin, 1);
            return xvchain_t::instance().get_xdbstore()->set_value(raw_tx_key,raw_tx_bin);
        }

        xauto_ptr<xdataunit_t>  xvtxstore_t::load_tx_obj(const std::string & raw_tx_hash)
        {
            xassert(false);
            const std::string raw_tx_bin = load_tx_bin(raw_tx_hash);
            if(raw_tx_bin.empty())
            {
                xwarn("xvtxstore_t::load_tx_obj,fail to load raw tx bin for hash(%s)",base::xstring_utl::to_hex(raw_tx_hash).c_str());
                return nullptr;
            }
            return xdataunit_t::read_from(raw_tx_bin);
        }

        bool     xvtxstore_t::store_txs(xvblock_t * block_ptr,bool store_raw_tx_bin)
        {
            xassert(false);
            xassert(block_ptr != NULL);
            if(NULL == block_ptr)
                return false;

            if(block_ptr->get_block_class() == enum_xvblock_class_nil) //nothing to store
                return true;

            std::vector<xobject_ptr_t<xvtxindex_t>> sub_txs;
            if(block_ptr->extract_sub_txs(sub_txs))
            {
                bool  has_error = false;
                std::map<std::string,int> counting_stored_raw_txs;
                for(auto & v : sub_txs)
                {
                    if(store_raw_tx_bin)
                    {
                        if(counting_stored_raw_txs.find(v->get_tx_hash()) == counting_stored_raw_txs.end())
                        {
                          if(store_tx_obj(v->get_tx_hash(), v->get_tx_obj()))
                              counting_stored_raw_txs[v->get_tx_hash()] = 1;
                        }
                    }
                    base::enum_txindex_type txindex_type = base::xvtxkey_t::transaction_subtype_to_txindex_type(v->get_tx_phase_type());
                    const std::string tx_key = xvdbkey_t::create_tx_index_key(v->get_tx_hash(), txindex_type);
                    std::string tx_bin;
                    v->serialize_to_string(tx_bin);
                    xassert(!tx_bin.empty());

                    if (v->get_tx_phase_type() == enum_transaction_subtype_send) {
                        XMETRICS_GAUGE(metrics::store_tx_index_send, 1);
                    } else if (v->get_tx_phase_type() == enum_transaction_subtype_recv) {
                        XMETRICS_GAUGE(metrics::store_tx_index_recv, 1);
                    } else if (v->get_tx_phase_type() == enum_transaction_subtype_self) {
                        XMETRICS_GAUGE(metrics::store_tx_index_self, 1);
                    } else if (v->get_tx_phase_type() == enum_transaction_subtype_confirm) {
                        XMETRICS_GAUGE(metrics::store_tx_index_confirm, 1);
                    }

                    if(base::xvchain_t::instance().get_xdbstore()->set_value(tx_key, tx_bin) == false)
                    {
                        xerror("xvtxstore_t::store_txs_index,fail to store tx for block(%s)",block_ptr->dump().c_str());
                        has_error = false; //mark it but let do rest work
                    }
                    else
                    {
                        xinfo("xvtxstore_t::store_txs_index,store tx to DB for block=%s,tx=%s",
                            block_ptr->dump().c_str(), base::xvtxkey_t::transaction_hash_subtype_to_string(v->get_tx_hash(), v->get_tx_phase_type()).c_str());
                    }

#ifdef  LONG_CONFIRM_CHECK
                    if (v->get_tx_phase_type() == enum_transaction_subtype_confirm) {
                        base::xauto_ptr<base::xvtxindex_t> send_txindex = base::xvchain_t::instance().get_xtxstore()->load_tx_idx(v->get_tx_hash(), base::enum_transaction_subtype_send);
                        if (send_txindex == nullptr)
                        {
                            xwarn("xvtxstore_t::store_txs,fail find sendtx index. tx=%s",base::xstring_utl::to_hex(v->get_tx_hash()).c_str());
                        }
                        else
                        {
                            uint64_t confirmtx_clock = block_ptr->get_clock();
                            uint64_t sendtx_clock = send_txindex->get_block_clock();
                            uint64_t delay_time = confirmtx_clock > sendtx_clock ? confirmtx_clock - sendtx_clock : 0;
                            static std::atomic<uint64_t> max_time{0};
                            if (max_time < delay_time)
                            {
                                max_time = delay_time;
                            }

                            if (delay_time >= 6)  // 6 clock
                            {
                                xwarn("xvtxstore_t::store_txs,confirm tx time long.max_time=%ld,time=%ld,tx=%s", (uint64_t)max_time, delay_time, base::xstring_utl::to_hex(v->get_tx_hash()).c_str());
                            }
                        }
                    }
#endif
                }
                if(has_error)
                    return false;
                return true;
            }
            else
            {
                xerror("xvtxstore_t::store_txs_index,fail to extract subtxs for block(%s)",block_ptr->dump().c_str());
                return false;
            }
        }

        xauto_ptr<xvtxindex_t> xvtxstore_t::load_tx_idx(const std::string & raw_tx_hash,enum_transaction_subtype type)
        {
            xassert(false);
            base::enum_txindex_type txindex_type = base::xvtxkey_t::transaction_subtype_to_txindex_type(type);
            const std::string tx_idx_key = xvdbkey_t::create_tx_index_key(raw_tx_hash, txindex_type);
            const std::string tx_idx_bin = base::xvchain_t::instance().get_xdbstore()->get_value(tx_idx_key);
            if(tx_idx_bin.empty())
            {
                xwarn("xvtxstore_t::load_tx_idx,index not find for hahs_tx=%s", base::xstring_utl::to_hex(raw_tx_hash).c_str());
                return nullptr;
            }
            xauto_ptr<xvtxindex_t> txindex(new xvtxindex_t());
            if(txindex->serialize_from_string(tx_idx_bin) <= 0)
            {
                xerror("xvtxstore_t::load_tx_idx,found bad index for hahs_tx=%s", base::xstring_utl::to_hex(raw_tx_hash).c_str());
                return nullptr;
            }
            return txindex;
        }

        void xvtxstore_t::update_node_type(uint32_t combined_node_type) {
            xassert(false);
        }

        //----------------------------------------xvblockstore_t----------------------------------------//
        xvblockstore_t::xvblockstore_t(base::xcontext_t & _context,const int32_t target_thread_id)
            :xiobject_t(_context,target_thread_id,(enum_xobject_type)enum_xobject_type_vblockstore)
        {
            xvblock_t::register_object(xcontext_t::instance()); //should only have one xvblockstore_t per process
        }

        xvblockstore_t::~xvblockstore_t()
        {
        };

        //caller need to cast (void*) to related ptr
        void*   xvblockstore_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(_enum_xobject_type_ == enum_xobject_type_vblockstore)
                return this;

            return xiobject_t::query_interface(_enum_xobject_type_);
        }
        //only allow remove flag within xvblockstore_t
        void  xvblockstore_t::remove_block_flag(xvblock_t* to_block, enum_xvblock_flag flag)
        {
            if(to_block != NULL)
                to_block->remove_block_flag(flag);
        }

        //----------------------------------------xvcontractstore_t----------------------------------------//
        xvcontractstore_t::xvcontractstore_t()
            :xobject_t((enum_xobject_type)enum_xobject_type_vcontractstore)
        {
        }

        xvcontractstore_t::~xvcontractstore_t()
        {
        }

        //caller need to cast (void*) to related ptr
        void*   xvcontractstore_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(_enum_xobject_type_ == enum_xobject_type_vcontractstore)
                return this;

            return xobject_t::query_interface(_enum_xobject_type_);
        }

        const std::string        xvcontractstore_t::get_sys_tep0_contract_uri(const std::string & contract_addr,const uint32_t contract_version)
        {
            return xvcontract_t::create_contract_uri(contract_addr,const_TEP0_contract_name, contract_version);
        }

        const std::string        xvcontractstore_t::get_sys_tep1_contract_uri(const std::string & contract_addr,const uint32_t contract_version)
        {
            return xvcontract_t::create_contract_uri(contract_addr,const_TEP1_contract_name, contract_version);
        }

        const std::string        xvcontractstore_t::get_sys_tep2_contract_uri(const std::string & contract_addr,const uint32_t contract_version)
        {
            return xvcontract_t::create_contract_uri(contract_addr,const_TEP2_contract_name, contract_version);
        }

        xauto_ptr<xvcontract_t>  xvcontractstore_t::get_sys_tep0_contract(const std::string & contract_addr,const uint32_t contract_version)
        {
            return new xvcontract_TEP0(contract_addr,contract_version);
        }

        xauto_ptr<xvcontract_t>  xvcontractstore_t::get_sys_tep1_contract(const std::string & contract_addr,const uint32_t contract_version)
        {
            return new xvcontract_TEP1(contract_addr,contract_version);
        }

        xauto_ptr<xvcontract_t>  xvcontractstore_t::get_sys_tep2_contract(const std::string & contract_addr,const uint32_t contract_version)
        {
            return new xvcontract_TEP2(contract_addr,contract_version);
        }

        xauto_ptr<xvcontract_t>  xvcontractstore_t::get_sys_contract(const std::string & contract_uri)
        {
            std::string contract_addr;
            std::string contract_name;
            uint32_t    contract_ver = 0;
            if(false == xvcontract_t::parse_contract_uri(contract_uri,contract_addr,contract_name,contract_ver))
            {
                xerror("xvcontractstore_t::get_sys_contract,bad uri(%s)",contract_uri.c_str());
                return nullptr;
            }
            return get_sys_contract(contract_addr,contract_name,contract_ver);
        }

        xauto_ptr<xvcontract_t>  xvcontractstore_t::get_sys_contract(const std::string & contract_addr,const std::string & contract_name,const uint32_t version)
        {
            if(contract_name.empty() == false)
            {
                if(contract_name == const_TEP0_contract_name)
                    return get_sys_tep0_contract(contract_addr,version);
                else if(contract_name == const_TEP1_contract_name)
                    return get_sys_tep1_contract(contract_addr,version);
                else if(contract_name == const_TEP2_contract_name)
                    return get_sys_tep2_contract(contract_addr,version);

                xerror("xvcontractstore_t::get_sys_contract,fail to load system contract for contract_addr(%s)/contract_name(%s)/version(%u)",contract_addr.c_str(),contract_name.c_str(),version);
                return nullptr;
            }
            else
            {
                xerror("xvcontractstore_t::get_sys_contract,try load non-system contract for contract_addr(%s)/contract_name(%s)/version(%u)",contract_addr.c_str(),contract_name.c_str(),version);
                return nullptr;
            }
        }

        xauto_ptr<xvcontract_t>  xvcontractstore_t::get_usr_contract(const std::string & contract_addr)
        {
            xerror("xvcontractstore_t::get_usr_contract,fail to load contract for contract_addr(%s)",contract_addr.c_str());
            return nullptr;
        }

        //universal api
        xauto_ptr<xvcontract_t>  xvcontractstore_t::get_contract(const std::string & contract_uri)
        {
            std::string contract_addr;
            std::string contract_name;
            uint32_t    contract_ver = 0;
            if(false == xvcontract_t::parse_contract_uri(contract_uri,contract_addr,contract_name,contract_ver))
            {
                xerror("xvcontractstore_t::get_contract,bad uri(%s)",contract_uri.c_str());
                return nullptr;
            }
            if(contract_name.empty() == false)
                return get_sys_contract(contract_addr,contract_name,contract_ver);
            else
                return get_usr_contract(contract_addr);
        }

        //----------------------------------------xveventbus_t----------------------------------------//
        xveventbus_t::xveventbus_t()
            :xobject_t(enum_xevent_route_path_by_mbus)
        {
        }
        xveventbus_t::~xveventbus_t()
        {
        }

         void*   xveventbus_t::query_interface(const int32_t type)
         {
             if(type == enum_xobject_type_veventbus)
                 return this;

             return xobject_t::query_interface(type);
         }

        bool   xveventbus_t::handle_event(const xvevent_t & ev)
        {
            if(ev.get_type() == enum_xevent_route_path_by_mbus)
            {
                auto * ev_ptr = const_cast<xvevent_t *>(&ev);
                ev_ptr->add_ref();
                mbus::xevent_ptr_t mbus_ev_ptr;
                mbus_ev_ptr.attach(dynamic_cast<mbus::xevent_t *>(ev_ptr));

                push_event(mbus_ev_ptr);
                return true;
            }
            return false;
        }

    };//end of namespace of base
};//end of namespace of top
