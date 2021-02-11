#pragma once

#include <stdint.h>
#include "xbase/xmem.h"
#include "xbasic/xversion.h"
#include "xbasic/xserializable_based_on.h"

namespace top {
    namespace store {

        struct xproperty_vote : public xserializable_based_on<void> {

            int32_t do_write(base::xstream_t& stream) const override {
                KEEP_SIZE();
                SERIALIZE_FIELD_BT(m_lock_hash);
                SERIALIZE_FIELD_BT(m_amount);
                SERIALIZE_FIELD_BT(m_available);
                SERIALIZE_FIELD_BT(m_expiration);
                return CALC_LEN();
            }

            int32_t do_read(base::xstream_t& stream) override {
                KEEP_SIZE();
                DESERIALIZE_FIELD_BT(m_lock_hash);
                DESERIALIZE_FIELD_BT(m_amount);
                DESERIALIZE_FIELD_BT(m_available);
                DESERIALIZE_FIELD_BT(m_expiration);
                return CALC_LEN();
            }

            std::string m_lock_hash;
            uint64_t m_amount;
            uint64_t m_available;
            uint64_t m_expiration;
        };

        struct xproperty_vote_out : public xserializable_based_on<void> {

            int32_t do_write(base::xstream_t& stream) const override {
                KEEP_SIZE();
                SERIALIZE_FIELD_BT(m_address);
                SERIALIZE_FIELD_BT(m_lock_hash);
                SERIALIZE_FIELD_BT(m_amount);
                SERIALIZE_FIELD_BT(m_expiration);
                return CALC_LEN();
            }

            int32_t do_read(base::xstream_t& stream) override {
                KEEP_SIZE();
                DESERIALIZE_FIELD_BT(m_address);
                DESERIALIZE_FIELD_BT(m_lock_hash);
                DESERIALIZE_FIELD_BT(m_amount);
                DESERIALIZE_FIELD_BT(m_expiration);
                return CALC_LEN();
            }

            std::string m_address;
            std::string m_lock_hash;
            uint64_t m_amount;
            uint64_t m_expiration;
        };
    }
}
