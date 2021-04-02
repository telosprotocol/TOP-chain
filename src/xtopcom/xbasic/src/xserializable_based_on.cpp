// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xcontext.h"
#include "xbasic/xserializable_based_on.h"
#include "xbasic/xerror/xerror.h"

#include <cassert>

NS_BEG1(top)

std::int32_t xtop_serializable_based_on<void>::serialize_to(base::xstream_t & stream) const {
    base::xstream_t internal_stream{ base::xcontext_t::instance() };
    do_write(internal_stream);
    return stream << internal_stream;
}

std::int32_t xtop_serializable_based_on<void>::serialize_to(base::xstream_t & stream, std::error_code & ec) const {
    assert(!ec);
    try {
        return serialize_to(stream);
    } catch (...) {
        ec = top::error::xbasic_errc_t::serialization_error;
    }

    return -1;
}

std::int32_t xtop_serializable_based_on<void>::serialize_from(base::xstream_t & stream) {
    base::xstream_t internal_stream{ base::xcontext_t::instance() };
    auto ret = stream >> internal_stream;
    do_read(internal_stream);
    return ret;
}

std::int32_t xtop_serializable_based_on<void>::serialize_from(base::xstream_t & stream, std::error_code & ec) {
    assert(!ec);
    try {
        return serialize_from(stream);
    } catch (...) {
        ec = top::error::xbasic_errc_t::deserialization_error;
    }

    return -1;
}

std::int32_t xtop_serializable_based_on<void>::serialize_to(base::xbuffer_t & stream) const {
    base::xstream_t internal_stream{base::xcontext_t::instance()};
    do_write(internal_stream);
    return stream << internal_stream;
}

std::int32_t xtop_serializable_based_on<void>::serialize_to(base::xbuffer_t & stream, std::error_code & ec) const {
    assert(!ec);
    try {
        return serialize_to(stream);
    } catch (...) {
        ec = top::error::xbasic_errc_t::serialization_error;
    }
    return -1;
}

std::int32_t xtop_serializable_based_on<void>::serialize_from(base::xbuffer_t & stream) {
    base::xstream_t internal_stream{base::xcontext_t::instance()};
    auto ret = stream >> internal_stream;
    do_read(internal_stream);
    return ret;
}

std::int32_t xtop_serializable_based_on<void>::serialize_from(base::xbuffer_t & stream, std::error_code & ec) {
    assert(!ec);
    try {
        return serialize_from(stream);
    } catch (...) {
        ec = top::error::xbasic_errc_t::deserialization_error;
    }
    return -1;
}

std::int32_t xtop_serializable_based_on<void>::do_write(base::xstream_t & stream, std::error_code & ec) const {
    assert(!ec);
    try {
        return do_write(stream);
    } catch (...) {
        ec = top::error::xbasic_errc_t::serialization_error;
    }

    return -1;
}

std::int32_t xtop_serializable_based_on<void>::do_read(base::xstream_t & stream, std::error_code & ec) {
    assert(!ec);
    try {
        return do_read(stream);
    } catch (...) {
        ec = top::error::xbasic_errc_t::deserialization_error;
    }
    return -1;
}

NS_END1
