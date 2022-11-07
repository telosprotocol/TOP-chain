// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xpbftservice.h"
#include "xconsensus/xconsensus_face.h"
#include "xdata/xconsensus_biz_type.h"
#include "xconsensus/xconsensus_mgr.h"
#include "xdata/xchain_param.h"
#include "xcommon/xnode_type.h"
#include "xcommon/xnode_id.h"
#include "xmutisig/xmutisig.h"
#include "xmutisig/xschnorr.h"
#include "xbase/xutl.h"


namespace top
{
    namespace xchain
    {
        static std::atomic<int> s_atomic_suc(1);
        static std::atomic<int> s_atomic_fail(0);
        int64_t pbft_starat_time = 0;

        class mock_xconsensus_object : public consensus::xconsensus_object_face {
        public:
            mock_xconsensus_object(consensus::xconsensus_instance * instance,const std::string & _consensus_obj_id,const std::string & _consensus_identify_id)
            : m_consensus_instance(instance)
            {
                m_consensus_obj_id = _consensus_obj_id;
                m_consensus_identify_id = _consensus_identify_id;
            }

            virtual ~mock_xconsensus_object()
            {
                //printf("~mock_xconsensus_object \n");
            }
        public:
            virtual data::xconsensus_id_ptr& consensus_id() {
                if (!consensus_id_is_inited()) {
                    set_consensus_id_inited();
                    data::xconsensus_id_ptr& id = get_create_block_data()->consensus_id();
                    id->m_object = m_consensus_obj_id;
                    id->m_identity = m_consensus_identify_id;
                    id->m_biz_type = 12;
                }
                return get_create_block_data()->consensus_id();
            }

            virtual int32_t get_consensus_targets(std::string &) { return 0; }
            virtual int32_t set_consensus_targets(const std::string &) { return 0; }

            virtual int32_t make_block(data::xconsensus_create_block_data_ptr&,
                               consensus::xmake_block_biz_callback_members & callback_mem) {
                callback_mem.m_block_hash = "test_pbft_hash";
                callback_mem.m_chain_max_block_height = 1;
                return 0;
            }

            virtual std::string get_block_serialize_str()
            {
                return "";
            }

            virtual data::xblock_ptr_t get_block_ptr() override { return nullptr; }

            virtual consensus::xconsensus_biz_extend_params get_biz_extend_params() { return {}; }

            virtual consensus::xconsensus_instance*    get_consensus_instance()
            {
                return m_consensus_instance;
            }


         //get notification when consensus finish
            virtual bool  on_consensus_finish(consensus::xconsensus_result & _result,std::string & _consensus_block)  //return true if handled this event
            {
                if (_result.m_result == 0)
                {
                    s_atomic_suc++;
                    //printf("test_pbft_callback,successful finish pbft consensus \n");
                    if(0 == pbft_starat_time)
                        pbft_starat_time = top::base::xtime_utl::time_now_ms();
                }
                else {
                    s_atomic_fail++;
                    //printf("test_pbft_callback,fail for pbft consensus as error(%d) \n",callback_obj.m_result);
                }
                if (((uint32_t)s_atomic_suc + (uint32_t)s_atomic_fail) % 100 == 0)
                {
                    //xinfo("consensus result[%d:%d]", (uint32_t)s_atomic_suc, (uint32_t)s_atomic_fail);
                    const int64_t duration = ((top::base::xtime_utl::time_now_ms() - pbft_starat_time) + 1) / 1000 + 1;
                    printf("on_consensus_finish result[%d:%d], TPS=%d /s \n", (uint32_t)s_atomic_suc, (uint32_t)s_atomic_fail,(int32_t)(s_atomic_suc/duration));
                }
                return true;
            }

            virtual void start() {}
            virtual void stop() {}
            virtual bool running() const noexcept { return false;}
            virtual void running(bool const v) noexcept {}
        private:
            std::string   m_consensus_obj_id;
            std::string   m_consensus_identify_id;
            consensus::xconsensus_instance* m_consensus_instance;
        };


        uint32_t test_pbft_callback(top::consensus::xbiz_callback_obj callback_obj)
        {
            if (callback_obj.m_result == 0)
            {
                s_atomic_suc++;
                //printf("test_pbft_callback,successful finish pbft consensus \n");
                if(0 == pbft_starat_time)
                    pbft_starat_time = top::base::xtime_utl::time_now_ms();
            }
            else {
                s_atomic_fail++;
                //printf("test_pbft_callback,fail for pbft consensus as error(%d) \n",callback_obj.m_result);
            }
            if (((uint32_t)s_atomic_suc + (uint32_t)s_atomic_fail) % 1024 == 0)
            {
                //xinfo("consensus result[%d:%d]", (uint32_t)s_atomic_suc, (uint32_t)s_atomic_fail);
                const int64_t duration = ((top::base::xtime_utl::time_now_ms() - pbft_starat_time) + 1) / 1000;
                printf("consensus result[%d:%d], TPS=%d /s \n", (uint32_t)s_atomic_suc, (uint32_t)s_atomic_fail,(int32_t)(s_atomic_suc/duration));
            }
            return 0;
        }

