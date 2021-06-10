// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../xvboffdata.h"
#include "xmetrics/xmetrics.h"
namespace top
{
    namespace base
    {
        xvboffdata_t::xvboffdata_t()
            :base::xdataunit_t((enum_xdata_type)enum_obj_type)
        {
            // TODO(jimmy)
            XMETRICS_GAUGE(metrics::dataobject_xvboffdata_t, 1);
        }

        xvboffdata_t::~xvboffdata_t()
        {
            XMETRICS_GAUGE(metrics::dataobject_xvboffdata_t, -1);
        }

        //caller respond to cast (void*) to related  interface ptr
        void*   xvboffdata_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(enum_obj_type == _enum_xobject_type_)
                return this;

            return xdataunit_t::query_interface(_enum_xobject_type_);
        }

        int32_t         xvboffdata_t::do_write(base::xstream_t & stream)
        {
            const int32_t begin_size = stream.size();
            xassert(m_propertys.size() < 65535);
            uint16_t count = (uint16_t)m_propertys.size();
            stream.write_compact_var(count);
            for (auto & v : m_propertys) {
                stream.write_compact_var(v.first);
                v.second->serialize_to(stream);
            }
            return (stream.size() - begin_size);
        }

        int32_t         xvboffdata_t::do_read(base::xstream_t & stream)
        {
            const int32_t begin_size = stream.size();
            uint16_t count = 0;
            stream.read_compact_var(count);
            for (uint16_t i = 0; i < count; i++) {
                std::string name;
                stream.read_compact_var(name);
                xdataunit_t* _data = xdataunit_t::read_from(stream);
                xassert(_data != nullptr);
                xobject_ptr_t<xdataunit_t> _data_ptr;
                _data_ptr.attach(_data);
                m_propertys[name] = _data_ptr;
            }
            return (begin_size - stream.size());
        }

        std::string xvboffdata_t::build_root_hash(enum_xhash_type hashtype)
        {
            std::string bin_str;
            serialize_to_string(bin_str);
            return xcontext_t::instance().hash(bin_str, hashtype);
        }

        void xvboffdata_t::set_offdata(const std::string & name, const xobject_ptr_t<xdataunit_t> & data)
        {
            m_propertys[name] = data;
        }

        xobject_ptr_t<xdataunit_t> xvboffdata_t::query_offdata(const std::string & name) const
        {
            auto iter = m_propertys.find(name);
            if (iter != m_propertys.end()) {
                return iter->second;
            }
            xassert(false);
            return nullptr;
        }

    }  //end of namespace base
} //end of namespace top

