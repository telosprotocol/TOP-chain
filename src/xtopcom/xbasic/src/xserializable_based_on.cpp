// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xserializable_based_on.h"

#include "xbase/xcontext.h"
#include "xbase/xlog.h"
#include "xbasic/xerror/xchain_error.h"
#include "xbasic/xerror/xerror.h"
#include "xbasic/xerror/xthrow_error.h"

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

template <>
xbyte_buffer_t xtop_serializable_based_on<void>::serialize_based_on<base::xstream_t>() const {
    base::xstream_t stream{ base::xcontext_t::instance() };
    if (do_write(stream) <= 0) {
        std::error_code ec = top::error::xbasic_errc_t::serialization_error;
        top::error::throw_error(ec);
    }

    return { stream.data(), stream.data() + static_cast<size_t>(stream.size()) };
}

template <>
xbyte_buffer_t xtop_serializable_based_on<void>::serialize_based_on<base::xstream_t>(std::error_code & ec) const {
    try {
        return serialize_based_on<base::xstream_t>();
    } catch (top::error::xtop_error_t const & eh) {
        ec = eh.code();
#if defined(XENABLE_TESTS)
        xwarn("xserializable_based_on serialize error category %s; msg %s", ec.category().name(), eh.what());
#else
        xerror("xserializable_based_on serialize error category %s; msg %s", ec.category().name(), eh.what());
#endif
    } catch (std::exception const & eh) {
        ec = top::error::xbasic_errc_t::serialization_error;
#if defined(XENABLE_TESTS)
        xwarn("xserializable_based_on serialize error %s", eh.what());
#else
        xerror("xserializable_based_on serialize error %s", eh.what());
#endif
    } catch (enum_xerror_code const error_code) {
        ec = top::error::xbasic_errc_t::serialization_error;
#if defined(XENABLE_TESTS)
        xwarn("xserializable_based_on serialize error code %d", static_cast<int>(error_code));
#else
        xerror("xserializable_based_on serialize error code %d", static_cast<int>(error_code));
#endif
    } catch (...) {
        ec = top::error::xbasic_errc_t::serialization_error;
#if defined(XENABLE_TESTS)
        xwarn("%s", "xserializable_based_on serialize unknown error");
#else
        xerror("%s", "xserializable_based_on serialize unknown error");
#endif
    }

    return {};
}

template <>
void xtop_serializable_based_on<void>::deserialize_based_on<base::xstream_t>(xbyte_buffer_t bytes) {
    base::xstream_t stream{ base::xcontext_t::instance(), bytes.data(), static_cast<uint32_t>(bytes.size()) };
    if (do_read(stream) <= 0) {
        std::error_code ec{ top::error::xbasic_errc_t::deserialization_error };
        top::error::throw_error(ec);
    }
}

template <>
void xtop_serializable_based_on<void>::deserialize_based_on<base::xstream_t>(xbyte_buffer_t bytes, std::error_code & ec) {
    try {
        deserialize_based_on<base::xstream_t>(std::move(bytes));
    } catch (top::error::xtop_error_t const & eh) {
        ec = eh.code();
#if defined(XENABLE_TESTS)
        xwarn("xserializable_based_on deserialize error category %s; msg %s", ec.category().name(), eh.what());
#else
        xerror("xserializable_based_on deserialize error category %s; msg %s", ec.category().name(), eh.what());
#endif
    } catch (std::exception const & eh) {
        ec = top::error::xbasic_errc_t::deserialization_error;
#if defined(XENABLE_TESTS)
        xwarn("xserializable_based_on deserialize error %s", eh.what());
#else
        xerror("xserializable_based_on deserialize error %s", eh.what());
#endif
    } catch (enum_xerror_code const error_code) {
        ec = top::error::xbasic_errc_t::deserialization_error;
#if defined(XENABLE_TESTS)
        xwarn("xserializable_based_on deserialize error code %d", static_cast<int>(error_code));
#else
        xerror("xserializable_based_on deserialize error code %d", static_cast<int>(error_code));
#endif
    } catch (...) {
        ec = top::error::xbasic_errc_t::deserialization_error;
#if defined(XENABLE_TESTS)
        xwarn("%s", "xserializable_based_on deserialize unknown error");
#else
        xerror("%s", "xserializable_based_on deserialize unknown error");
#endif
    }
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
