// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "../xvcnode.h"

#ifdef DEBUG
#include "xcrypto/xckey.h"
#endif
#include "xpbase/base/top_utils.h"
#include "xmetrics/xmetrics.h"

namespace top
{
    namespace base
    {
        xvnode_t::xvnode_t(const std::string & account,const xvip2_t & xip2_addr,const std::string & sign_pub_key)
        {
            m_account = account;
            m_sign_pubkey             = sign_pub_key;
            m_node_address.high_addr  = xip2_addr.high_addr;
            m_node_address.low_addr   = xip2_addr.low_addr;
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvnode_t, 1);
// #ifdef DEBUG  // double check whether public key matched the account addresss
//             if (!sign_pub_key.empty()) { // unit test might to use empty keys.
//                 utl::xecpubkey_t pub_key((uint8_t *)sign_pub_key.data(), (int)sign_pub_key.size());
//                 xassert(account == pub_key.to_address(get_addr_type(), get_ledger_id()));
//             }
// #endif
        }
        
        xvnode_t::xvnode_t(const xvnode_t & obj)
        {
            m_sign_pubkey = obj.m_sign_pubkey;
            
            m_node_address.high_addr  = obj.m_node_address.high_addr;
            m_node_address.low_addr   = obj.m_node_address.low_addr;
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvnode_t, 1);
        }
        
        xvnode_t::~xvnode_t()
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvnode_t, -1);            
        }
        
        xvnodegroup_t::xvnodegroup_t(const xvip2_t & group_address,const uint64_t effect_clock_height,std::vector<xvnode_t*> const & nodes)
        {
            m_group_address.high_addr = group_address.high_addr;
            m_group_address.low_addr  = group_address.low_addr;
            m_start_clock_height      = effect_clock_height;
            m_network_height          = get_network_height_from_xip2(m_group_address);
            
            m_nodes.resize(get_group_nodes_count_from_xip2(group_address));
            for(auto it : nodes)
            {
                if(it != NULL)
                {
                    if(is_xip2_group_equal(m_group_address,it->get_xip2_addr()))
                    {
                        const uint32_t node_index = get_node_id_from_xip2(it->get_xip2_addr());
                        if(node_index < get_group_nodes_count_from_xip2(group_address))
                        {
                            it->add_ref();
                            m_nodes[node_index] = it;
                        }
                        else
                        {
                            xdbgassert(0);
                        }
                    }
                    else
                    {
                        xdbgassert(0);
                    }
                }
            }
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvnodegroup, 1);
        }
        
        xvnodegroup_t::xvnodegroup_t(const xvip2_t & group_address,const uint64_t effect_clock_height,std::deque<xvnode_t*> const & nodes)
        {
            m_group_address.high_addr = group_address.high_addr;
            m_group_address.low_addr  = group_address.low_addr;
            m_start_clock_height      = effect_clock_height;
            m_network_height          = get_network_height_from_xip2(m_group_address);
            
            m_nodes.resize(get_group_nodes_count_from_xip2(group_address));
            for(auto it : nodes)
            {
                if(it != NULL)
                {
                    if(is_xip2_group_equal(m_group_address,it->get_xip2_addr()))
                    {
                        const uint32_t node_slot = get_node_id_from_xip2(it->get_xip2_addr());
                        if(node_slot < get_group_nodes_count_from_xip2(group_address))
                        {
                            it->add_ref();
                            m_nodes[node_slot] = it;
                        }
                        else
                        {
                            xdbgassert(0);
                        }
                    }
                    else
                    {
                        xdbgassert(0);
                    }
                }
            }
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvnodegroup, 1);
        }
        
        xvnodegroup_t::~xvnodegroup_t()
        {
            for(auto it : m_nodes)
            {
                if(it != NULL)
                {
                    it->release_ref();
                }
            }
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvnodegroup, -1);
        }
        
        xvnode_t* xvnodegroup_t::get_node(const xvip2_t & target_node_xip2) const
        {
            const uint32_t node_slot = get_node_id_from_xip2(target_node_xip2);
            xvnode_t * target_node_obj = get_node(node_slot);
            if(NULL != target_node_obj)
            {
                if(is_xip2_equal(target_node_xip2,target_node_obj->get_xip2_addr()))
                    return target_node_obj;
                
                xwarn_err("xvnodegroup_t::get_node,fail-find target node by xip2{% " PRIu64 " : % " PRIu64 " } at gruop{% " PRIu64 " : % " PRIu64 " }",target_node_xip2.high_addr,target_node_xip2.low_addr,m_group_address.high_addr,m_group_address.low_addr);
            }
            return NULL;
        }
        
        xvnode_t* xvnodegroup_t::get_node(const uint32_t node_slot) const
        {
            if(node_slot < get_group_nodes_count_from_xip2(m_group_address))
            {
                xvnode_t* _target_node = m_nodes[node_slot];
                return _target_node;
            }
            xwarn_err("xvnodegroup_t::get_node,fail-find target node by node_slot=%u vs nodes_count=%u",node_slot,get_size());
            return NULL;
        }
        
        xvnodesrv_t::xvnodesrv_t()
            :xdataobj_t((enum_xdata_type)enum_xobject_type_vnodesvr)
        {
        }
        
        xvnodesrv_t::xvnodesrv_t(enum_xdata_type type)
            :xdataobj_t(type)
        {
        }
        
        xvnodesrv_t::~xvnodesrv_t()
        {
        }
        
        void*  xvnodesrv_t::query_interface(const int32_t _enum_xobject_type_)//caller need to cast (void*) to related ptr
        {
            if(_enum_xobject_type_ == enum_xobject_type_vnodesvr)
                return this;
           
            return xdataobj_t::query_interface(_enum_xobject_type_);
        }
        
        int32_t   xvnodesrv_t::do_write(base::xstream_t & stream)//write whole object to binary
        {
            return 0;
        }
        int32_t   xvnodesrv_t::do_read(base::xstream_t & stream) //read from binary and regeneate content
        {
            return 0;
        }
        
        xvnodehouse_t::xvnodehouse_t()
            :xvnodesrv_t()
        {
        }
        xvnodehouse_t::xvnodehouse_t(enum_xdata_type type)
            :xvnodesrv_t(type)
        {
        }
        xvnodehouse_t::~xvnodehouse_t()
        {
            m_lock.lock();
            for(auto it = m_vgroups.begin(); it != m_vgroups.end(); ++it)
            {
                if(it->second != nullptr)
                    it->second->release_ref();
            }
            m_vgroups.clear();
            m_lock.unlock();
        }
 
        xauto_ptr<xvnode_t>        xvnodehouse_t::get_node(const xvip2_t & target_node) const
        {
            //GroupKey = [elect-height:21bit][xnetwork-id: 7-7-7 bit][zone-id:7bit|cluster-id:7bit|group-id:8bit]
            const uint64_t group_key = ((target_node.low_addr << 11) >> 21) | ((target_node.high_addr & 0x1FFFFF) << 43);
            
            std::lock_guard<std::mutex> locker(m_lock);
            auto it = m_vgroups.find(group_key);
            if(it != m_vgroups.end())
            {
                xvnode_t * node_ptr = it->second->get_node(target_node);
                if(node_ptr != NULL)
                    node_ptr->add_ref();
                
                return node_ptr;
            }
            return nullptr;
        }
        
        /*
        XIP definition as total 64bit = [xaddress_domain:1bit | xaddress_type:2bit | xnetwork_type:5bit] [xnetwork_version#:3bit][xnetwork-id: 7-7-7 bit][xhost-id:32bit] =
        {
            //xaddress_domain is    enum_xaddress_domain_xip(0) or  enum_xaddress_domain_xip2(1)
            //xaddress_type   is    enum_xip_type
            //xnetwork_type         refer enum_xnetwork_type
            -[enum_xaddress_domain_xip:1bit | enum_xip_type:2bit | xnetwork_type:5bit]
            -[xnetwork_version#:3bit] //elect round# at Chain
            -[xnetwork-id: 7-7-7 bit] //A-Class,B-Class,C-Class,D-Class,E-Class Network...
            -[zone-id:7bit|cluster-id:7bit|group-id:8bit|node-id:10bit]
        }        
        //XIP2 is 128bit address like IPv6 design on top of XIP
        XIP2 = [high 64bit:label data][low 64bit:XIP]
        {
            high 64bit for enum_xnetwork_type_xchain
                -[xgroup_node_count:10bit]
                -[xnetwork_round/height:54bit]
       
            low  64bit:
                -[XIP: 64bit]
        }
        */
        xauto_ptr<xvnodegroup_t>   xvnodehouse_t::get_group(const xvip2_t & target_group) const
        {
            //GroupKey = [elect-height:21bit][xnetwork-id: 7-7-7 bit][zone-id:7bit|cluster-id:7bit|group-id:8bit]
            const uint64_t group_key = ((target_group.low_addr << 11) >> 21) | ((target_group.high_addr & 0x1FFFFF) << 43);
           
            std::lock_guard<std::mutex> locker(m_lock);
            auto it = m_vgroups.find(group_key);
            if(it != m_vgroups.end())
            {
                if(it->second->get_network_height() == get_network_height_from_xip2(target_group)) //double check exactly height
                {
                    it->second->add_ref();
                    return it->second;
                }
            }
            return nullptr;
        }
                
        bool    xvnodehouse_t::add_group(const xvnodegroup_t* group_ptr)
        {
            if(NULL == group_ptr)
                return false;
            
            //GroupKey = [elect-height:21bit][xnetwork-id: 7-7-7 bit][zone-id:7bit|cluster-id:7bit|group-id:8bit]
            const uint64_t group_key = ((group_ptr->get_xip2_addr().low_addr << 11) >> 21) | ((group_ptr->get_xip2_addr().high_addr & 0x1FFFFF) << 43);
            ((xvnodegroup_t*)group_ptr)->add_ref(); //add reference first
            std::lock_guard<std::mutex> locker(m_lock);
            auto it = m_vgroups.emplace(group_key,(xvnodegroup_t*)group_ptr);
            if(false == it.second) //foud existing one
            {
                xvnodegroup_t * old_ptr = it.first->second;
                it.first->second = (xvnodegroup_t*)group_ptr; //replace old one
                old_ptr->release_ref();
            }
            return true;
        }
        
        bool    xvnodehouse_t::remove_group(const xvip2_t & target_group)
        {
            //GroupKey = [elect-height:21bit][xnetwork-id: 7-7-7 bit][zone-id:7bit|cluster-id:7bit|group-id:8bit]
            const uint64_t group_key = ((target_group.low_addr << 11) >> 21) | ((target_group.high_addr & 0x1FFFFF) << 43);
            std::lock_guard<std::mutex> locker(m_lock);
            auto it = m_vgroups.find(group_key);
            if(it != m_vgroups.end())
            {
                it->second->release_ref();
                m_vgroups.erase(it);
                return true;
            }
            return false;
        }
    };//end of namespace of base
};//end of namespace of top
