// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xauthscheme.h"
#include "xbase/xint.h"
#include "xbase/xdata.h"

namespace top
{
    namespace auth
    {
        //each group has max amount of nodes is 1024,refer xip definition at xbase.h
        class xnodebitset : public base::xvarbitset<const_max_nodes_count_of_group>
        {
            friend class xmutisigdata_t;
            typedef base::xvarbitset<const_max_nodes_count_of_group> base_class;
        public:
            xnodebitset(const uint16_t alloc_bits_count);
            xnodebitset(const xnodebitset & obj);
            ~xnodebitset();
        private:
            xnodebitset(); //only open for xmutisigdata_t when serialize from binary data
            xnodebitset & operator = (const xnodebitset &);
        public:
            int32_t     do_write(base::xstream_t & stream);     //write whole object to binary
            int32_t     do_read(base::xstream_t & stream);      //read from binary and regeneate content
        };
        
        //note:tradditional mutisig is diffierent approach from official threshold signature,but they might treat as one kind of Threshold Signature through a modified version
        class xmutisigdata_t : public base::xdataunit_t
        {
        public:
            xmutisigdata_t();
            xmutisigdata_t(const std::string & rand_ecpoint, const std::string & muti_sig_seal,const uint16_t total_signer_count,const uint64_t mutisig_token);
            virtual ~xmutisigdata_t();
        private:
            xmutisigdata_t(const xmutisigdata_t &);
            xmutisigdata_t & operator = (const xmutisigdata_t &);
            
        public:
            inline const uint64_t      get_mutisig_token()  const {return m_mutisig_token;}
            inline const std::string&  get_mutisig_point()  const {return m_mutisig_ecpoint;}
            inline const std::string&  get_mutisig_seal()   const {return m_mutisig_seal;}
            inline xnodebitset &       get_nodebitset()     const {return *m_mutisig_node_bits;}
            
        public:
            int32_t                    serialize_from_string(const std::string & _data); //wrap function fo serialize_from(stream)
            std::string                serialize_to_string(); //wrap function fo serialize_to(stream)          
            
        private://not safe for multiple threads, do_write & do_read write and read raw data of dataobj
            //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
            virtual int32_t     do_write(base::xstream_t & stream);     //write whole object to binary
            virtual int32_t     do_read(base::xstream_t & stream);      //read from binary and regeneate content
        private:
            uint64_t            m_mutisig_token;        //view# + viewtoken to identify, same for each node
            std::string         m_mutisig_ecpoint;      //random ec point for mutisign,it might be empty for bls
            std::string         m_mutisig_seal;         //combined result of muti-signer' result
            xnodebitset*        m_mutisig_node_bits;    //who(mutiple) sign
        };
        
    }; //end of namespace of auth
};//end of namesapce of top
