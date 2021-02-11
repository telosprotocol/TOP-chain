// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>

#include "xbasic/xversion.h"
#include "xbasic/xdataobj_base.hpp"
#include "xdata/xlightunit_info.h"
#include "xdata/xprovecert.h"
#include "xutility/xmerkle.hpp"
namespace top { namespace data {

class xtx_receipt_t : public xbase_dataunit_t<xtx_receipt_t, xdata_type_tx_receipt> {
 public:
    xtx_receipt_t() = default;
    xtx_receipt_t(const xlightunit_output_entity_t* txinfo, const xmerkle_path_256_t & path, base::xvqcert_t* cert);
 protected:
    virtual ~xtx_receipt_t();
 private:
    xtx_receipt_t(const xtx_receipt_t &);
    xtx_receipt_t & operator = (const xtx_receipt_t &);

    int32_t do_write(base::xstream_t & stream) override;
    int32_t do_read(base::xstream_t & stream) override;

 public:
    void                            set_tx_prove(base::xvqcert_t* prove_cert, xprove_cert_class_t _class, xprove_cert_type_t _type, const xmerkle_path_256_t & path);
    void                            set_commit_prove_cert(base::xvqcert_t* prove_cert, xprove_cert_class_t _class, xprove_cert_type_t _type, const std::string & _path);
    bool                            is_commit_prove_cert_set() const {return m_commit_prove != nullptr;}
    bool                            is_valid();
    const xlightunit_output_entity_t*     get_tx_info() const {return m_tx_info;}
    const base::xvqcert_t*          get_unit_cert() const {return m_tx_info_prove->get_prove_cert();}
    const xprove_cert_t*            get_tx_info_prove() const {return m_tx_info_prove;}
    const xprove_cert_t*            get_commit_prove() const {return m_commit_prove;}

 private:
    xlightunit_output_entity_t*   m_tx_info{nullptr};
    xprove_cert_t*          m_tx_info_prove{nullptr};
    xprove_cert_t*          m_commit_prove{nullptr};
};

using xtx_receipt_ptr_t = xobject_ptr_t<xtx_receipt_t>;

}  // namespace data
}  // namespace top
