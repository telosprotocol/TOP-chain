// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsigndata.h"

namespace top
{
    namespace auth
    {
        xnodebitset::xnodebitset()
        {
        }
        
        xnodebitset::xnodebitset(const uint16_t alloc_bits_count)
            :base_class(alloc_bits_count)
        {
        }
        
        xnodebitset::xnodebitset(const xnodebitset & obj)
            :base_class(obj)
        {
        }
        xnodebitset::~xnodebitset()
        {
        };
        
        int32_t     xnodebitset::do_write(base::xstream_t & stream)     //write whole object to binary
        {
            const int32_t begin_size = stream.size();
            
            const uint16_t total_alloc_bits = base_class::get_alloc_bits();
            stream << total_alloc_bits;
            stream.push_back(base_class::data(), base_class::get_alloc_bytes());
            
            return (stream.size() - begin_size);
        }
        int32_t     xnodebitset::do_read(base::xstream_t & stream)      //read from binary and regeneate content
        {
            const int32_t begin_size = stream.size();
            
            uint16_t total_alloc_bits = 0;
            stream >> total_alloc_bits;
            if(total_alloc_bits <= base_class::enum_const_max_bits_count)
            {
                base_class::alloc_bits(total_alloc_bits);
                if(stream.size() >= base_class::get_alloc_bytes())
                {
                    memcpy(base_class::data(), stream.data(), base_class::get_alloc_bytes());
                    stream.pop_front(get_alloc_bytes());
                }
                else
                {
                    xerror("xnodebitset::do_read,stream dont ha%d",total_alloc_bits);
                }
            }
            else
            {
                xerror("xnodebitset::do_read,read bad bits count=%d",total_alloc_bits);
            }
            return (begin_size - stream.size());
        }
    
        xmutisigdata_t::xmutisigdata_t()
            :base::xdataunit_t(enum_xdata_type_mutisigndata)
        {
            m_mutisig_token     = 0;
            m_mutisig_node_bits = NULL;
            m_mutisig_node_bits = new xnodebitset();
        }
        //total_signer_count must be less than const_max_nodes_count_of_group
        xmutisigdata_t::xmutisigdata_t(const std::string & rand_ecpoint, const std::string & muti_sig_seal,const uint16_t total_signer_count,const uint64_t mutisig_token)
            :base::xdataunit_t(enum_xdata_type_mutisigndata)
        {
            m_mutisig_token = mutisig_token;
            m_mutisig_node_bits = NULL;
            m_mutisig_node_bits = new xnodebitset(total_signer_count);
            m_mutisig_ecpoint   = rand_ecpoint;
            m_mutisig_seal      = muti_sig_seal;
        }
        xmutisigdata_t::~xmutisigdata_t()
        {
            if(m_mutisig_node_bits != NULL)
                delete m_mutisig_node_bits;
        }
        
        int32_t     xmutisigdata_t::do_write(base::xstream_t & stream)     //write whole object to binary
        {
            const int32_t begin_size = stream.size();
            
            stream << m_mutisig_token;
            stream.write_short_string(m_mutisig_ecpoint);
            stream.write_short_string(m_mutisig_seal);
            m_mutisig_node_bits->do_write(stream);
            
            return (stream.size() - begin_size);
        }
        int32_t     xmutisigdata_t::do_read(base::xstream_t & stream)      //read from binary and regeneate content
        {
            const int32_t begin_size = stream.size();
            
            stream >> m_mutisig_token;
            stream.read_short_string(m_mutisig_ecpoint);
            stream.read_short_string(m_mutisig_seal);
            m_mutisig_node_bits->do_read(stream);
            
            return (begin_size - stream.size());
        }
        
        int32_t    xmutisigdata_t::serialize_from_string(const std::string & _data) //wrap function fo serialize_to(stream)
        {
            base::xstream_t _stream(base::xcontext_t::instance(),(uint8_t*)_data.data(),(uint32_t)_data.size());
            const int result = base::xdataunit_t::serialize_from(_stream);
            return result;
        }
        
        std::string xmutisigdata_t::serialize_to_string() //wrap function fo serialize_to(stream)
        {
            base::xautostream_t<2048> _stream(base::xcontext_t::instance());
            const int result = base::xdataunit_t::serialize_to(_stream);
            if(result > 0)
                return std::string((const char*)_stream.data(),_stream.size());
            
            return std::string();

        }
    }; //end of namespace of auth
};//end of namesapce of top
