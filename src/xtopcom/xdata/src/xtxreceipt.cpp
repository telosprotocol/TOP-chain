// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xdata/xtxreceipt.h"
#include "xdata/xdata_common.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xdata/xlightunit.h"

namespace top {
namespace data {

REG_CLS(xtx_receipt_t);

xtx_receipt_t::xtx_receipt_t(const xlightunit_output_entity_t* txinfo, const xmerkle_path_256_t & path, base::xvqcert_t* cert) : xtx_receipt_t{txinfo, {}, path, cert} {
}

xtx_receipt_t::xtx_receipt_t(const xlightunit_output_entity_t * txinfo, std::map<std::string, xbyte_buffer_t> data, const xmerkle_path_256_t & path, base::xvqcert_t * cert)
  : m_receipt_data{std::move(data)} {
    m_tx_info = (xlightunit_output_entity_t *)txinfo;
    m_tx_info->add_ref();

    set_tx_prove(cert, xprove_cert_class_self_cert, xprove_cert_type_output_root, path);

#if defined(DEBUG)
    for (auto const & d : m_receipt_data) {
        xdbg("xtx_receipt_t: %s:%s", d.first.c_str(), std::string{std::begin(d.second), std::end(d.second)}.c_str());
    }
#endif
}

xtx_receipt_t::~xtx_receipt_t() {
    if (m_tx_info != nullptr) {
        m_tx_info->release_ref();
    }
    if (m_tx_info_prove != nullptr) {
        m_tx_info_prove->release_ref();
    }
    if (m_commit_prove != nullptr) {
        m_commit_prove->release_ref();
    }
}

int32_t xtx_receipt_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    m_tx_info->serialize_to(stream);
    m_tx_info_prove->serialize_to(stream);
    m_commit_prove->serialize_to(stream);
    MAP_SERIALIZE_SIMPLE(stream, m_receipt_data);
#if defined(DEBUG)
    for (auto const & d : m_receipt_data) {
        xdbg("xtx_receipt_t::do_write %s:%s", d.first.c_str(), std::string{std::begin(d.second), std::end(d.second)}.c_str());
    }
#endif
    return CALC_LEN();
}
int32_t xtx_receipt_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    m_tx_info = new xlightunit_output_entity_t();
    m_tx_info->serialize_from(stream);
    m_tx_info_prove = new xprove_cert_t();
    m_tx_info_prove->serialize_from(stream);
    m_commit_prove = new xprove_cert_t();
    m_commit_prove->serialize_from(stream);
    MAP_DESERIALIZE_SIMPLE(stream, m_receipt_data);
#if defined(DEBUG)
    for (auto const & d : m_receipt_data) {
        xdbg("xtx_receipt_t::do_read %s:%s", d.first.c_str(), std::string{std::begin(d.second), std::end(d.second)}.c_str());
    }
#endif
    return CALC_LEN();
}

void xtx_receipt_t::set_tx_prove(base::xvqcert_t* prove_cert, xprove_cert_class_t _class, xprove_cert_type_t _type, const xmerkle_path_256_t & _path) {
    xassert(m_tx_info_prove == nullptr);
    m_tx_info_prove = new xprove_cert_t(prove_cert, _class, _type, _path);
}
void xtx_receipt_t::set_commit_prove_cert(base::xvqcert_t* prove_cert, xprove_cert_class_t _class, xprove_cert_type_t _type, const std::string & _path) {
    xassert(m_commit_prove == nullptr);
    m_commit_prove = new xprove_cert_t(prove_cert, _class, _type, _path);
}

bool xtx_receipt_t::is_valid() {
    if (!(m_tx_info->is_send_tx() || m_tx_info->is_recv_tx())) {
        xerror("xtx_receipt_t::is_valid not send or recv tx.");
        return false;
    }

    if (m_tx_info_prove == nullptr || m_commit_prove == nullptr) {
        xerror("xtx_receipt_t::is_valid prove cert null.");
        return false;
    }

    const std::string tx_info_leaf = m_tx_info->get_merkle_leaf();
    if (!m_tx_info_prove->is_valid(tx_info_leaf)) {
        xerror("xtx_receipt_t::is_valid tx info prove check fail. tx=%s", m_tx_info->get_tx_dump_key().c_str());
        return false;
    }

    const std::string unit_leaf = m_tx_info_prove->get_prove_cert()->get_hash_to_sign();
    if (!m_commit_prove->is_valid(unit_leaf)) {
        xerror("xtx_receipt_t::is_valid commit prove check fail.tx=%s", m_tx_info->get_tx_dump_key().c_str());
        return false;
    }

    xdbg("xtx_receipt_t::is_valid success. tx=%s", m_tx_info->get_tx_dump_key().c_str());
    return true;
}


}  // namespace data
}  // namespace top