        xvnetwork_test::xvnetwork_test()
        {
            std::string raw_key("WGeF6P1j5prR3vkuH10oMhLrj5S7H/ScbFihYFFFqoU=");
            //consensus::consensus_adapter::s_consensus_prikey = base::xstring_utl::base64_decode(raw_key);

            xmutisig::xprikey _pri_key;
            xmutisig::xpubkey _pub_key(_pri_key);
            m_public_key = _pub_key.get_serialize_str();
            m_private_key = _pri_key.get_serialize_str();

            shard_nodes.push_back(new xvnetwork_node_t(std::string("T-1"),this));
            shard_nodes.push_back(new xvnetwork_node_t(std::string("T-2"),this));
            shard_nodes.push_back(new xvnetwork_node_t(std::string("T-3"),this));
            //shard_nodes.push_back(new xvnetwork_node_t(std::string("T-4"),this));
            //shard_nodes.push_back(new xvnetwork_node_t(std::string("T-5"),this));
            //shard_nodes.push_back(new xvnetwork_node_t(std::string("T-6"),this));
            //shard_nodes.push_back(new xvnetwork_node_t(std::string("T-7"),this));
        }

        bool   xvnetwork_test::start_consensus(uint64_t seq_id)
        {
            for(std::vector<xvnetwork_node_t*>::iterator it = shard_nodes.begin(); it != shard_nodes.end(); ++it)
            {
                xvnetwork_node_t * _node = *it;
                _node->start_consensus(seq_id);
            }
            return true;
        }

        void xvnetwork_test::send_to(vnetwork::xvnode_address_t const & from, vnetwork::xvnode_address_t const & to, vnetwork::xmessage_t const & message) {
            // printf("xvnetwork_driver_face::send_to,from(%s), to(%s) \n",from.account_address().to_string().c_str(),to.account_address().to_string().c_str());
            if (to.account_address().empty()) {
                for (std::vector<xvnetwork_node_t *>::iterator it = shard_nodes.begin(); it != shard_nodes.end(); ++it) {
                    xvnetwork_node_t * _node = *it;
                    if (_node->address() != from)
                        _node->on_msg_in(from, to, message, 0);
                }
            } else {
                for (std::vector<xvnetwork_node_t *>::iterator it = shard_nodes.begin(); it != shard_nodes.end(); ++it) {
                    xvnetwork_node_t * _node = *it;
                    if (_node->address() == to) {
                        _node->on_msg_in(from, to, message, 0);
                    }
                }
            }
        }

        void    xvnetwork_test::broadcast(vnetwork::xvnode_address_t const & from,vnetwork::xmessage_t const & message)
        {
            printf("xvnetwork_driver_face::broadcast,from(%s)\n",from.account_address().to_string().c_str());

            top::vnetwork::xvnode_address_t empty_target;
            for(std::vector<xvnetwork_node_t*>::iterator it = shard_nodes.begin(); it != shard_nodes.end(); ++it)
            {
                xvnetwork_node_t * _node = *it;
                _node->on_msg_in(from, empty_target, message, 0);
            }
        }

        /**
         * \brief Forward a broadcast to dst address. dst address must be a cluster address.
         */
        void xvnetwork_test::forward_broadcast_message(vnetwork::xvnode_address_t const & from,vnetwork::xvnode_address_t const & to,vnetwork::xmessage_t const & message)
        {
            printf("xvnetwork_driver_face::forward_broadcast_message,from(%s), to(%s) \n",from.account_address().to_string().c_str(),to.account_address().to_string().c_str());

            for(std::vector<xvnetwork_node_t*>::iterator it = shard_nodes.begin(); it != shard_nodes.end(); ++it)
            {
                xvnetwork_node_t * _node = *it;
                if(_node->address() == to)
                {
                    _node->on_msg_in(from, to, message, 0);
                }
            }
        }

        std::map<xvnode_address_t, xcrypto_key_t<pub>>  xvnetwork_test::neighbor_crypto_keys()
        {
            //xcrypto_key_t<pub> pub_key{ base::xstring_utl::base64_decode("Ax+pxzxFvcYrRGYa7iBJm9btbQVau2gZhu/5xEVrPlvN") };

            std::map<xvnode_address_t, xcrypto_key_t<pub>> acount_keys;
            for(std::vector<xvnetwork_node_t*>::iterator it = shard_nodes.begin(); it != shard_nodes.end(); ++it)
            {
                xvnetwork_node_t * _node = *it;
                acount_keys[_node->address()] = xcrypto_key_t<pub>(_node->get_public_key());
            }
            return acount_keys;
        }

