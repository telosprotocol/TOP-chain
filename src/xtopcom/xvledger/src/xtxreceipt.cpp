// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xvledger/xtxreceipt.h"

namespace top
{
    namespace base
    {
        REG_CLS(xtx_receipt_t);

        xtx_receipt_t::xtx_receipt_t()
        : m_tx_action({},{},{},"invalid") {  // TODO(jimmy)

        }
        xtx_receipt_t::xtx_receipt_t(const base::xvaction_t & txaction, const base::xmerkle_path_256_t & path, base::xvqcert_t * cert)
        : m_tx_action(txaction) {
            set_tx_prove(cert, xprove_cert_class_self_cert, xprove_cert_type_input_root, path);
        }

        xtx_receipt_t::~xtx_receipt_t() {
            if (m_tx_info_prove != nullptr) {
                m_tx_info_prove->release_ref();
            }
            if (m_commit_prove != nullptr) {
                m_commit_prove->release_ref();
            }
        }

        int32_t xtx_receipt_t::do_write(base::xstream_t & stream) {
            KEEP_SIZE();
            std::string action_bin;
            m_tx_action.serialize_to(action_bin);
            stream.write_compact_var(action_bin);
            m_tx_info_prove->serialize_to(stream);
            m_commit_prove->serialize_to(stream);
            return CALC_LEN();
        }
        int32_t xtx_receipt_t::do_read(base::xstream_t & stream) {
            KEEP_SIZE();
            std::string action_bin;
            stream.read_compact_var(action_bin);
            m_tx_action.serialize_from(action_bin);
            m_tx_info_prove = new xprove_cert_t();
            m_tx_info_prove->serialize_from(stream);
            m_commit_prove = new xprove_cert_t();
            m_commit_prove->serialize_from(stream);
            return CALC_LEN();
        }

        void xtx_receipt_t::set_tx_prove(base::xvqcert_t* prove_cert, xprove_cert_class_t _class, xprove_cert_type_t _type, const base::xmerkle_path_256_t & _path) {
            xassert(m_tx_info_prove == nullptr);
            m_tx_info_prove = new xprove_cert_t(prove_cert, _class, _type, _path);
        }
        void xtx_receipt_t::set_commit_prove_cert(base::xvqcert_t* prove_cert, xprove_cert_class_t _class, xprove_cert_type_t _type, const std::string & _path) {
            xassert(m_commit_prove == nullptr);
            m_commit_prove = new xprove_cert_t(prove_cert, _class, _type, _path);
        }
        void xtx_receipt_t::set_commit_prove_with_parent_cert(base::xvqcert_t* prove_cert, const std::string & _path) {
            set_commit_prove_cert(prove_cert, xprove_cert_class_parent_cert, xprove_cert_type_justify_cert, _path);
        }

        void xtx_receipt_t::set_commit_prove_with_self_cert(base::xvqcert_t* prove_cert) {
            set_commit_prove_cert(prove_cert, xprove_cert_class_self_cert, xprove_cert_type_justify_cert, {});
        }

        bool xtx_receipt_t::is_valid() {
            if (get_tx_subtype() != base::enum_transaction_subtype_send && get_tx_subtype() != base::enum_transaction_subtype_recv) {
                xerror("xtx_receipt_t::is_valid not send or recv tx.");
                return false;
            }

            if (m_tx_info_prove == nullptr || m_commit_prove == nullptr) {
                xerror("xtx_receipt_t::is_valid prove cert null.");
                return false;
            }

            std::string tx_info_leaf;
            m_tx_action.serialize_to(tx_info_leaf);
            if (!m_tx_info_prove->is_valid(tx_info_leaf)) {
                xerror("xtx_receipt_t::is_valid tx info prove check fail.");
                return false;
            }

            const std::string unit_leaf = m_tx_info_prove->get_prove_cert()->get_hash_to_sign();
            if (!m_commit_prove->is_valid(unit_leaf)) {
                xerror("xtx_receipt_t::is_valid commit prove check fail.");
                return false;
            }

            return true;
        }

        std::string xtx_receipt_t::get_tx_result_property(const std::string & key) {
            const std::map<std::string,std::string>* map_ptr = m_tx_action.get_method_result()->get_map<std::string>();
            if (map_ptr != nullptr) {
                auto iter = map_ptr->find(key);
                if (iter != map_ptr->end()) {
                    return iter->second;
                }
                return {};
            }
            xassert(false);
            return {};
        }

    }  // namespace base
}  // namespace top
