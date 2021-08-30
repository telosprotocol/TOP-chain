// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../xvtblconnect.h"
#include "xbase/xcontext.h"
#include "xbase/xutl.h"

namespace top
{
    namespace base
    {
        xtblvend_t::xtblvend_t(const uint16_t table_id,const uint64_t send_tx_id,const uint64_t recv_tx_id,const uint64_t confirm_tx_id)
        {
            m_table_id       = table_id;
            m_send_tx_id     = send_tx_id;
            m_recv_tx_id     = recv_tx_id;
            m_confrm_tx_id   = confirm_tx_id;
        }
    
        xtblvend_t::xtblvend_t(xtblvend_t && obj)
        {
            *this = obj;
        }
    
        xtblvend_t::xtblvend_t(const xtblvend_t & obj)
        {
            *this = obj;
        }
    
        xtblvend_t & xtblvend_t::operator = (const xtblvend_t & obj)
        {
            m_table_id      = obj.m_table_id;
            m_send_tx_id    = obj.m_send_tx_id;
            m_recv_tx_id    = obj.m_recv_tx_id;
            m_confrm_tx_id  = obj.m_confrm_tx_id;
            return *this;
        }
    
        xtblvend_t::~xtblvend_t()
        {
        }
    
        int32_t   xtblvend_t::serialize_to_string(std::string & bin_data)
        {
            base::xautostream_t<64> _stream(base::xcontext_t::instance());
            
            _stream << m_table_id;
            _stream << m_send_tx_id;
            _stream << m_recv_tx_id;
            _stream << m_confrm_tx_id;
            
            bin_data.assign((const char*)_stream.data(),_stream.size());
            return _stream.size();
        }
        
        int32_t   xtblvend_t::serialize_from_string(const std::string & bin_data) //wrap function fo serialize_from(stream)
        {
            base::xstream_t _stream(base::xcontext_t::instance(),(uint8_t*)bin_data.data(),(uint32_t)bin_data.size());
            
            _stream >> m_table_id;
            _stream >> m_send_tx_id;
            _stream >> m_recv_tx_id;
            _stream >> m_confrm_tx_id;

            return (int32_t)bin_data.size();
        }
        
        xtblpair_t::xtblpair_t(const uint16_t this_table_id,const uint16_t peer_table_id)
            :m_this_vend(this_table_id,0,0,0),
             m_peer_vend(peer_table_id,0,0,0)
        {
        }
        
        xtblpair_t::~xtblpair_t()
        {
        }
        
        void xtblpair_t::update_this(xtblvend_t & new_val)
        {
             m_this_vend = new_val;
        }
    
        void xtblpair_t::update_peer(xtblvend_t & new_val)
        {
            m_peer_vend = new_val;
        }
    
        //----------------------------------xtblvconn_t------------------------------------//
        //each xtableobj_t have only one xtblvconn_t
        xtblvconn_t::xtblvconn_t(xvtblnet_t & vnet,const uint16_t owner_table_id)//which table own this vnet
            :m_table_vnet(vnet)
        {
            m_owner_table_id = owner_table_id;
            memset(m_peer_paris,0,sizeof(m_peer_paris));
            xassert(owner_table_id < enum_vbucket_has_tables_count);
        }
     
        xtblvconn_t::~xtblvconn_t()
        {
            for(int i = 0; i < enum_vbucket_has_tables_count; ++i)
            {
                xtblpair_t * pair_tpr = xatomic_t::xexchange(m_peer_paris[i],(xtblpair_t*)NULL);;
                if(pair_tpr != NULL)
                    delete pair_tpr;
            }
        }
    
        xtblpair_t*  xtblvconn_t::get_pair(const uint16_t peer_table_id)
        {
            if(peer_table_id >= enum_vbucket_has_tables_count)
            {
                xassert(0);
                return NULL;
            }
            xtblpair_t * pair_ptr = m_peer_paris[peer_table_id];
            if(pair_ptr != NULL)
                return pair_ptr;
            
            m_table_vnet.get_lock(); //lock first
            pair_ptr = m_peer_paris[peer_table_id];//then test again
            if(NULL == pair_ptr)
            {
                pair_ptr = new xtblpair_t(get_owner_table_id(),peer_table_id);
                m_peer_paris[peer_table_id] = pair_ptr; //assign and store
            }
            m_table_vnet.get_lock().unlock();//final unlock before return
            
            return pair_ptr;
        }
 