        std::map<xvnode_address_t, xcrypto_key_t<pub>>  xvnetwork_test::parent_crypto_keys(common::xminer_type_t const parent_type ) const
        {
            std::map<xvnode_address_t, xcrypto_key_t<pub>>  parent_keys;
            //parent_keys[(*shard_nodes.begin())->address()] = xcrypto_key_t<pub>(m_public_key);
            return parent_keys;
        }

        std::vector<xvnode_address_t>  xvnetwork_test::neighbor_addresses()
        {
            std::vector<xvnode_address_t> list;
            for(std::vector<xvnetwork_node_t*>::iterator it = shard_nodes.begin(); it != shard_nodes.end(); ++it)
            {
                xvnetwork_node_t * _node = *it;
                list.push_back(_node->address());
            }
            return list;
        }


        xvnetwork_node_t::xvnetwork_node_t(const std::string _node_id,xvnetwork_test * _network)
            : _network(_network),
            _net_id(common::xtopchain_network_id),
            _node_id(_node_id.c_str()),
            _node_address{vnetwork::xcluster_address_t
            {
                common::xtopchain_network_id,
                common::xcommittee_zone_id,    // to be bec network
                common::xcommittee_cluster_id,    // {vnetwork::consensus_cluster_id_base}
                common::xcommittee_group_id,
                common::xnode_type_t::consensus_validator
            },
                vnetwork::xaccount_address_t{common::xnode_id_t{ _node_id.c_str() }},vnetwork::xversion_t(0)}
        {

            /*
            const uint64_t start = base::xtime_utl::time_now_ms();
            const int total_round = 1000;
            for(int i = 0; i < total_round; ++i)
            {
                std::pair<xmutisig::xsecret_rand, xmutisig::xrand_point> item = xmutisig::xschnorr::instance()->generate_rand_pair();
            }
            const uint64_t end = base::xtime_utl::time_now_ms();
            const int duration = (int)(end - start + 1);
            printf("generate_rand_pair,speed = %d",total_round / duration);
            */
//            std::string test_signature("test_signature");
//            xmutisig::bn_ptr_t ptr_bn = xmutisig::xschnorr::instance()->generate_message_bn(test_signature);
//            if(ptr_bn.get() == NULL)
//            {
//                throw "";
//            }

            xmutisig::xprikey _pri_key;
            xmutisig::xpubkey _pub_key(_pri_key);
            m_public_key = _pub_key.get_serialize_str();
            m_private_key = _pri_key.get_serialize_str();

#ifdef __USING_OLD_CONSENSUS_METHOD__
            top::consensus::xconsensus_object_params params;
            params.m_net_host = this;
            params.m_state_machine_type = top::consensus::BT_shard_alone_consensus;
            params.m_least_node_num = 2;
            params.m_rate.m_success_num = 3;
            params.m_rate.m_total_num = 5;
            params.m_consensus_private_key = m_private_key;
            params.m_consensus_public_key = m_public_key;

            //consensus::consensus_adapter::get_instance()->create_consensus_object(params, _consensus_object_id);
            //consensus::consensus_adapter::get_instance()->register_biz_type_notify_handler(_consensus_object_id,12, test_pbft_callback);
#else
            top::consensus::xconsensus_instance_params params;
            params.m_net_host = this;
            params.m_state_machine_type = top::consensus::BT_shard_alone_consensus;
            params.m_least_node_num = 2;
            params.m_rate.m_success_num = 3;
            params.m_rate.m_total_num = 5;
            params.m_consensus_private_key = m_private_key;
            params.m_consensus_public_key = m_public_key;

            _consensus_instance = consensus::xconsensus_mgr::get_mgr()->create_unit_contract_consensus_instance(params, nullptr, nullptr);
#endif
        }

        xvnetwork_node_t::~xvnetwork_node_t()
        {

        }

        consensus::xconsensus_object_face * xvnetwork_node_t::create_consensus_object(const std::string & obj_id,const std::string& identify_id)
        {
            return new mock_xconsensus_object(_consensus_instance,obj_id,identify_id);
        }

