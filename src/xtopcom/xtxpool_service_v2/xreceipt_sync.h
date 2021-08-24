// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <vector>
#include "xdata/xcons_transaction.h"
#include "xbasic/xserialize_face.h"

NS_BEG2(top, xtxpool_service_v2)

const uint32_t max_require_receipts = 15;
class xreceipt_pull_recv_receipt_t : public top::basic::xserialize_face_t {
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

class xreceipt_pull_confirm_receipt_t : public top::basic::xserialize_face_t {
protected:
    int32_t do_write(base::xstream_t & stream) override {
        KEEP_SIZE();

        SERIALIZE_FIELD_BT(m_req_node);
        SERIALIZE_FIELD_BT(m_tx_from_account);
        SERIALIZE_FIELD_BT(m_tx_to_account);
        SERIALIZE_FIELD_BT(m_id_hash_of_receipts);

        return CALC_LEN();
    }

    int32_t do_read(base::xstream_t & stream) override {

        KEEP_SIZE();
        DESERIALIZE_FIELD_BT(m_req_node);
        DESERIALIZE_FIELD_BT(m_tx_from_account);
        DESERIALIZE_FIELD_BT(m_tx_to_account);
        DESERIALIZE_FIELD_BT(m_id_hash_of_receipts);
        // restore padding
        return CALC_LEN();
    }
public:
    common::xnode_address_t m_req_node;
    std::string m_tx_from_account;
    std::string m_tx_to_account;
    std::map<uint64_t, uint256_t> m_id_hash_of_receipts;
};

class xreceipt_push_t : public top::basic::xserialize_face_t{
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

NS_END2