        bool xtblvconn_t::update_my_window(xtblvend_t & window,const uint16_t paired_peer_table_id) //update self
        {
            if(paired_peer_table_id == get_owner_table_id())
            {
                xassert(0);
                return false;
            }
            if(paired_peer_table_id >= enum_vbucket_has_tables_count)
            {
                xassert(0);
                return NULL;
            }
            
            std::string vend_bin;
            window.serialize_to_string(vend_bin);
            
            xdatapdu_t window_update_pdu;
            window_update_pdu.reset_message(enum_vtable_msg_type_update_self_window,0,vend_bin,0,paired_peer_table_id,get_owner_table_id());
            return m_table_vnet.send_packet(window_update_pdu);
        }
    
        bool xtblvconn_t::send_window_update(const uint16_t dst_table_id)//send my endpoint to update peer
        {
            if(dst_table_id == get_owner_table_id())
            {
                xassert(0);
                return false;
            }
            
            xtblpair_t*  target_pair = get_pair(dst_table_id);
            if(target_pair == NULL)
            {
                xassert(target_pair != NULL);
                return false;
            }
            
            std::string vend_bin;
            m_table_vnet.get_lock().lock();
            target_pair->m_this_vend.serialize_to_string(vend_bin);
            m_table_vnet.get_lock().unlock();
            
            xdatapdu_t window_update_pdu;
            window_update_pdu.reset_message(enum_vtable_msg_type_update_peer_window,0,vend_bin,0,get_owner_table_id(),dst_table_id);
            return m_table_vnet.send_packet(window_update_pdu);
        }
        
        //pull receipt of request_send_tx_id from dst_table_id
        bool    xtblvconn_t::pull_sendreceipt(const uint16_t dst_table_id,const uint64_t request_send_tx_id)
        {
            xtblpair_t*  target_pair = get_pair(dst_table_id);
            if(target_pair == NULL)
            {
                xassert(target_pair != NULL);
                return false;
            }
            std::string my_vend_bin;
            m_table_vnet.get_lock().lock();
            target_pair->m_this_vend.serialize_to_string(my_vend_bin);//always carry information to peer end
            m_table_vnet.get_lock().unlock();
            
            xdatapdu_t request_receipt_pdu;
            request_receipt_pdu.reset_message(enum_vtable_msg_type_pull_sendreceipt,0,my_vend_bin, request_send_tx_id,get_owner_table_id(),dst_table_id);
            
            return m_table_vnet.send_packet(request_receipt_pdu);
        }
    
        //pull receipt of request_recv_tx_id from dst_table_id
        bool    xtblvconn_t::pull_recvreceipt(const uint16_t dst_table_id,const uint64_t request_recv_tx_id)
        {
            xtblpair_t*  target_pair = get_pair(dst_table_id);
            if(target_pair == NULL)
            {
                xassert(target_pair != NULL);
                return false;
            }
            std::string my_vend_bin;
            m_table_vnet.get_lock().lock();
            target_pair->m_this_vend.serialize_to_string(my_vend_bin);
            m_table_vnet.get_lock().unlock();
            
            xdatapdu_t request_receipt_pdu;
            request_receipt_pdu.reset_message(enum_vtable_msg_type_pull_recvreceipt,0,my_vend_bin, request_recv_tx_id,get_owner_table_id(),dst_table_id);
            
            return m_table_vnet.send_packet(request_receipt_pdu);
        }
    
        //pull receipt of request_confirm_tx_id from dst_table_id
        bool    xtblvconn_t::pull_confirmreceipt(const uint16_t dst_table_id,const uint64_t request_confirm_tx_id)
        {
            xtblpair_t*  target_pair = get_pair(dst_table_id);
            if(target_pair == NULL)
            {
                xassert(target_pair != NULL);
                return false;
            }
            std::string my_vend_bin;
            m_table_vnet.get_lock().lock();
            target_pair->m_this_vend.serialize_to_string(my_vend_bin);
            m_table_vnet.get_lock().unlock();
            
            xdatapdu_t request_receipt_pdu;
            request_receipt_pdu.reset_message(enum_vtable_msg_type_pull_confirmreceipt,0,my_vend_bin, request_confirm_tx_id,get_owner_table_id(),dst_table_id);
            
            return m_table_vnet.send_packet(request_receipt_pdu);
        }
    
