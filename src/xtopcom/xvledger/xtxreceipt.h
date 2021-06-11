// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>

#include "xbasic/xversion.h"
#include "xvledger/xdataobj_base.hpp"
#include "xvledger/xprovecert.h"
#include "xvledger/xmerkle.hpp"
#include "xvledger/xvaction.h"

namespace top
{
    namespace base
    {

        class xtx_receipt_t : public xbase_dataunit_t<xtx_receipt_t, xdata_type_tx_receipt> {
        public:
            xtx_receipt_t();
            xtx_receipt_t(const base::xvaction_t & txaction, const base::xmerkle_path_256_t & path, base::xvqcert_t* cert);
        protected:
            virtual ~xtx_receipt_t();
        private:
            xtx_receipt_t(const xtx_receipt_t &);
            xtx_receipt_t & operator = (const xtx_receipt_t &);

            int32_t do_write(base::xstream_t & stream) override;
            int32_t do_read(base::xstream_t & stream) override;

        public:
            void                            set_tx_prove(base::xvqcert_t* prove_cert, xprove_cert_class_t _class, xprove_cert_type_t _type, const base::xmerkle_path_256_t & path);
            void                            set_commit_prove_with_parent_cert(base::xvqcert_t* prove_cert, const std::string & _path);
            void                            set_commit_prove_with_self_cert(base::xvqcert_t* prove_cert);
            void                            set_commit_prove_cert(base::xvqcert_t* prove_cert, xprove_cert_class_t _class, xprove_cert_type_t _type, const std::string & _path);
            bool                            is_commit_prove_cert_set() const {return m_commit_prove != nullptr;}
            bool                            is_valid();
            const base::xvqcert_t*          get_unit_cert() const {return m_tx_info_prove->get_prove_cert();}
            const xprove_cert_t*            get_tx_info_prove() const {return m_tx_info_prove;}
            const xprove_cert_t*            get_commit_prove() const {return m_commit_prove;}
            const std::string &             get_tx_hash() const {return m_tx_action.get_org_tx_hash();}
            base::enum_transaction_subtype  get_tx_subtype() const {return (base::enum_transaction_subtype)m_tx_action.get_org_tx_action_id();}
            bool                            is_recv_tx() const {return get_tx_subtype() == base::enum_transaction_subtype_recv;}
            bool                            is_confirm_tx() const {return get_tx_subtype() == base::enum_transaction_subtype_confirm;}
            std::string                     get_tx_result_property(const std::string & key);

        private:
            base::xvaction_t        m_tx_action;
            xprove_cert_t*          m_tx_info_prove{nullptr};
            xprove_cert_t*          m_commit_prove{nullptr};
        };

        using xtx_receipt_ptr_t = xobject_ptr_t<xtx_receipt_t>;

    }  // namespace base
}  // namespace top
