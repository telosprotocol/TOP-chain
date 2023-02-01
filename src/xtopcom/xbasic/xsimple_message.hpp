// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xhashable.hpp"
#include "xstatistic/xclass_type_converter.h"
#include "xstatistic/xstatistic.h"
#include "xutility/xhash.h"

#include <type_traits>
#include <utility>

NS_BEG1(top)

/**
 * @brief A message class contains message payload and message type.  The message payload is transparent to itself.
 * This class is not thread-safe.
 * @tparam MessageT The message type.
 */
template <typename MessageT>
class xtop_simple_message final : public xhashable_t<xtop_simple_message<MessageT>>, public xstatistic::xstatistic_obj_face_t {
    XSTATIC_ASSERT(std::is_enum<MessageT>::value);

public:
    using message_type = MessageT;
    using hash_result_type = typename xhashable_t<xtop_simple_message<MessageT>>::hash_result_type;

private:
    xbyte_buffer_t m_payload{};
    message_type m_id_or_type{message_type::invalid};

public:
    xtop_simple_message() : xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_undetermined) {}
    xtop_simple_message(xtop_simple_message const &) = default;
    xtop_simple_message & operator=(xtop_simple_message const & obj) {
        m_payload = obj.m_payload;
        m_id_or_type = obj.m_id_or_type;
        modify_class_type(xstatistic::message_id_to_class_type((uint32_t)m_id_or_type));
        return *this;
    }
    xtop_simple_message(xtop_simple_message && obj) : xstatistic::xstatistic_obj_face_t(xstatistic::message_id_to_class_type((uint32_t)obj.id())) {
        // xdbg("xtop_simple_message move construct this id:%u, obj id:%u", (uint32_t)m_id_or_type, (uint32_t)obj.id());
        m_payload = obj.m_payload;
        m_id_or_type = obj.m_id_or_type;
    }
    // xtop_simple_message & operator=(xtop_simple_message &&) = default;

    xtop_simple_message(xbyte_buffer_t msg_payload, message_type const id_or_type)
      : xstatistic::xstatistic_obj_face_t(xstatistic::message_id_to_class_type((uint32_t)id_or_type)), m_payload{std::move(msg_payload)}, m_id_or_type{id_or_type} {
    }

    ~xtop_simple_message() {statistic_del();}

    bool operator==(xtop_simple_message const & other) const noexcept {
        return m_payload == other.m_payload && m_id_or_type == other.m_id_or_type;
    }

    bool operator!=(xtop_simple_message const & other) const noexcept {
        return !(*this == other);
    }

    xbyte_buffer_t const & payload() const noexcept {
        return m_payload;
    }

    message_type id() const noexcept {
        return m_id_or_type;
    }

    message_type type() const noexcept {
        return m_id_or_type;
    }

    bool empty() const noexcept {
        return m_id_or_type == message_type::invalid;
    }

    hash_result_type hash() const override {
        utl::xxh64_t h64;
        h64.update(m_payload.data(), m_payload.size());
        auto id_or_type_value = static_cast<typename std::underlying_type<message_type>::type>(m_id_or_type);
        h64.update(&id_or_type_value, sizeof(typename std::underlying_type<message_type>::type));
        return h64.get_hash();
    }

    virtual int32_t get_class_type() const override {return xstatistic::message_id_to_class_type((uint32_t)m_id_or_type);}
private:
    virtual int32_t get_object_size_real() const override {
        return sizeof(*this);
    }
};

template <typename MessageT>
using xsimple_message_t = xtop_simple_message<MessageT>;

NS_END1
