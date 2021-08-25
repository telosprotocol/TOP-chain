// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xatom.h"
#include "xbase/xdata.h"

namespace top
{
    namespace base
    {
        class xvtblnet_t;//announce first
    
        //xtblvend_t only valid and meaning within xtblpair_t
        class xtblvend_t
        {
            friend class xtblpair_t;
            friend class xtblvconn_t;
        public:
            xtblvend_t(const uint16_t table_id,const uint64_t send_tx_id,const uint64_t recv_tx_id,const uint64_t confirm_tx_id);
            xtblvend_t(xtblvend_t && obj);
            xtblvend_t(const xtblvend_t & obj);
            ~xtblvend_t();
        protected://only allow called by xtblpair_t
            xtblvend_t & operator = (const xtblvend_t & obj);
        private: //dont impl
            xtblvend_t();

        public:
            inline const uint16_t   get_table_id() const {return (uint16_t)m_table_id;}
            inline const uint64_t   get_send_tx_id() const {return m_send_tx_id;}
            inline const uint64_t   get_recv_tx_id()  const {return m_recv_tx_id;}
            inline const uint64_t   get_confirm_tx_id() const {return m_confrm_tx_id;}
            
            int32_t         serialize_to_string(std::string & bin_data);
            
        private:
            int32_t         serialize_from_string(const std::string & bin_data);
        private:
            int             m_table_id;       //unique to identify one table
            uint64_t        m_send_tx_id;     //latest id of send tx at this endpoint
            uint64_t        m_recv_tx_id;     //latest id of recev tx at this endpoint
            uint64_t        m_confrm_tx_id;   //latest id of confirm tx at this endpoint
        };
        
        //repsent one logic link between [vend,vend] pair
        class xtblpair_t
        {
            friend class xtblvconn_t;
        protected: //only allow construct by xtblvconn_t
            xtblpair_t(const uint16_t this_table_id,const uint16_t peer_table_id);
        public:
            ~xtblpair_t();
        private:
            xtblpair_t();
            xtblpair_t(xtblpair_t &&);
            xtblpair_t(const xtblpair_t &);
            xtblpair_t & operator = (const xtblpair_t &);
            
        private: //only allow called by xtblvconn_t
            void update_this(xtblvend_t & new_val);
            void update_peer(xtblvend_t & new_val);
            
        public: //readonly
            xtblvend_t      m_this_vend;
            xtblvend_t      m_peer_vend;
        };

        //each xtableobj_t have only one xtblvconn_t that manage multiple pairs
        //xtblvconn_t present one virtual channel that point to multiple peers from owner_table_id
        class xtblvconn_t
        {
            friend class xvtblnet_t;
            enum enum_vtable_msg_type
            {
                enum_vtable_msg_type_base                    = 0,
                enum_vtable_msg_type_invalid                 = enum_vtable_msg_type_base + 0,
                
                enum_vtable_msg_type_update_self_window      = enum_vtable_msg_type_base + 1,
                enum_vtable_msg_type_update_peer_window      = enum_vtable_msg_type_base + 2,
                
                enum_vtable_msg_type_pull_sendreceipt        = enum_vtable_msg_type_base + 4,
                enum_vtable_msg_type_pull_recvreceipt        = enum_vtable_msg_type_base + 5,
                enum_vtable_msg_type_pull_confirmreceipt     = enum_vtable_msg_type_base + 6,
                
                enum_vtable_msg_type_update_sendreceipt      = enum_vtable_msg_type_base + 7,
                enum_vtable_msg_type_update_recvreceipt      = enum_vtable_msg_type_base + 8,
                enum_vtable_msg_type_update_confirmreceipt   = enum_vtable_msg_type_base + 9,
            };
        public:
            xtblvconn_t(xvtblnet_t & vnet,const uint16_t owner_table_id);//which table own this virtual connection
        protected:
            virtual ~xtblvconn_t();
        private:
            xtblvconn_t();
            xtblvconn_t(xtblvconn_t &&);
            xtblvconn_t(const xtblvconn_t &);
            xtblvconn_t & operator = (const xtblvconn_t &);
            
        public: //operate interface at multiple thread safe
            inline xvtblnet_t &    get_tbl_vnet() {return m_table_vnet;}
            inline const uint16_t  get_owner_table_id() const {return (uint16_t)m_owner_table_id;}

            bool    update_my_window(xtblvend_t & self_window,const uint16_t paired_peer_table_id); //update self
            bool    send_window_update(const uint16_t dst_table_id);//send my endpoint to update peer
            
            //pull receipt of request_send_tx_id from dst_table_id
            bool    pull_sendreceipt(const uint16_t dst_table_id,const uint64_t request_send_tx_id);
            //pull receipt of request_recv_tx_id from dst_table_id
            bool    pull_recvreceipt(const uint16_t dst_table_id,const uint64_t request_recv_tx_id);
            //pull receipt of request_confirm_tx_id from dst_table_id
            bool    pull_confirmreceipt(const uint16_t dst_table_id,const uint64_t request_confirm_tx_id);
            
