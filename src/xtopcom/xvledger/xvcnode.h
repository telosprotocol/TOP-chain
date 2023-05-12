// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>
#include <map>
#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"
#    pragma clang diagnostic ignored "-Wsign-compare"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#    pragma GCC diagnostic ignored "-Wsign-compare"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

#include "xbase/xobject.h"
#include "xbase/xdata.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include "xvaccount.h"

//interface for chain node
namespace top
{
    namespace base
    {
        ///////////////////////provide general structure for chain node  //////////////////
        //xvnode_t not to allow modify after construct,and auth account not allow change public/private key neither.
        //note:once a node lost it's pub/pri keys,the node dont have chance to update keys. the only way is to create new account with new keys and follow process of node joining.
        //each node has unqie xip2 address, but one node may have multiple roles who at different groups
        class xvnode_t : public xrefcount_t
        {
        public:
            //sign_pub_key must = 33 bytes of the compressed public key of ECC(secp256k1 or ed25519 curve)
            //sign_pri_key must = 32bytes of raw private key of ECC(secp256k1 or ed25519 curve, decied by account address)
            xvnode_t(const std::string & account,const xvip2_t & xip2_addr,const std::string & sign_pub_key);
        protected:
            virtual ~xvnode_t();
        private:
            xvnode_t();
            xvnode_t(const xvnode_t &);
            xvnode_t & operator = (const xvnode_t &);
        public:
            inline const xvip2_t &      get_xip2_addr()     const {return m_node_address;}
            inline const std::string&   get_sign_pubkey()   const {return m_sign_pubkey;}//public  key for signing by node 'account
            inline const std::string&   get_address()       const {return m_account;}
            inline const std::string&   get_account()       const {return m_account;}
        private:
            std::string         m_account;      //account address of node
            xvip2_t             m_node_address;
            std::string         m_sign_pubkey;  //33 bytes of the compressed public key of ECC(secp256k1 or ed25519 curve)
        };
        
        //note: once construction,xvgroup_t never to allow modify until destroy
        class xvnodegroup_t : virtual public xrefcount_t
        {
        public:
            xvnodegroup_t(const xvip2_t & group_address,const uint64_t effect_clock_height,std::vector<xvnode_t*> const & nodes);
            xvnodegroup_t(const xvip2_t & group_address,const uint64_t effect_clock_height,std::deque<xvnode_t*> const & nodes);
        protected:
            virtual ~xvnodegroup_t();
        private:
            xvnodegroup_t();
            xvnodegroup_t(const xvnodegroup_t &);
            xvnodegroup_t & operator = (const xvnodegroup_t &);
        public:
            inline const xvip2_t &                   get_xip2_addr()     const {return m_group_address;}
            inline const uint64_t                    get_network_height()const {return m_network_height;}
            inline const uint64_t                    get_effect_clock()  const {return m_start_clock_height;}
            //note: it return raw ptr without reference added, so caller need hold xvnodegroup_t until stop using returned ptr
            xvnode_t*                                get_node(const xvip2_t & target_node_xip2) const;
            xvnode_t*                                get_node(const uint32_t node_slot) const; //node_slot must be range of [0,size()-1]
            inline const std::vector<xvnode_t*>&     get_nodes() const {return m_nodes;}//caller need xvnodegroup_t first
            inline const uint32_t                    get_size()  const {return (uint32_t)m_nodes.size();}
        protected:
            xvip2_t                 m_group_address;
            uint64_t                m_start_clock_height;   //when the vnode'xip2 address start effective
            uint64_t                m_network_height;       //election height from xip2
            std::vector<xvnode_t*>  m_nodes;                //store actual nodes
        };
        
        //interface to manage node' key & election result
        class xvnodesrv_t : public xdataobj_t
        {
        public:
            static  const std::string   name(){return "xvnodesrv";} //"xvnodesrv"
            virtual std::string         get_obj_name() const override {return name();}
        protected:
            xvnodesrv_t();
            xvnodesrv_t(enum_xdata_type type);
            virtual ~xvnodesrv_t();
        private:
            xvnodesrv_t(const xvnodesrv_t &);
            xvnodesrv_t & operator = (const xvnodesrv_t &);
        public:
            virtual void*                      query_interface(const int32_t _enum_xobject_type_) override;//caller need to cast (void*) to related ptr
        public:
            virtual xauto_ptr<xvnode_t>        get_node(const xvip2_t & target_node) const = 0;
        public:
            virtual xauto_ptr<xvnodegroup_t>   get_group(const xvip2_t & target_group) const = 0;
            virtual bool                       add_group(const xvnodegroup_t* group_ptr)    = 0;
            virtual bool                       remove_group(const xvip2_t & target_group)   = 0;
        protected:
            virtual int32_t                    do_write(base::xstream_t & stream) override;//write whole object to binary
            virtual int32_t                    do_read(base::xstream_t & stream) override; //read from binary and regeneate content
        };
        
        //xvnodehouse_t is a implemenation for xvnodesrv_t interface
        class xvnodehouse_t : public xvnodesrv_t
        {
        public:
            xvnodehouse_t();
        protected:
            xvnodehouse_t(enum_xdata_type type);
            virtual ~xvnodehouse_t();
        private:
            xvnodehouse_t(const xvnodehouse_t &);
            xvnodehouse_t  & operator = (const xvnodehouse_t &);
        public:
            virtual xauto_ptr<xvnode_t>        get_node(const xvip2_t & target_node) const override;
            virtual xauto_ptr<xvnodegroup_t>   get_group(const xvip2_t & target_group) const override;
            virtual bool                       add_group(const xvnodegroup_t* group_ptr)   override;
            virtual bool                       remove_group(const xvip2_t & target_group)  override;
        private:
            mutable std::mutex                 m_lock;
            uint64_t                           m_vnetwork_id; //network id,refer definition of xip2 at xbase.h
            uint64_t                           m_vnet_version;//version is same concept as round of election
            std::map<uint64_t,xvnodegroup_t*>  m_vgroups;     //mapping <version/round --> group>
        };
        
        struct xvoter
        {
        public:
            xvoter()
            {
                xip_addr.high_addr = 0;
                xip_addr.low_addr  = 0;
                is_voted    = false;
                is_leader   = false;
            }
            xvoter(const std::string & _account,const xvip2_t & _xip_addr,bool _is_voted,bool _is_leader)
            {
                account     = _account;
                xip_addr    = _xip_addr;
                is_voted    = _is_voted;
                is_leader   = _is_leader;
            }
            xvoter(const xvoter & obj)
            {
                *this = obj;
            }
            xvoter & operator = (const xvoter & obj)
            {
                account   = obj.account;
                xip_addr  = obj.xip_addr;
                is_voted  = obj.is_voted;
                is_leader = obj.is_leader;
                return *this;
            }
        public:
            std::string  account;
            xvip2_t      xip_addr;
            bool         is_voted;
            bool         is_leader;
        };
    }//end of namespace of base
}//end of namespace top