        //push receipt of send_receipt_id to dst_table_id with receipt_bin
        bool    xtblvconn_t::push_sendreceipt(const uint16_t dst_table_id,const uint64_t send_receipt_id,const std::string & receipt_bin)
        {
            xdatapdu_t respond_receipt_pdu;
            respond_receipt_pdu.reset_message(enum_vtable_msg_type_update_sendreceipt,0,receipt_bin,send_receipt_id,get_owner_table_id(),dst_table_id);
            
            return m_table_vnet.send_packet(respond_receipt_pdu);
        }
    
        //push receipt of recv_receipt_id to dst_table_id with receipt_bin
        bool    xtblvconn_t::push_recvreceipt(const uint16_t dst_table_id,const uint64_t recv_receipt_id,const std::string & receipt_bin)
        {
            xdatapdu_t respond_receipt_pdu;
            respond_receipt_pdu.reset_message(enum_vtable_msg_type_update_recvreceipt,0,receipt_bin,recv_receipt_id,get_owner_table_id(),dst_table_id);
            
            return m_table_vnet.send_packet(respond_receipt_pdu);
        }
    
        //push receipt of confirm_receipt_id to dst_table_id with receipt_bin
        bool    xtblvconn_t::push_confirmreceipt(const uint16_t dst_table_id,const uint64_t confirm_receipt_id,const std::string & receipt_bin)
        {
            xdatapdu_t respond_receipt_pdu;
            respond_receipt_pdu.reset_message(enum_vtable_msg_type_update_confirmreceipt,0,receipt_bin,confirm_receipt_id,get_owner_table_id(),dst_table_id);
            
            return m_table_vnet.send_packet(respond_receipt_pdu);
        }
        
        //multiple thread safe,since vnet already locked it
        bool xtblvconn_t::on_packet_recv(const xdatapdu_t & pdu)
        {
            if(pdu.get_msg_to() != get_owner_table_id())
            {
                xassert(0); //wrong destnation
                return false;
            }
            
            switch(pdu.get_msg_type())
            {
            case enum_vtable_msg_type_update_self_window:
                {
                    xtblpair_t*  target_pair = get_pair(pdu.get_msg_from());
                    if(target_pair == NULL)
                    {
                        xassert(target_pair != NULL);
                        return false;
                    }
                    
                    xtblvend_t this_end(pdu.get_msg_to(),0,0,0);
                    this_end.serialize_from_string(pdu.get_msg_body());
                    if(  (this_end.get_table_id() != get_owner_table_id())
                       ||(this_end.get_send_tx_id() < target_pair->m_this_vend.get_send_tx_id())
                       ||(this_end.get_recv_tx_id() < target_pair->m_this_vend.get_recv_tx_id())
                       ||(this_end.get_confirm_tx_id() < target_pair->m_this_vend.get_confirm_tx_id())
                       )
                    {
                        xassert(0); //bad window
                        return false;
                    }
                    target_pair->update_this(this_end);//now safe to update local
                    
                    on_my_window_update(this_end,pdu.get_msg_from()); //fire event
                    return true;
                }
                break;
                
            case enum_vtable_msg_type_update_peer_window:
                {
                    xtblpair_t*  target_pair = get_pair(pdu.get_msg_from());
                    if(target_pair == NULL)
                    {
                        xassert(target_pair != NULL);
                        return false;
                    }
                    
                    xtblvend_t peer_end(pdu.get_msg_from(),0,0,0);
                    peer_end.serialize_from_string(pdu.get_msg_body());
                    if(  (peer_end.get_table_id() != pdu.get_msg_from())
                       ||(peer_end.get_send_tx_id() < target_pair->m_peer_vend.get_send_tx_id())
                       ||(peer_end.get_recv_tx_id() < target_pair->m_peer_vend.get_recv_tx_id())
                       ||(peer_end.get_confirm_tx_id() < target_pair->m_peer_vend.get_confirm_tx_id())
                       )
                    {
                        xassert(0); //bad window
                        return false;
                    }
                    target_pair->update_peer(peer_end);//now safe to update local
                    
                    on_peer_window_update(peer_end,get_owner_table_id());//fire event
                    return true;
                }
                break;
                
            case enum_vtable_msg_type_pull_sendreceipt:
                {
                    const std::string target_receipt = load_sendreceipt(get_owner_table_id(),pdu.get_msg_nonce());
                    return push_sendreceipt(pdu.get_msg_from(), pdu.get_msg_nonce(), target_receipt);
                }
                break;
                
             case enum_vtable_msg_type_pull_recvreceipt:
                {
                    const std::string target_receipt = load_recvreceipt(get_owner_table_id(),pdu.get_msg_nonce());
                    return push_recvreceipt(pdu.get_msg_from(), pdu.get_msg_nonce(), target_receipt);
                }
                break;
                
             case enum_vtable_msg_type_pull_confirmreceipt:
                {
                    const std::string target_receipt = load_confirmreceipt(get_owner_table_id(),pdu.get_msg_nonce());
                    return push_confirmreceipt(pdu.get_msg_from(), pdu.get_msg_nonce(), target_receipt);
                }
                break;
                
             case enum_vtable_msg_type_update_sendreceipt:
                {
                    const std::string receipt_bin = pdu.get_msg_body();
                    on_sendreceipt_update(receipt_bin, pdu.get_msg_nonce(), pdu.get_msg_from());//fire event
                    return true;
                }
                break;
                
             case enum_vtable_msg_type_update_recvreceipt:
                {
                    const std::string receipt_bin = pdu.get_msg_body();
                    on_recvreceipt_update(receipt_bin, pdu.get_msg_nonce(), pdu.get_msg_from());//fire event
                    return true;
                }
                break;
                
             case enum_vtable_msg_type_update_confirmreceipt:
                {
                    const std::string receipt_bin = pdu.get_msg_body();
                    on_confirmreceipt_update(receipt_bin, pdu.get_msg_nonce(), pdu.get_msg_from());//fire event
                    return true;
                }
                break;
            }
            return false;
        }
    
