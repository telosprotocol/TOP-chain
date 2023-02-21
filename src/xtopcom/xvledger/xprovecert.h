// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>

#include "xvledger/xdataobj_base.hpp"
#include "xvledger/xvblock.h"

namespace top
{
    namespace base
    {
        enum enum_xprove_cert_type {
            enum_xprove_cert_type_unit_justify  = 0,  // unit cert with justify hash
            enum_xprove_cert_type_table_justify = 1,  // table cert with justify hash

            enum_xprove_cert_type_max
        };

        class xprove_cert_t : public xbase_dataunit_t<xprove_cert_t, xdata_type_prove_cert> {
        public:
            xprove_cert_t();
            xprove_cert_t(base::xvqcert_t* prove_cert, enum_xprove_cert_type prove_type, const std::string & _path);

            int32_t get_object_size() const;
        protected:
            virtual ~xprove_cert_t();
        private:
            xprove_cert_t(const xprove_cert_t &);
            xprove_cert_t & operator = (const xprove_cert_t &);

            int32_t do_write(base::xstream_t & stream) override;
            int32_t do_read(base::xstream_t & stream) override;

        public:
            enum_xprove_cert_type   get_prove_type() const {return (enum_xprove_cert_type)(m_prove_type);}
            const xobject_ptr_t<base::xvqcert_t> &  get_prove_cert() const {return m_prove_cert;}
            const std::string &     get_prove_path() const {return m_prove_path;}
            std::string             get_prove_root_hash() const;
            bool                    is_valid() const;
        private:
            xobject_ptr_t<base::xvqcert_t>  m_prove_cert{nullptr};
            uint8_t                         m_prove_type{0};
            std::string                     m_prove_path;
        };

    }  // namespace base
}  // namespace top
