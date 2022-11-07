// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xconsensus/xconsensus_face.h"
#include "xdata/xconsensus_biz_type.h"
#include "xconsensus/xconsensus_mgr.h"
#include "xdata/xchain_param.h"
#include "xcommon/xnode_type.h"
#include "xcommon/xnode_id.h"
#include "xstore/xstore.h"

#include "xbase/xobject.h"
#include "xbase/xtimer.h"
#include "xbase/xcontext.h"
#include "xvnetwork/tests/xdummy_vnetwork_driver.h"

#include <map>

#ifndef __USING_OLD_CONSENSUS_METHOD__
    //#define __USING_OLD_CONSENSUS_METHOD__
#endif

namespace top
{
    namespace xchain
    {
        class xvnetwork_test;
        class xvnetwork_node_t : public vnetwork::tests::xdummy_vnetwork_driver_t
        {
        public:
            xvnetwork_node_t(const std::string _node_id,xvnetwork_test * _network);
            ~xvnetwork_node_t();
        private:
            xvnetwork_node_t();
            xvnetwork_node_t(const xvnetwork_node_t &);
            xvnetwork_node_t & operator = (const xvnetwork_node_t &);
        public:
            virtual void    start()
            {
                printf("xvnetwork_driver_face::start \n");
            }

            virtual void    stop()
            {
                printf("xvnetwork_driver_face::stop \n");
            }

            bool    start_consensus(uint64_t seq_id);
        public:
            void    on_msg_in(const xvnode_address_t & from,const xvnode_address_t & to,const vnetwork::xmessage_t & message,std::uint64_t const timer_height)
            {
                _msg_callback(from,message,timer_height);
            }
        public:
            virtual void    register_message_ready_notify(top::common::xmessage_category_t const message_category,
                                                          vnetwork::xvnetwork_message_ready_callback_t cb)
            {
                printf("xvnetwork_driver_face::register_message_ready_notify \n");
                _msg_callback = cb;
            }

            /**
             * \brief Un-register the message notify for a message category.
             * \param message_category Message category to un-register.
             */
            virtual void    unregister_message_ready_notify(top::common::xmessage_category_t const message_category)
            {
                printf("xvnetwork_driver_face::unregister_message_ready_notify \n");
            }

            virtual void    handshake(common::xnode_type_t const type)
            {
                printf("xvnetwork_driver_face::handshake \n");
            }

            virtual common::xnetwork_id_t const &   network_id() const noexcept
            {
                printf("xvnetwork_driver_face::network_id \n");
                return _net_id;
            }

            virtual xvnode_address_t const &    address() const noexcept
            {
                //printf("xvnetwork_driver_face::address \n");
                return _node_address;
            }

            virtual void    send_to(xvnode_address_t const & to,vnetwork::xmessage_t const & message);

            virtual void    broadcast(vnetwork::xmessage_t const & message);

            /**
             * \brief Forward a broadcast to dst address. dst address must be a cluster address.
             */
            virtual void forward_broadcast_message(vnetwork::xmessage_t const & message,xvnode_address_t const & dst);

            virtual common::xnode_id_t const & host_node_id() const noexcept
            {
                printf("xvnetwork_driver_face::host_node_id \n");
                return _node_id;
            }

            virtual std::vector<xvnode_address_t>   member_of_group(common::xcluster_id_t const & cid, consensus::xvnode_type_t const tp) const;


            virtual std::map<xvnode_address_t, xcrypto_key_t<pub>>  neighbor_crypto_keys() const;

            virtual std::map<xvnode_address_t, xcrypto_key_t<pub>>  parent_crypto_keys(common::xminer_type_t const parent_type ) const;

            virtual std::map<xvnode_address_t, xcrypto_key_t<pub>>  child_crypto_keys(common::xcluster_id_t const & cid, common::xminer_type_t const child_type) const
            {
                return std::map<xvnode_address_t, xcrypto_key_t<pub>>();
            }

            virtual xvnode_address_t  archive_address() const
            {
                return xvnode_address_t();
            }

            virtual std::vector<xvnode_address_t>   parent_addresses(common::xminer_type_t const parent_type = common::xminer_type_t::advance) const
            {
                //printf("xvnetwork_driver_face::parent_addresses \n");
                return std::vector<xvnode_address_t>();
            }

            virtual std::vector<xvnode_address_t>   child_addresses(common::xcluster_id_t const & cid, common::xminer_type_t const child_type) const
            {
                printf("xvnetwork_driver_face::child_addresses \n");
                return std::vector<xvnode_address_t>();
            }

            virtual std::vector<xvnode_address_t>   neighbor_addresses() const;
//
//            virtual std::vector<xvnode_address_t>   child_addresses(common::xminer_type_t const child_type = common::xminer_type_t::validator) const
//            {
//                printf("xvnetwork_driver_face::child_addresses \n");
//                return std::vector<xvnode_address_t>();
//            }

            virtual observer_ptr<vnetwork::xvhost_face_t> virtual_host() const noexcept

            {
                return nullptr;
            }