        bool xtblvconn_t::on_my_window_update(const xtblvend_t & self_window,const uint16_t paired_table_id)
        {
            return false;//not handled
        }
    
        bool xtblvconn_t::on_peer_window_update(const xtblvend_t & peer_window,const uint16_t paired_table_id)
        {
            return false;//not handled
        }
        
        bool    xtblvconn_t::on_sendreceipt_update(const std::string & receipt_bin,const uint64_t send_receipt_id,const uint16_t from_table_id)
        {
            return false;//not handled
        }
    
        bool    xtblvconn_t::on_recvreceipt_update(const std::string & receipt_bin,const uint64_t recv_receipt_id,const uint16_t from_table_id)
        {
            return false;//not handled
        }
    
        bool    xtblvconn_t::on_confirmreceipt_update(const std::string & receipt_bin,const uint64_t confirm_receipt_id,const uint16_t from_table_id)
        {
            return false;//not handled
        }

        const   std::string xtblvconn_t::load_sendreceipt(const uint16_t owner_table_id,const uint64_t request_send_tx_id)
        {
            return std::string();//not impl as default
        }
    
        const   std::string xtblvconn_t::load_recvreceipt(const uint16_t owner_table_id,const uint64_t request_recv_tx_id)
        {
            return std::string();//not impl as default
        }
    
        const   std::string xtblvconn_t::load_confirmreceipt(const uint16_t owner_table_id,const uint64_t request_confirm_tx_id)
        {
            return std::string();//not impl as default
        }
    
        
        
        //----------------------------------xvtblnet_t------------------------------------//
        //virtual network for all tables,a global & shared object
        xvtblnet_t::xvtblnet_t()
        {
            m_tbl_vsocket = NULL;
            memset(m_connects,0,sizeof(m_connects));
        }
    
        xvtblnet_t::xvtblnet_t(xtblvsock_t & vsocket)
        {
            m_tbl_vsocket = NULL;
            memset(m_connects,0,sizeof(m_connects));
            
            m_tbl_vsocket = &vsocket;
            m_tbl_vsocket->add_ref();
            m_tbl_vsocket->reset_sink(this);
        }
        
        xvtblnet_t::~xvtblnet_t()
        {
            for(int i = 0; i < enum_vbucket_has_tables_count; ++i)
            {
                xtblvconn_t * connection = xatomic_t::xexchange(m_connects[i],(xtblvconn_t*)NULL);;
                if(connection != NULL)
                    delete connection;
            }
            if(m_tbl_vsocket != NULL)
            {
                m_tbl_vsocket->reset_sink(NULL);
                m_tbl_vsocket->release_ref();
                m_tbl_vsocket = NULL;
            }
        }
    
