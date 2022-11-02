// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xprovecert.h"
#include "xvledger/xvcertauth.h"
#include "xvledger/xvaccount.h"

#include <mutex>

namespace top {
namespace base {
class xunit_proof_t {
public:
    xunit_proof_t() {
    }
    xunit_proof_t(uint64_t height, uint64_t viewid, base::xvqcert_t * prove_cert) : m_height(height), m_viewid(viewid) {
        m_unit_prove = make_object_ptr<xprove_cert_t>(prove_cert, enum_xprove_cert_type_table_justify, "");
    }

    uint64_t get_height() const {
        return m_height;
    }

    uint64_t get_viewid() const {
        return m_viewid;
    }

    bool verify_unit_block(base::xvcertauth_t * certauth, xobject_ptr_t<base::xvblock_t> unit_block) {
        if (!m_unit_prove->is_valid()) {
            xerror("xunit_proof_t::verify_unit_block prove invalid.unit:%s", unit_block->dump().c_str());
            return false;
        }

        base::xauto_ptr<base::xvqcert_t> table_qcert(base::xvblock_t::create_qcert_object(unit_block->get_cert()->get_extend_cert()));
        if (table_qcert == nullptr) {
            xerror("xunit_proof_t::verify_unit_block,fail-invalid extend cert carried by cert:%s", unit_block->dump().c_str());
            return false;
        }

        std::string account = unit_block->get_account();
        std::string table_account = base::xvaccount_t::make_table_account_address(base::xvaccount_t(account));

        base::enum_vcert_auth_result auth_result = certauth->verify_muti_sign(table_qcert.get(), table_account);
        if (auth_result != base::enum_vcert_auth_result::enum_successful) {
            xerror("xunit_proof_t::verify_unit_block,fail-verify muti sign:%s", unit_block->dump().c_str());
            return false;
        }

        const std::string & table_qcert_input_root_hash = table_qcert->get_input_root_hash();
        const std::string & justify_cert_hash = m_unit_prove->get_prove_cert()->get_justify_cert_hash();
        if (justify_cert_hash != table_qcert_input_root_hash) {
            xerror("xunit_proof_t::verify_unit_block,justify cert hash not match unit:%s", unit_block->dump().c_str());
            return false;
        }
        return true;
    }

    int32_t do_write(base::xstream_t & stream) const {
        const int32_t begin_size = stream.size();
        stream.write_compact_var(m_height);
        stream.write_compact_var(m_viewid);
        m_unit_prove->serialize_to(stream);
        stream.write_compact_var(m_proof_flag);
        return (stream.size() - begin_size);
    }

    int32_t do_read(base::xstream_t & stream) {
        const int32_t begin_size = stream.size();
        stream.read_compact_var(m_height);
        stream.read_compact_var(m_viewid);
        m_unit_prove = make_object_ptr<xprove_cert_t>();
        m_unit_prove->serialize_from(stream);
        stream.read_compact_var(m_proof_flag);
        return (begin_size - stream.size());
    }

    int32_t serialize_to(std::string & bin_data) const {
        base::xautostream_t<1024> _stream(base::xcontext_t::instance());
        int32_t result = do_write(_stream);
        if (result > 0)
            bin_data.assign((const char *)_stream.data(), _stream.size());
        xassert(result > 0);
        return result;
    }

    int32_t serialize_from(const std::string & bin_data) {
        base::xstream_t _stream(base::xcontext_t::instance(), (uint8_t *)bin_data.data(), (uint32_t)bin_data.size());
        int32_t result = do_read(_stream);
        xassert(result > 0);
        return result;
    }

private:
    uint64_t m_height{0};
    uint64_t m_viewid{0};
    xobject_ptr_t<xprove_cert_t> m_unit_prove{nullptr};
    uint16_t m_proof_flag{0};
};

}  // namespace base

}  // end of namespace top