            /**
             * \brief Get the working type of this virtual node.
             * \return The virtual node type.
             */
            virtual common::xnode_type_t   type() const noexcept
            {
                //printf("xvnetwork_driver_face::type \n");
                return top::common::xnode_type_t::consensus_validator;
            }

            std::string & get_public_key() {return m_public_key;}
            std::string & get_private_key() {return m_private_key;}
        public:
            consensus::xconsensus_object_face * create_consensus_object(const std::string & obj_id,const std::string& identify_id);
        protected:
            xvnetwork_test *                      _network;
            top::common::xnetwork_id_t            _net_id;
            top::common::xtop_node_id            _node_id;
            top::vnetwork::xvnode_address_t     _node_address;
            vnetwork::xvnetwork_message_ready_callback_t    _msg_callback;
            consensus::xconsensus_object_id       _consensus_object_id;
            consensus::xconsensus_instance*       _consensus_instance;
        protected:
            std::string     m_public_key;
            std::string     m_private_key;
        };

        class xvnetwork_test
        {
        public:
            xvnetwork_test();
        public:
            bool    start_consensus(uint64_t seq_id);
        public:
            void    send_to(vnetwork::xvnode_address_t const & from,vnetwork::xvnode_address_t const & to,vnetwork::xmessage_t const & message);
            void    broadcast(vnetwork::xvnode_address_t const & from,vnetwork::xmessage_t const & message);
            void    forward_broadcast_message(vnetwork::xvnode_address_t const & from,vnetwork::xvnode_address_t const & to,vnetwork::xmessage_t const & message);

            virtual std::vector<xvnode_address_t>                   neighbor_addresses();
            virtual std::map<xvnode_address_t, xcrypto_key_t<pub>>  neighbor_crypto_keys();
            virtual std::map<xvnode_address_t, xcrypto_key_t<pub>>  parent_crypto_keys(common::xminer_type_t const parent_type ) const;
        private:
            std::vector<xvnetwork_node_t*> shard_nodes;
            std::string                    m_public_key;
            std::string                    m_private_key;
        };

        class xpbft_service_t : public top::base::xtimersink_t
        {
        public:
            xpbft_service_t();
            ~xpbft_service_t();
        protected:
            virtual bool        on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms) override;

            virtual bool        on_timer_start(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms) override;    //attached into io-thread
            virtual bool        on_timer_stop(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms) override;  //detach means it detach from io-thread
        protected:
            xvnetwork_test              _test_network;
            top::base::xtimer_t*        _raw_timer;
            top::base::xiothread_t*     _raw_thread;
            int64_t                     _tx_index;
        };

        //system contract(natvie c++ contract)
        class xsystem_contract_t : public base::xxtimer_t
        {
        protected:
            explicit xsystem_contract_t(const int32_t run_at_thread_id,store::xstore_face_t* _xstore_ptr,consensus::xconsensus_instance* _consensus_instance);
            virtual ~xsystem_contract_t();
        private:
            xsystem_contract_t();
            xsystem_contract_t(const xsystem_contract_t &);
            xsystem_contract_t & operator = (const xsystem_contract_t &);
        public:
            virtual void*   query_interface(const int32_t type) override;//caller respond to cast (void*) to related interface ptr
        public:
            //virtual bool    on_block_fire(data::xblock_ptr * block){return true;}
        protected:
            store::xstore_face_t*                   m_xstore_ptr;
            consensus::xconsensus_instance*         m_consensus_instance; //dedicated instance
        private:
        };

        class xsystem_contract_runtime
        {

        };

        //native contract of timer
        class xconsensus_timer : public xsystem_contract_t
        {
        public:
            explicit xconsensus_timer(const int32_t run_at_thread_id,store::xstore_face_t* _xstore_ptr,consensus::xconsensus_instance* _consensus_instance);
        protected:
            virtual ~xconsensus_timer();
        private:
            xconsensus_timer();
            xconsensus_timer(const xconsensus_timer &);
            xconsensus_timer & operator = (const xconsensus_timer &);
        private:
            class xconsensus_timer_block : public consensus::xconsensus_object_face
            {
                virtual data::xconsensus_id_ptr& consensus_id() = 0;  // consensus object identity

                virtual void consensus_id(data::xconsensus_id_ptr & _id)  //performance optimization
                {
                    _id = consensus_id(); //default implementation to avoid other module build issue
                }

                virtual int32_t get_consensus_targets(std::string &) = 0;
                virtual int32_t set_consensus_targets(const std::string &) = 0;

                virtual int32_t make_block(top::data::xconsensus_create_block_data_ptr & ,
                               consensus::xmake_block_biz_callback_members & ) = 0; // consensus create block

                virtual std::string get_block_serialize_str() = 0;

                virtual consensus::xconsensus_biz_extend_params get_biz_extend_params() = 0;

                virtual consensus::xconsensus_instance*    get_consensus_instance() = 0;
            };
        public:
            //native timer callback
            virtual bool   on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms) override;
        };
    }
}