            //push receipt of send_receipt_id to dst_table_id with receipt_bin
            bool    push_sendreceipt(const uint16_t dst_table_id,const uint64_t send_receipt_id,const std::string & receipt_bin);
            //push receipt of recv_receipt_id to dst_table_id with receipt_bin
            bool    push_recvreceipt(const uint16_t dst_table_id,const uint64_t recv_receipt_id,const std::string & receipt_bin);
            //push receipt of confirm_receipt_id to dst_table_id with receipt_bin
            bool    push_confirmreceipt(const uint16_t dst_table_id,const uint64_t confirm_receipt_id,const std::string & receipt_bin);
            
        protected: //multiple thread safe,and subclass need overide those to provide real data
            virtual const   std::string load_sendreceipt(const uint16_t owner_table_id,const uint64_t request_send_tx_id);
            virtual const   std::string load_recvreceipt(const uint16_t owner_table_id,const uint64_t request_recv_tx_id);
            virtual const   std::string load_confirmreceipt(const uint16_t owner_table_id,const uint64_t request_confirm_tx_id);
            
        protected: //multiple thread safe and return false when not handled,and subclass need overide those to trigger logic
            virtual bool    on_my_window_update(const xtblvend_t & self_window,const uint16_t paired_table_id);
            virtual bool    on_peer_window_update(const xtblvend_t & peer_window,const uint16_t paired_table_id);

            virtual bool    on_sendreceipt_update(const std::string & receipt_bin,const uint64_t send_receipt_id,const uint16_t from_table_id);
            virtual bool    on_recvreceipt_update(const std::string & receipt_bin,const uint64_t recv_receipt_id,const uint16_t from_table_id);
            virtual bool    on_confirmreceipt_update(const std::string & receipt_bin,const uint64_t confirm_receipt_id,const uint16_t from_table_id);
            
        private:
            virtual bool    on_packet_recv(const xdatapdu_t & pdu);
            xtblpair_t*     get_pair(const uint16_t peer_table_id);
            
        private:
            xvtblnet_t&    m_table_vnet;
            xtblpair_t*    m_peer_paris[enum_vbucket_has_tables_count];
            int            m_owner_table_id;
        };
        
        //callback of virtual communication layer for tables
        class xtblvsink_t : public xobject_t
        {
        protected:
            xtblvsink_t(){}
            virtual ~xtblvsink_t(){};
        private:
            xtblvsink_t(xtblvsink_t &&);
            xtblvsink_t(const xtblvsink_t &);
            xtblvsink_t & operator = (const xtblvsink_t &);
        public:
            virtual bool on_packet_recv(const xdatapdu_t & pdu) = 0;
        };
        
        //interface of virtual communication layer for tables
        class xtblvsock_t : public xobject_t
        {
            friend class xvtblnet_t;
        public:
            xtblvsock_t(xtblvsink_t & sink);
        protected:
            xtblvsock_t();
            virtual ~xtblvsock_t();
        private:
            xtblvsock_t(xtblvsock_t &&);
            xtblvsock_t(const xtblvsock_t &);
            xtblvsock_t & operator = (const xtblvsock_t &);
            
        public://subclass need override this function to create real object of xtblvconn_t
            virtual xtblvconn_t*  create_vconnect(xvtblnet_t & vnet,const uint16_t owner_table_id) = 0;
            virtual bool          send_packet(xdatapdu_t & pdu) = 0;//send to other endpoints
            
            bool   recv_packet(const xdatapdu_t & pdu); //received from other endpoints
        private:
            bool   reset_sink(xtblvsink_t * new_sink); //just allow rest by xtblvnet_t
        private:
            xtblvsink_t *  m_sink;
        };
    
        //virtual network for all tables,a global & shared object
        class xvtblnet_t : public xtblvsink_t
        {
            friend class xvchain_t;
        public:
            xvtblnet_t(xtblvsock_t & sock);
        protected:
            xvtblnet_t();
            virtual ~xvtblnet_t();
        private:
            xvtblnet_t(xvtblnet_t &&);
            xvtblnet_t(const xvtblnet_t &);
            xvtblnet_t & operator = (const xvtblnet_t &);
            
        public: //only worked when m_tbl_vsocket is nil
            bool  init_vsocket(xtblvsock_t & sock);//pair with xtblvnet_t()
            
        public://multiple thread safe
            std::recursive_mutex &  get_lock() {return m_lock;}
            //each table has a vconnection that include mutiple pair
            xtblvconn_t *           get_vconnect(const uint16_t my_table_id);
            xtblvsock_t *           get_vsocket();
            
            bool                    send_packet(xdatapdu_t & pdu); //send to other endpoints
            
        protected: //multiple thread safe
            xtblvconn_t*            new_vconnect(const uint16_t owner_table_id);
            virtual bool            on_packet_recv(const xdatapdu_t & pdu) override;//event of xtblvsink_t
        private:
            std::recursive_mutex   m_lock;
            xtblvsock_t*           m_tbl_vsocket;
            xtblvconn_t*           m_connects[enum_vbucket_has_tables_count];
        };
    
        
    }
}
