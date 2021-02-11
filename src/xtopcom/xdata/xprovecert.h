// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>

#include "xbasic/xversion.h"
#include "xbasic/xdataobj_base.hpp"
#include "xbase/xvblock.h"
#include "xutility/xmerkle.hpp"

namespace top { namespace data {

enum xprove_cert_class_t {
    xprove_cert_class_self_cert = 0,
    xprove_cert_class_parent_cert = 1,
};

enum xprove_cert_type_t {
    xprove_cert_type_output_root = 0,
    xprove_cert_type_justify_cert = 1,
};

class xprove_cert_t : public xbase_dataunit_t<xprove_cert_t, xdata_type_prove_cert> {
 public:
    xprove_cert_t() = default;
    xprove_cert_t(base::xvqcert_t* prove_cert, xprove_cert_class_t _class, xprove_cert_type_t _type, const xmerkle_path_256_t & _path);
    xprove_cert_t(base::xvqcert_t* prove_cert, xprove_cert_class_t _class, xprove_cert_type_t _type, const std::string & _path);
 protected:
    virtual ~xprove_cert_t();
 private:
    xprove_cert_t(const xprove_cert_t &);
    xprove_cert_t & operator = (const xprove_cert_t &);

    int32_t do_write(base::xstream_t & stream) override;
    int32_t do_read(base::xstream_t & stream) override;

 public:
    xprove_cert_class_t get_prove_class() const {return (xprove_cert_class_t)((m_prove_type >> 4) & 0x0F);}
    xprove_cert_type_t  get_prove_type() const {return (xprove_cert_type_t)(m_prove_type & 0x0F);}
    base::xvqcert_t*    get_prove_cert() const {return m_prove_cert;}
    void    set_prove_class(xprove_cert_class_t _class) {m_prove_type = (m_prove_type & 0x0F) | (_class << 4);}
    void    set_prove_type(xprove_cert_type_t _type) {m_prove_type = (m_prove_type & 0xF0) | (_type & 0x0F);}
    void    set_prove_path(const xmerkle_path_256_t & path);
    bool    is_valid(const std::string & prove_object);
 private:
    base::xvqcert_t*    m_prove_cert{nullptr};
    uint8_t             m_prove_type{0};
    std::string         m_prove_path;
};


}  // namespace data
}  // namespace top
