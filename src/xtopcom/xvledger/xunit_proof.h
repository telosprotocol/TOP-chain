// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
 
#include <mutex>
#include "xbasic/xserialize_face.h"
#include "xprovecert.h"

namespace top
{
    namespace base
    {
        class xunit_proof_t : public top::basic::xserialize_face_t {
        public:
            xunit_proof_t() {
                
            }
            xunit_proof_t(uint64_t height, uint64_t viewid, base::xvqcert_t* prove_cert) : m_height(height), m_viewid(viewid) {
                m_unit_prove = make_object_ptr<xprove_cert_t>(prove_cert, enum_xprove_cert_type_table_justify, "");
            }

            uint64_t get_height() const {
                return m_height;
            }

            uint64_t get_viewid() const {
                return m_viewid;
            }

            bool verify_unit_block(xobject_ptr_t<base::xvblock_t> unit_block) {
                if (!m_unit_prove->is_valid()) {
                    xerror("xunit_proof_t::verify_unit_block prove invalid.unit:%s",unit_block->dump().c_str());
                }

                base::xauto_ptr<base::xvqcert_t> extend_cert(base::xvblock_t::create_qcert_object(unit_block->get_cert()->get_extend_cert()));
                if(extend_cert == nullptr)
                {
                    xerror("xunit_proof_t::verify_unit_block,fail-invalid extend cert carried by cert:%s",unit_block->dump().c_str());
                    return false;
                }

                const std::string & unit_input_root_hash = extend_cert->get_input_root_hash();
                const std::string & justify_cert_hash = m_unit_prove->get_prove_cert()->get_justify_cert_hash();
                if (justify_cert_hash != unit_input_root_hash) {
                    xerror("xunit_proof_t::verify_unit_block,justify cert hash not match unit:%s",unit_block->dump().c_str());
                    return false;
                }
                return true;
            }
            
        protected:
            int32_t do_write(base::xstream_t & stream) {
                const int32_t begin_size = stream.size();
                stream.write_compact_var(m_height);
                stream.write_compact_var(m_viewid);
                m_unit_prove->serialize_to(stream);
                return (stream.size() - begin_size);
            }

            int32_t do_read(base::xstream_t & stream) {
                const int32_t begin_size = stream.size();
                stream.read_compact_var(m_height);
                stream.read_compact_var(m_viewid);
                m_unit_prove = make_object_ptr<xprove_cert_t>();
                m_unit_prove->serialize_from(stream);
                return (begin_size - stream.size());
            }
        
        private:
            uint64_t m_height{0};
            uint64_t m_viewid{0};
            xobject_ptr_t<xprove_cert_t> m_unit_prove{nullptr};
        };
        
    }//end of namespace of base

}//end of namespace top
