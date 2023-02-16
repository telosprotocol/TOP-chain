// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <map>
#include "xbase/xobject.h"
#include "xbase/xns_macro.h"
#include "xbase/xobject_ptr.h"
#include "xsync/xsync_message.h"
#include "xcommon/xmessage_id.h"
#include "xvnetwork/xaddress.h"
#include "xvnetwork/xvnetwork_message.h"

NS_BEG2(top, sync)

class xsync_session_handler_t : public base::xobject_t{
    public:
        xsync_session_handler_t(){
        }

        bool handler(common::xmessage_id_t msgid, uint32_t msg_size, const vnetwork::xvnode_address_t &from_address,
            const vnetwork::xvnode_address_t &network_self,
            const xsync_message_header_ptr_t &header,
            base::xstream_t &stream,
            vnetwork::xtop_vnetwork_message::hash_result_type msg_hash,
            int64_t recv_time);

        virtual std::string id() = 0;
    protected:
        virtual bool is_request(common::xmessage_id_t msgid) = 0;
        
        virtual void on_request(common::xmessage_id_t msgid, uint32_t msg_size, const vnetwork::xvnode_address_t &from_address,
            const vnetwork::xvnode_address_t &network_self,
            const xsync_message_header_ptr_t &header,
            base::xstream_t &stream,
            vnetwork::xtop_vnetwork_message::hash_result_type msg_hash,
            int64_t recv_time) = 0;

        virtual bool is_response(common::xmessage_id_t msgid) = 0;

        virtual void on_response(common::xmessage_id_t msgid, uint32_t msg_size, const vnetwork::xvnode_address_t &from_address,
            const vnetwork::xvnode_address_t &network_self,
            const xsync_message_header_ptr_t &header,
            base::xstream_t &stream,
            vnetwork::xtop_vnetwork_message::hash_result_type msg_hash,
            int64_t recv_time) = 0;
};

class xsync_session_dispatcher_t{
    public:
        xsync_session_dispatcher_t(){
        }

        void handle(common::xmessage_id_t msgid, uint32_t msg_size, const vnetwork::xvnode_address_t &from_address,
                const vnetwork::xvnode_address_t &network_self,
                const xsync_message_header_ptr_t &header,
                base::xstream_t &stream,
                vnetwork::xtop_vnetwork_message::hash_result_type msg_hash,
                int64_t recv_time);

        void add(xobject_ptr_t<xsync_session_handler_t> handler);
        void remove(xobject_ptr_t<xsync_session_handler_t> handler);

    private:
        std::map<std::string, xobject_ptr_t<xsync_session_handler_t>> m_handlers;
};

NS_END2
