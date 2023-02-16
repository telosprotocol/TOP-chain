// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>

#include "xstatistic/xbasic_size.hpp"
#include "xvledger/xprovecert.h"
#include "xmetrics/xmetrics.h"

namespace top
{
    namespace base
    {
        xprove_cert_t::xprove_cert_t() {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_provcert, 1);
        }
        xprove_cert_t::xprove_cert_t(base::xvqcert_t* prove_cert, enum_xprove_cert_type prove_type, const std::string & _path) {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_provcert, 1);
            prove_cert->add_ref();
            m_prove_cert.attach(prove_cert);
            m_prove_type =  prove_type;
            m_prove_path = _path;
        }

        xprove_cert_t::~xprove_cert_t() {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_provcert, -1);
        }

        int32_t xprove_cert_t::do_write(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            std::string prove_cert_bin;
            xassert(m_prove_cert != nullptr);
            if (m_prove_cert != nullptr) {
                m_prove_cert->serialize_to_string(prove_cert_bin);
            }
            stream << prove_cert_bin;
            stream << m_prove_type;
            stream << m_prove_path;
            return (stream.size() - begin_size);
        }
        int32_t xprove_cert_t::do_read(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            std::string cert_bin;
            stream >> cert_bin;
            xvqcert_t* _qcert = base::xvblock_t::create_qcert_object(cert_bin);
            if (_qcert == nullptr) {
                xassert(0);
                return -1;
            }
            m_prove_cert.attach(_qcert);
            stream >> m_prove_type;
            stream >> m_prove_path;
            return (begin_size - stream.size());
        }

        bool xprove_cert_t::is_valid() const {
            if (m_prove_cert == nullptr) {
                xerror("xprove_cert_t::is_valid prove cert null.");
                return false;
            }

            enum_xprove_cert_type _cert_type = get_prove_type();
            if (_cert_type >=  enum_xprove_cert_type_max) {
                xerror("xprove_cert_t::is_valid invalid cert type %d.", _cert_type);
                return false;
            }
            return true;
        }

        std::string xprove_cert_t::get_prove_root_hash() const {
            std::string root_hash;
            enum_xprove_cert_type cert_type = get_prove_type();
            if (cert_type == enum_xprove_cert_type_unit_justify || cert_type == enum_xprove_cert_type_table_justify) {
                root_hash = m_prove_cert->get_justify_cert_hash();
            } else {
                xerror("xprove_cert_t::is_valid prove type not valid. type=%d", cert_type);
            }
            return root_hash;
        }

        int32_t xprove_cert_t::get_object_size() const {
            int32_t total_size = sizeof(*this);
            // avoid double counting.
            // if (m_prove_cert != nullptr) {
            //     total_size += m_prove_cert->get_object_size();
            // }
            total_size += get_size(m_prove_path);
            xdbg("-----cache size----- xprove_cert_t this:%d,m_prove_path:%d", sizeof(*this), get_size(m_prove_path));
            return total_size;
        }

    }  // namespace base
}  // namespace top
