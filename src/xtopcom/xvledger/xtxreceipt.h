// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>

#include "xbasic/xversion.h"
#include "xbasic/xbyte_buffer.h"
#include "xvledger/xdataobj_base.hpp"
#include "xvledger/xprovecert.h"
#include "xvledger/xmerkle.hpp"
#include "xvledger/xvaction.h"
#include "xstatistic/xstatistic.h"

namespace top
{
    namespace base
    {
        class xtxreceipt_build_t;
        class xtx_receipt_t : public xbase_dataunit_t<xtx_receipt_t, xdata_type_tx_receipt>, public xstatistic::xstatistic_obj_face_t {
            friend class xtxreceipt_build_t;
        public:
            xtx_receipt_t();
            xtx_receipt_t(const base::xvaction_t & txaction);  // XTODO(jimmy) no prove receipt for local usage
            xtx_receipt_t(const base::xvaction_t & txaction, base::xvqcert_t* prove_cert, const std::string & path, enum_xprove_cert_type type);            
            virtual ~xtx_receipt_t();
        private:
            xtx_receipt_t(const xtx_receipt_t &);
            xtx_receipt_t & operator = (const xtx_receipt_t &);

            int32_t do_write(base::xstream_t & stream) override;
            int32_t do_read(base::xstream_t & stream) override;

        protected:
            static std::string              merkle_path_to_string(const base::xmerkle_path_256_t & path);
        public:
            bool                            is_valid() const;
            enum_xprove_cert_type           get_prove_type() const {return m_tx_action_prove->get_prove_type();}
            const xobject_ptr_t<base::xvqcert_t> &  get_prove_cert() const {return m_tx_action_prove->get_prove_cert();}
            const std::string &             get_tx_hash() const {return m_tx_action.get_org_tx_hash();}
            enum_transaction_subtype        get_tx_subtype() const {return (base::enum_transaction_subtype)m_tx_action.get_org_tx_action_id();}
            bool                            is_recv_tx() const {return get_tx_subtype() == base::enum_transaction_subtype_recv;}
            bool                            is_confirm_tx() const {return get_tx_subtype() == base::enum_transaction_subtype_confirm;}
            std::string                     get_tx_result_property(const std::string & key) const;
            std::string                     get_contract_address() const;
            std::string                     get_caller() const {return m_tx_action.get_caller();}
            const base::xvaction_t &        get_action() const {return m_tx_action;}

            virtual int32_t get_class_type() const override {return xstatistic::enum_statistic_receipt;}

        private:
            size_t get_object_size_real() const override;

        private:
            base::xvaction_t                m_tx_action;
            xobject_ptr_t<xprove_cert_t>    m_tx_action_prove{nullptr};
        };

        using xtx_receipt_ptr_t = xobject_ptr_t<xtx_receipt_t>;

        class xfull_txreceipt_t {
        public:
            xfull_txreceipt_t(const xtx_receipt_ptr_t & tx_receipt, const std::string & tx_org_bin)
            : m_tx_receipt(tx_receipt), m_tx_org_bin(tx_org_bin) {}
            const xtx_receipt_ptr_t &   get_txreceipt() const {return m_tx_receipt;}
            const std::string &         get_tx_org_bin() const {return m_tx_org_bin;}
        private:
            xtx_receipt_ptr_t       m_tx_receipt{nullptr};
            std::string             m_tx_org_bin;
        };
        using xfull_txreceipt_ptr_t = std::shared_ptr<xfull_txreceipt_t>;

        class xtxreceipt_build_t {
        public:
            // static std::vector<xfull_txreceipt_t>    create_all_txreceipts(xvblock_t* commit_block, xvblock_t* cert_block);
            // static std::vector<xfull_txreceipt_t>    create_all_txreceipts(xvblock_t* commit_block, xvblock_t* cert_block, const std::vector<xvaction_t> & actions);
            // static xfull_txreceipt_ptr_t             create_one_txreceipt(xvblock_t* commit_block, xvblock_t* cert_block, const std::string & txhash);

            static xtx_receipt_ptr_t                 create_table_input_primary_action_receipt(xvblock_t* commit_block, xvblock_t* cert_block);
        };
    }  // namespace base
}  // namespace top