        bool   xvnetwork_node_t::start_consensus(uint64_t seq_id)
        {
            const std::string tag = std::string("test_pbft") + std::to_string(seq_id);

            #ifdef __USING_OLD_CONSENSUS_METHOD__
            consensus::biz_ptr_t _bizobj = new mock_xconsensus_object(_consensus_instance,tag,tag);
            consensus::consensus_adapter::get_instance()->start_consensus(_consensus_object_id, _bizobj);
            _bizobj->release_ref();
            #else
            mock_xconsensus_object * _object = new mock_xconsensus_object(_consensus_instance,tag,tag);
            _consensus_instance->start_consensus(_object);
            _object->release_ref();
            #endif
            return true;
        }

        void    xvnetwork_node_t::send_to(xvnode_address_t const & to,vnetwork::xmessage_t const & message)
        {
            return _network->send_to(_node_address,to, message);
        }

        void    xvnetwork_node_t::broadcast(vnetwork::xmessage_t const & message)
        {
            return _network->broadcast(_node_address,message);
        }

        std::vector<xvnode_address_t>   xvnetwork_node_t::neighbor_addresses() const
        {
            //printf("xvnetwork_driver_face::neighbor_addresses \n");
            return _network->neighbor_addresses();
        }

        std::map<xvnode_address_t, xcrypto_key_t<pub>>  xvnetwork_node_t::neighbor_crypto_keys() const
        {
            //xcrypto_key_t<pub>{ base::xstring_utl::base64_decode("Ax+pxzxFvcYrRGYa7iBJm9btbQVau2gZhu/5xEVrPlvN") };
            return _network->neighbor_crypto_keys();
        }

        std::map<xvnode_address_t, xcrypto_key_t<pub>>  xvnetwork_node_t::parent_crypto_keys(common::xminer_type_t const parent_type ) const
        {
            return _network->parent_crypto_keys(parent_type);
        }

        std::vector<xvnode_address_t>   xvnetwork_node_t::member_of_group(common::xcluster_id_t const & cid, consensus::xvnode_type_t const tp) const
        {
            //printf("xvnetwork_driver_face::member_of_group \n");
            return _network->neighbor_addresses();
        }

        xpbft_service_t::xpbft_service_t()
        {
            _tx_index = 0;

            _raw_timer =  NULL;
            _raw_thread = NULL;
            _raw_thread = top::base::xiothread_t::create_thread(top::base::xcontext_t::instance(),0,-1);

            _raw_timer = _raw_thread->create_timer((xtimersink_t*)this);
            _raw_timer->start(2000, 100);//every 1 second
        }

        xpbft_service_t::~xpbft_service_t()
        {

        }

        bool   xpbft_service_t::on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms)
        {
            //if(_tx_index < 10000)
            {
                for(int i = 0; i < 10; ++i)
                {
                    const int64_t seq_id = (++_tx_index) + current_time_ms;
                    _test_network.start_consensus(seq_id);
                }
            }

            return true;
        }

        bool   xpbft_service_t::on_timer_start(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms)    //attached into io-thread
        {
            return true;
        }
        bool   xpbft_service_t::on_timer_stop(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms)   //detach means it detach from io-thread
        {
            return true;
        }

        xsystem_contract_t::xsystem_contract_t(const int32_t run_at_thread_id,store::xstore_face_t* _xstore_ptr,consensus::xconsensus_instance* _consensus_instance)
        :base::xxtimer_t(base::xcontext_t::instance(),run_at_thread_id,base::enum_xobject_type_system_contract)
        {
            m_xstore_ptr = NULL;
            m_consensus_instance = NULL;

            xassert(_xstore_ptr != NULL);
            _xstore_ptr->add_ref(); //must be valid
            m_xstore_ptr = _xstore_ptr;

            xassert(_consensus_instance != NULL);
            _consensus_instance->add_ref(); //must be valid
            m_consensus_instance = _consensus_instance;

            start(4000,2000); //start timer at every 2 seconds
        }

        xsystem_contract_t::~xsystem_contract_t()
        {
            if(m_xstore_ptr != NULL)
                m_xstore_ptr->release_ref();

            if(m_consensus_instance != NULL)
                m_consensus_instance->release_ref();
        }

        void*  xsystem_contract_t::query_interface(const int32_t type)//caller respond to cast (void*) to related  interface ptr
        {
            if(type == base::enum_xobject_type_system_contract)
                return this;

            return base::xxtimer_t::query_interface(type);
        }

        xconsensus_timer::xconsensus_timer(const int32_t run_at_thread_id,store::xstore_face_t* _xstore_ptr,consensus::xconsensus_instance* _consensus_instance)
            :xsystem_contract_t(run_at_thread_id,_xstore_ptr,_consensus_instance)
        {

        }

        xconsensus_timer::~xconsensus_timer()
        {

        }

        bool  xconsensus_timer::on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms)
        {
            return true;
        }
    }
}
