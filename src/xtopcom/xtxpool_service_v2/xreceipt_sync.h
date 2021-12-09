// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xserialize_face.h"
#include "xdata/xcons_transaction.h"

#include <vector>

NS_BEG2(top, xtxpool_service_v2)

class xreceipt_pull_receipt_t : public top::basic::xserialize_face_t {
public:
    xreceipt_pull_receipt_t() {
    }
    xreceipt_pull_receipt_t(const common::xnode_address_t & req_node,
                            const std::string & tx_from_account,
                            const std::string & tx_to_account,
                            const std::vector<uint64_t> & receipt_ids,
                            uint32_t copy_id_num)
      : m_req_node(req_node), m_tx_from_account(tx_from_account), m_tx_to_account(tx_to_account) {
        uint32_t num = (copy_id_num < receipt_ids.size()) ? copy_id_num : receipt_ids.size();
        m_receipt_ids.insert(m_receipt_ids.end(), receipt_ids.begin(), receipt_ids.begin() + num);
    }

protected:
    int32_t do_write(base::xstream_t & stream) override {
        KEEP_SIZE();

        SERIALIZE_FIELD_BT(m_req_node);
        SERIALIZE_FIELD_BT(m_tx_from_account);
        SERIALIZE_FIELD_BT(m_tx_to_account);
        SERIALIZE_FIELD_BT(m_receipt_ids);

        return CALC_LEN();
    }

    int32_t do_read(base::xstream_t & stream) override {
        KEEP_SIZE();
        DESERIALIZE_FIELD_BT(m_req_node);
        DESERIALIZE_FIELD_BT(m_tx_from_account);
        DESERIALIZE_FIELD_BT(m_tx_to_account);
        DESERIALIZE_FIELD_BT(m_receipt_ids);
        // restore padding
        return CALC_LEN();
    }

public:
    common::xnode_address_t m_req_node;
    std::string m_tx_from_account;
    std::string m_tx_to_account;
    std::vector<uint64_t> m_receipt_ids;
};

class xreceipt_push_t : public top::basic::xserialize_face_t {
public:
protected:
    int32_t do_write(base::xstream_t & stream) override {
        KEEP_SIZE();

        SERIALIZE_CONTAINER(m_receipts) {
            item->serialize_to(stream);
        }

        SERIALIZE_FIELD_BT(m_tx_from_account);
        SERIALIZE_FIELD_BT(m_tx_to_account);
        SERIALIZE_FIELD_BT(m_req_node);
        SERIALIZE_FIELD_BT(m_receipt_type);

        return CALC_LEN();
    }

    int32_t do_read(base::xstream_t & stream) override {
        try {
            KEEP_SIZE();

            DESERIALIZE_CONTAINER(m_receipts) {
                data::xcons_transaction_ptr_t receipt = make_object_ptr<data::xcons_transaction_t>();
                receipt->serialize_from(stream);

                if (receipt != nullptr)
                    m_receipts.push_back(receipt);
            }

            DESERIALIZE_FIELD_BT(m_tx_from_account);
            DESERIALIZE_FIELD_BT(m_tx_to_account);
            DESERIALIZE_FIELD_BT(m_req_node);
            DESERIALIZE_FIELD_BT(m_receipt_type);

            return CALC_LEN();
        } catch (...) {
            m_tx_from_account = "";
            m_tx_to_account = "";
            m_receipt_type = base::enum_transaction_subtype::enum_transaction_subtype_invalid;
            m_receipts.clear();
        }

        return 0;
    }

public:
    std::vector<data::xcons_transaction_ptr_t> m_receipts;
    std::string m_tx_from_account;
    std::string m_tx_to_account;
    common::xnode_address_t m_req_node;
    base::enum_transaction_subtype m_receipt_type;
};

// class xneighbor_sync_req_t : public top::basic::xserialize_face_t {
// protected:
//     int32_t do_write(base::xstream_t & stream) override {
//         KEEP_SIZE();

//         SERIALIZE_FIELD_BT(m_table_sid);

//         return CALC_LEN();
//     }

//     int32_t do_read(base::xstream_t & stream) override {
//         KEEP_SIZE();
//         DESERIALIZE_FIELD_BT(m_table_sid);
//         // restore padding
//         return CALC_LEN();
//     }

// public:
//     base::xtable_shortid_t m_table_sid;
// };

// class xneighbor_sync_rsp_t : public top::basic::xserialize_face_t {
// protected:
//     int32_t do_write(base::xstream_t & stream) override {
//         KEEP_SIZE();

//         SERIALIZE_CONTAINER(m_receipts) {
//             item->serialize_to(stream);
//         }

//         return CALC_LEN();
//     }

//     int32_t do_read(base::xstream_t & stream) override {
//         try {
//             KEEP_SIZE();

//             DESERIALIZE_CONTAINER(m_receipts) {
//                 data::xcons_transaction_ptr_t receipt = make_object_ptr<data::xcons_transaction_t>();
//                 receipt->serialize_from(stream);

//                 if (receipt != nullptr)
//                     m_receipts.push_back(receipt);
//             }

//             return CALC_LEN();
//         } catch (...) {
//             m_receipts.clear();
//         }
//     }

// public:
//     std::vector<data::xcons_transaction_ptr_t> m_receipts;
// };

NS_END2