        bool  xvtblnet_t::init_vsocket(xtblvsock_t & vsocket) //only allow init when m_tbl_vsocket is nil
        {
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            if(m_tbl_vsocket != NULL)
            {
                xassert(0);
                return false;
            }
            m_tbl_vsocket = &vsocket;
            m_tbl_vsocket->add_ref();
            m_tbl_vsocket->reset_sink(this);
            return true;
        }
    
        xtblvsock_t * xvtblnet_t::get_vsocket() //may override it by own implementation
        {
            return m_tbl_vsocket;
        }
    
        xtblvconn_t* xvtblnet_t::new_vconnect(const uint16_t owner_table_id)
        {
            if(m_tbl_vsocket != NULL)
                return m_tbl_vsocket->create_vconnect(*this,owner_table_id);
            
            xassert(m_tbl_vsocket != NULL);
            return NULL;
        }
        
        xtblvconn_t *  xvtblnet_t::get_vconnect(const uint16_t my_table_id)
        {
            if(my_table_id >= enum_vbucket_has_tables_count)
            {
                xassert(my_table_id < enum_vbucket_has_tables_count);
                return NULL;
            }
            
            xtblvconn_t * exist_connect = m_connects[my_table_id];
            if(NULL != exist_connect)//hit most cases
                return exist_connect;
            
            std::lock_guard<std::recursive_mutex> locker(m_lock); //lock first
            exist_connect = m_connects[my_table_id];//then test again
            if(NULL == exist_connect)
            {
                exist_connect = new_vconnect(my_table_id);
                m_connects[my_table_id] = exist_connect; //assign and store
            }
            return exist_connect;
        }
    
        bool xvtblnet_t::send_packet(xdatapdu_t & pdu) //send to other endpoints
        {
            xtblvsock_t * vsocket = get_vsocket();
            if(NULL == vsocket)
            {
                xassert(vsocket != NULL);
                return false;
            }
            
            std::lock_guard<std::recursive_mutex> locker(m_lock); //multiple-thread safe for send
            return vsocket->send_packet(pdu);
        }
        
        bool xvtblnet_t::on_packet_recv(const xdatapdu_t & pdu)
        {
            if(pdu.get_msg_to() >= enum_vbucket_has_tables_count)
            {
                xassert(0);//add error log
                return false;
            }
            
            xtblvconn_t * target_table = get_vconnect(pdu.get_msg_to());
            if(NULL == target_table)
            {
                xassert(0);
                return false;
            }
            
            std::lock_guard<std::recursive_mutex> locker(m_lock); //multiple-thread safe for recv
            return target_table->on_packet_recv(pdu);
        }
     
        //communication
        xtblvsock_t::xtblvsock_t()
        {
            m_sink = NULL;
        }
    
        xtblvsock_t::xtblvsock_t(xtblvsink_t & sink)
        {
            m_sink = NULL;
            m_sink = &sink;
            m_sink->add_ref();
        }
        
        xtblvsock_t:: ~xtblvsock_t()
        {
            if(m_sink != NULL)
                m_sink->release_ref();
        }
    
        bool   xtblvsock_t::reset_sink(xtblvsink_t * new_sink)
        {
            xtblvsink_t * old_sink = xatomic_t::xexchange(m_sink, new_sink);
            if(old_sink != NULL)
                old_sink->release_ref();
            
            return true;
        }
        
        bool xtblvsock_t::recv_packet(const xdatapdu_t & pdu) //received from other endpoints
        {
            if(NULL == m_sink)
                return false;
            
            return m_sink->on_packet_recv(pdu);
        }
    
        xtblvconn_t*  xtblvsock_t::create_vconnect(xvtblnet_t & vnet,const uint16_t owner_table_id)
        {
            return new xtblvconn_t(vnet,owner_table_id);
        }
    
        bool xtblvsock_t::send_packet(xdatapdu_t & pdu)//send to other endpoints
        {
            std::string _pdu_bin;
            pdu.serialize_to_string(_pdu_bin);
            //send through P2P network
            
            //subclass must implement it
            //xassert(0);
            return false;//as default return false;
        }
        
    } //end of namespace of base
} //end of namespace of top
