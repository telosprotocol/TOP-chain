// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>

#include "xbasic/xversion.h"
#include "xbasic/xdataobj_base.hpp"
#include "xbase/xvblock.h"
#include "xbase/xvledger.h"
#include "xdata/xprovecert.h"

namespace top { namespace data {

xprove_cert_t::xprove_cert_t(base::xvqcert_t* prove_cert, xprove_cert_class_t _class, xprove_cert_type_t _type, const xmerkle_path_256_t & _path) {
    m_prove_cert = prove_cert;
    m_prove_cert->add_ref();
    set_prove_class(_class);
    set_prove_type(_type);
    set_prove_path(_path);
}

xprove_cert_t::xprove_cert_t(base::xvqcert_t* prove_cert, xprove_cert_class_t _class, xprove_cert_type_t _type, const std::string & _path) {
    m_prove_cert = prove_cert;
    m_prove_cert->add_ref();
    set_prove_class(_class);
    set_prove_type(_type);
    m_prove_path = _path;
}

xprove_cert_t::~xprove_cert_t() {
    if (m_prove_cert != nullptr) {
        m_prove_cert->release_ref();
    }
}

int32_t xprove_cert_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    std::string prove_cert_bin;
    xassert(m_prove_cert != nullptr);
    m_prove_cert->serialize_to_string(prove_cert_bin);
    xassert(!prove_cert_bin.empty());
    stream << prove_cert_bin;
    stream << m_prove_type;
    stream << m_prove_path;
    return CALC_LEN();
}
int32_t xprove_cert_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    std::string cert_bin;
    stream >> cert_bin;
    m_prove_cert = base::xvblockstore_t::create_qcert_object(cert_bin);
    if (m_prove_cert == nullptr) {
        xassert(0);
        return -1;
    }
    stream >> m_prove_type;
    stream >> m_prove_path;
    return CALC_LEN();
}

void xprove_cert_t::set_prove_path(const xmerkle_path_256_t & path) {
    base::xstream_t stream2(base::xcontext_t::instance());
    path.serialize_to(stream2);
    m_prove_path = std::string((char *)stream2.data(), stream2.size());
    xassert(!m_prove_path.empty());
}

bool xprove_cert_t::is_valid(const std::string & prove_object) {
    if (m_prove_cert == nullptr) {
        xerror("xprove_cert_t::is_valid prove cert null.");
        return false;
    }

    xprove_cert_type_t cert_type = get_prove_type();
    if (cert_type != xprove_cert_type_output_root
        && cert_type != xprove_cert_type_justify_cert) {
        xerror("xprove_cert_t::is_valid prove type not valid. type=%d", cert_type);
        return false;
    }

    xprove_cert_class_t cert_class = get_prove_class();
    if (cert_class != xprove_cert_class_self_cert
        && cert_class != xprove_cert_class_parent_cert) {
        xerror("xprove_cert_t::is_valid prove class not valid. class=%d", cert_class);
        return false;
    }

    std::string root_hash;
    if (cert_type == xprove_cert_type_output_root) {
        root_hash = m_prove_cert->get_output_root_hash();
    } else if (cert_type == xprove_cert_type_justify_cert) {
        root_hash = m_prove_cert->get_justify_cert_hash();
    } else {
        xassert(0);
    }

    if (!m_prove_path.empty()) {
        xmerkle_path_256_t path;
        base::xstream_t _stream(base::xcontext_t::instance(), (uint8_t *)m_prove_path.data(), (uint32_t)m_prove_path.size());
        int32_t ret = path.serialize_from(_stream);
        if (ret <= 0) {
            xerror("xprove_cert_t::is_valid deserialize merkle path fail. ret=%d", ret);
            return false;
        }
        xmerkle_t<utl::xsha2_256_t, uint256_t> merkle;
        if (!merkle.validate_path(prove_object, root_hash, path.m_levels)) {
            xerror("xprove_cert_t::is_valid check merkle path fail.");
            return false;
        }
    } else {
        if (prove_object != root_hash) {
            xerror("xprove_cert_t::is_valid check prove object not equal with root fail");
            return false;
        }
    }

    return true;
}

}  // namespace data
}  // namespace top
