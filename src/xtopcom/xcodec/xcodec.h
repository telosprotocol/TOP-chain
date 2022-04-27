// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcodec/xcodec_type.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xcodec/xbytes_codec.h"

#include <msgpack.hpp>

#include <iterator>
#include <system_error>
#include <type_traits>

NS_BEG2(top, codec)

template <typename T, xcodec_type_t CodecTypeV>
struct xtop_codec;

template <typename T, xcodec_type_t CodecTypeV>
using xcodec_t = xtop_codec<T, CodecTypeV>;

template <typename T>
struct xtop_codec<T, xcodec_type_t::msgpack> {
    xtop_codec() = delete;
    xtop_codec(xtop_codec const &) = delete;
    xtop_codec & operator=(xtop_codec const &) = delete;
    xtop_codec(xtop_codec &&) = delete;
    xtop_codec & operator=(xtop_codec &&) = delete;
    ~xtop_codec() = delete;

    static_assert(std::is_default_constructible<T>::value, "object must be default constructible");

    template <typename... ArgsT>
    static xbytes_t encode(T const & input, std::error_code & ec, ArgsT &&... args) {
        assert(!ec);
        try {
            auto bytes = xmsgpack_codec_t<T>::encode(input, std::forward<ArgsT>(args)...);
            xbytes_t r;
            r.reserve(bytes.size() + 1);
            r.push_back(to_byte(xcodec_type_t::msgpack));

            std::copy(std::begin(bytes), std::end(bytes), std::back_inserter(r));
            return r;
        } catch (top::error::xtop_error_t const & eh) {
            ec = eh.code();
            return {};
        }
    }

    static T decode(xbytes_t const & in, std::error_code & ec) {
        try {
            if (in.empty()) {
                return T{};
            }

            if (from_byte<xcodec_type_t>(in.front()) != xcodec_type_t::msgpack) {
                top::error::throw_error(top::codec::xcodec_errc_t::decode_wrong_codec_type);
            }

            auto begin = std::next(std::begin(in), 1);

            xbytes_t effective;
            std::copy(begin, std::end(in), std::back_inserter(effective));
            return xmsgpack_codec_t<T>::decode(effective, ec);
        } catch (top::error::xtop_error_t const & eh) {
            ec = eh.code();
            return T{};
        }
    }
};

template <typename T>
struct xtop_codec<T, xcodec_type_t::bytes> {
    xtop_codec() = delete;
    xtop_codec(xtop_codec const &) = delete;
    xtop_codec & operator=(xtop_codec const &) = delete;
    xtop_codec(xtop_codec &&) = delete;
    xtop_codec & operator=(xtop_codec &&) = delete;
    ~xtop_codec() = delete;

    template <typename... ArgsT>
    static xbytes_t encode(T const & input, std::error_code & ec, ArgsT &&... args) {
        try {
            auto bytes = top::codec::xbytes_codec_t<T>::encode(input, ec, std::forward<ArgsT>(args)...);
            xbytes_t r;
            r.reserve(bytes.size() + 1);
            r.push_back(to_byte(xcodec_type_t::bytes));

            std::copy(std::begin(bytes), std::end(bytes), std::back_inserter(r));
            return r;
        } catch (top::error::xtop_error_t const & eh) {
            ec = eh.code();
            return {};
        }
    }

    template <typename... ArgsT>
    static T decode(xbytes_t const & in, std::error_code & ec, ArgsT &&... args) {
        try {
            if (in.empty()) {
                top::error::throw_error(top::codec::xcodec_errc_t::decode_missing_codec_type);
            }

            if (from_byte<xcodec_type_t>(in.front()) != xcodec_type_t::msgpack) {
                top::error::throw_error(top::codec::xcodec_errc_t::decode_wrong_codec_type);
            }

            auto begin = std::next(std::begin(in), 1);

            xbytes_t effective;
            std::copy(begin, std::end(in), std::back_inserter(effective));
            return xbytes_codec_t<T>::decode(effective, ec, std::forward<ArgsT>(args)...);
        } catch (top::error::xtop_error_t const & eh) {
            ec = eh.code();
            return T{};
        }
    }
};


NS_END2
