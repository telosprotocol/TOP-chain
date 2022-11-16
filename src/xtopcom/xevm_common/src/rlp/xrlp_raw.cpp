// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/rlp/xrlp_raw.h"

#include "xevm_common/xerror/xerror.h"

#include <utility>

NS_BEG3(top, evm_common, rlp)

uint64_t ListSize(uint64_t contentSize) {
    return static_cast<uint64_t>(headsize(contentSize)) + contentSize;
}

std::size_t IntSize(uint64_t x) {
    return x < 0x80 ? 1 : 1 + intsize(x);
}

std::tuple<xrlp_elem_kind, xbytes_t, xbytes_t> Split(xbytes_t const & b, std::error_code & ec) {
    xrlp_elem_kind k;
    uint64_t ts, cs;
    std::tie(k, ts, cs) = readKind(b, ec);
    xdbg("Split: %d %lu %lu %d", static_cast<int>(k), ts, cs, ec.value());
    if (ec) {
        return std::make_tuple(k, xbytes_t{}, b);
    }
    return std::make_tuple(k, xbytes_t{b.begin() + ts, b.begin() + ts + cs}, xbytes_t{b.begin() + ts + cs, b.end()});
}

std::tuple<xrlp_elem_kind, gsl::span<xbyte_t const>, gsl::span<xbyte_t const>> split(gsl::span<xbyte_t const> const b, std::error_code & ec) {
    // xrlp_elem_kind k;
    // uint64_t ts, cs;
    // std::tie(k, ts, cs) = read_kind(b, ec);
    auto const result = read_kind(b, ec);
    auto const k = std::get<0>(result);
    auto const ts = std::get<1>(result);
    auto const cs = std::get<2>(result);

    xdbg("Split: %d %lu %lu %d", static_cast<int>(k), ts, cs, ec.value());
    if (ec) {
        return std::make_tuple(k, gsl::span<xbyte_t const>{}, b);
    }

    return std::make_tuple(k, b.subspan(ts, cs), b.subspan(ts + cs));
}

std::pair<xbytes_t, xbytes_t> SplitString(xbytes_t const & b, std::error_code & ec) {
    xrlp_elem_kind k;
    xbytes_t content, rest;
    std::tie(k, content, rest) = Split(b, ec);
    if (ec) {
        return std::make_pair(xbytes_t{}, b);
    }
    if (k == xrlp_elem_kind::List) {
        ec = error::xerrc_t::rlp_expected_string;
        return std::make_pair(xbytes_t{}, b);
    }
    return std::make_pair(content, rest);
}

std::pair<gsl::span<xbyte_t const>, gsl::span<xbyte_t const>> split_string(gsl::span<xbyte_t const> const b, std::error_code & ec) {
    // xrlp_elem_kind k;
    // gsl::span<xbyte_t const> content, rest;
    // std::tie(k, content, rest) = split(b, ec);
    auto const result = split(b, ec);
    if (ec) {
        return {gsl::span<xbyte_t const>{}, b};
    }
    if (std::get<0>(result) == xrlp_elem_kind::List) {
        ec = error::xerrc_t::rlp_expected_string;
        return {gsl::span<xbyte_t const>{}, b};
    }
    return {std::get<1>(result), std::get<2>(result)};
}

std::pair<uint64_t, xbytes_t> SplitUint64(xbytes_t const & b, std::error_code & ec) {
    xbytes_t content, rest;
    std::tie(content, rest) = SplitString(b, ec);
    if (ec) {
        return std::make_pair(0, b);
    }
    if (content.size() == 0) {
        return std::make_pair(0, rest);
    } else if (content.size() == 1) {
        if (content[0] == 0) {
            ec = error::xerrc_t::rlp_canonint;
            return std::make_pair(0, b);
        }
        return std::make_pair(uint64_t(content[0]), rest);
    } else if (content.size() > 8) {
        ec = error::xerrc_t::rlp_uint_overflow;
        return std::make_pair(0, b);
    } else {
        auto x = readSize(content, static_cast<xbyte_t>(content.size()), ec);
        if (ec) {
            ec = error::xerrc_t::rlp_canonint;
            return std::make_pair(0, b);
        }
        return std::make_pair(x, rest);
    }
    __builtin_unreachable();
}

std::pair<xbytes_t, xbytes_t> SplitList(xbytes_t const & b, std::error_code & ec) {
    xrlp_elem_kind k;
    xbytes_t content, rest;
    std::tie(k, content, rest) = Split(b, ec);
    if (ec) {
        return std::make_pair(xbytes_t{}, b);
    }
    if (k != xrlp_elem_kind::List) {
        ec = error::xerrc_t::rlp_expected_list;
        return std::make_pair(xbytes_t{}, b);
    }
    return std::make_pair(content, rest);
}

std::pair<gsl::span<xbyte_t const>, gsl::span<xbyte_t const>> split_list(gsl::span<xbyte_t const> const b, std::error_code & ec) {
    // xrlp_elem_kind k;
    // gsl::span<xbyte_t const> content, rest;
    // std::tie(k, content, rest) = split(b, ec);
    auto const compound_result = split(b, ec);
    if (ec) {
        return {gsl::span<xbyte_t const>{}, b};
    }
    if (std::get<0>(compound_result) != xrlp_elem_kind::List) {
        ec = error::xerrc_t::rlp_expected_list;
        return {gsl::span<xbyte_t const>{}, b};
    }
    return {std::get<1>(compound_result), std::get<2>(compound_result)};
}

std::size_t CountValue(xbytes_t const & input, std::error_code & ec) {
    std::size_t i = 0;
    auto b = input;
    for (; b.size() > 0; i++) {
        xrlp_elem_kind _;
        uint64_t tagsize, size;
        std::tie(_, tagsize, size) = readKind(b, ec);
        if (ec) {
            return 0;
        }
        b = xbytes_t{b.begin() + tagsize + size, b.end()};
    }
    return i;
}

std::size_t count_value(gsl::span<xbyte_t const> b, std::error_code & ec) {
    std::size_t i = 0;
    for (; !b.empty(); i++) {
        // xrlp_elem_kind _;
        // uint64_t tagsize, size;
        // std::tie(_, tagsize, size) = read_kind(b, ec);
        auto const result = read_kind(b, ec);
        if (ec) {
            return 0;
        }

        b = b.subspan(std::get<1>(result) + std::get<2>(result));
    }
    return i;
}

std::tuple<xrlp_elem_kind, uint64_t, uint64_t> readKind(xbytes_t const & buf, std::error_code & ec) {
    if (buf.size() == 0) {
        ec = error::xerrc_t::not_enough_data;
        return std::make_tuple(xrlp_elem_kind{}, 0, 0);
    }
    xrlp_elem_kind k;
    uint64_t tagsize, contentsize;
    auto b = buf[0];
    if (b < 0x80) {
        k = xrlp_elem_kind::Byte;
        tagsize = 0;
        contentsize = 1;
    } else if (b < 0xb8) {
        k = xrlp_elem_kind::String;
        tagsize = 1;
        contentsize = static_cast<uint64_t>(b - 0x80);
        if (contentsize == 1 && buf.size() > 1 && buf[1] < 128) {
            ec = error::xerrc_t::rlp_canonsize;
            return std::make_tuple(xrlp_elem_kind{}, 0, 0);
        }
    } else if (b < 0xc0) {
        k = xrlp_elem_kind::String;
        tagsize = static_cast<uint64_t>(b - 0xb7) + 1;
        contentsize = readSize(xbytes_t{buf.begin() + 1, buf.end()}, b - 0xb7, ec);
    } else if (b < 0xf8) {
        k = xrlp_elem_kind::List;
        tagsize = 1;
        contentsize = static_cast<uint64_t>(b - 0xc0);
    } else {
        k = xrlp_elem_kind::List;
        tagsize = static_cast<uint64_t>(b - 0xf7) + 1;
        contentsize = readSize(xbytes_t{buf.begin() + 1, buf.end()}, b - 0xf7, ec);
    }
    if (ec) {
        return std::make_tuple(xrlp_elem_kind{}, 0, 0);
    }

    if (contentsize > uint64_t(buf.size()) - tagsize) {
        ec = error::xerrc_t::rlp_value_too_large;
        return std::make_tuple(xrlp_elem_kind{}, 0, 0);
    }
    return std::make_tuple(k, tagsize, contentsize);

    if (ec) {
        return std::make_tuple(xrlp_elem_kind{}, 0, 0);
    }
}

std::tuple<xrlp_elem_kind, uint64_t, uint64_t> read_kind(gsl::span<xbyte_t const> const buf, std::error_code & ec) {
    if (buf.empty()) {
        ec = error::xerrc_t::not_enough_data;
        return std::make_tuple(xrlp_elem_kind{}, 0, 0);
    }
    xrlp_elem_kind k;
    uint64_t tagsize, contentsize;
    auto const b = buf[0];
    if (b < 0x80) {
        k = xrlp_elem_kind::Byte;
        tagsize = 0;
        contentsize = 1;
    } else if (b < 0xb8) {
        k = xrlp_elem_kind::String;
        tagsize = 1;
        contentsize = static_cast<uint64_t>(b - 0x80);
        if (contentsize == 1 && buf.size() > 1 && buf[1] < 128) {
            ec = error::xerrc_t::rlp_canonsize;
            return std::make_tuple(xrlp_elem_kind{}, 0, 0);
        }
    } else if (b < 0xc0) {
        k = xrlp_elem_kind::String;
        tagsize = static_cast<uint64_t>(b - 0xb7) + 1;
        contentsize = read_size(buf.subspan(1), b - 0xb7, ec);
    } else if (b < 0xf8) {
        k = xrlp_elem_kind::List;
        tagsize = 1;
        contentsize = static_cast<uint64_t>(b - 0xc0);
    } else {
        k = xrlp_elem_kind::List;
        tagsize = static_cast<uint64_t>(b - 0xf7) + 1;
        contentsize = read_size(buf.subspan(1), b - 0xf7, ec);
    }
    if (ec) {
        return std::make_tuple(xrlp_elem_kind{}, 0, 0);
    }

    if (contentsize > static_cast<uint64_t>(buf.size()) - tagsize) {
        ec = error::xerrc_t::rlp_value_too_large;
        return std::make_tuple(xrlp_elem_kind{}, 0, 0);
    }
    return std::make_tuple(k, tagsize, contentsize);
}

uint64_t readSize(xbytes_t const & b, uint8_t slen, std::error_code & ec) {
    if (static_cast<std::size_t>(slen) > b.size()) {
        ec = error::xerrc_t::not_enough_data;
        return 0;
    }
    uint64_t res_size{0};
    switch (slen) {
    case 1: {
        res_size = (uint64_t)b[0];
        break;
    }
    case 2: {
        res_size = (uint64_t)b[0] << 8 | (uint64_t)b[1];
        break;
    }
    case 3: {
        res_size = (uint64_t)b[0] << 16 | (uint64_t)b[1] << 8 | (uint64_t)b[2];
        break;
    }
    case 4: {
        res_size = (uint64_t)b[0] << 24 | (uint64_t)b[1] << 16 | (uint64_t)b[2] << 8 | (uint64_t)b[3];
        break;
    }
    case 5: {
        res_size = (uint64_t)b[0] << 32 | (uint64_t)b[1] << 24 | (uint64_t)b[2] << 16 | (uint64_t)b[3] << 8 | (uint64_t)b[4];
        break;
    }
    case 6: {
        res_size = (uint64_t)b[0] << 40 | (uint64_t)b[1] << 32 | (uint64_t)b[2] << 24 | (uint64_t)b[3] << 16 | (uint64_t)b[4] << 8 | (uint64_t)b[5];
        break;
    }
    case 7: {
        res_size = (uint64_t)b[0] << 48 | (uint64_t)b[1] << 40 | (uint64_t)b[2] << 32 | (uint64_t)b[3] << 24 | (uint64_t)b[4] << 16 | (uint64_t)b[5] << 8 | (uint64_t)b[6];
        break;
    }
    case 8: {
        res_size = (uint64_t)b[0] << 56 | (uint64_t)b[1] << 48 | (uint64_t)b[2] << 40 | (uint64_t)b[3] << 32 | (uint64_t)b[4] << 24 | (uint64_t)b[5] << 16 | (uint64_t)b[6] << 8 |
                   (uint64_t)b[7];
        break;
    }
    default: {
        ec = error::xerrc_t::rlp_canonsize;
        return 0;
    }
    }
    // Reject sizes < 56 (shouldn't have separate size) and sizes with
    // leading zero bytes.
    if (res_size < 56 || b[0] == 0) {
        ec = error::xerrc_t::rlp_canonsize;
        return 0;
    }
    return res_size;
}

uint64_t read_size(gsl::span<xbyte_t const> const b, uint8_t const slen, std::error_code & ec) {
    if (static_cast<std::size_t>(slen) > b.size()) {
        ec = error::xerrc_t::not_enough_data;
        return 0;
    }
    uint64_t res_size{0};
    switch (slen) {
    case 1: {
        res_size = static_cast<uint64_t>(b[0]);
        break;
    }
    case 2: {
        res_size = static_cast<uint64_t>(b[0]) << 8 | static_cast<uint64_t>(b[1]);
        break;
    }
    case 3: {
        res_size = static_cast<uint64_t>(b[0]) << 16 | static_cast<uint64_t>(b[1]) << 8 | static_cast<uint64_t>(b[2]);
        break;
    }
    case 4: {
        res_size = static_cast<uint64_t>(b[0]) << 24 | static_cast<uint64_t>(b[1]) << 16 | static_cast<uint64_t>(b[2]) << 8 | static_cast<uint64_t>(b[3]);
        break;
    }
    case 5: {
        res_size = static_cast<uint64_t>(b[0]) << 32 | static_cast<uint64_t>(b[1]) << 24 | static_cast<uint64_t>(b[2]) << 16 | static_cast<uint64_t>(b[3]) << 8 |
                   static_cast<uint64_t>(b[4]);
        break;
    }
    case 6: {
        res_size = static_cast<uint64_t>(b[0]) << 40 | static_cast<uint64_t>(b[1]) << 32 | static_cast<uint64_t>(b[2]) << 24 | static_cast<uint64_t>(b[3]) << 16 |
                   static_cast<uint64_t>(b[4]) << 8 | static_cast<uint64_t>(b[5]);
        break;
    }
    case 7: {
        res_size = static_cast<uint64_t>(b[0]) << 48 | static_cast<uint64_t>(b[1]) << 40 | static_cast<uint64_t>(b[2]) << 32 | static_cast<uint64_t>(b[3]) << 24 |
                   static_cast<uint64_t>(b[4]) << 16 | static_cast<uint64_t>(b[5]) << 8 | static_cast<uint64_t>(b[6]);
        break;
    }
    case 8: {
        res_size = static_cast<uint64_t>(b[0]) << 56 | static_cast<uint64_t>(b[1]) << 48 | static_cast<uint64_t>(b[2]) << 40 | static_cast<uint64_t>(b[3]) << 32 |
                   static_cast<uint64_t>(b[4]) << 24 | static_cast<uint64_t>(b[5]) << 16 | static_cast<uint64_t>(b[6]) << 8 | static_cast<uint64_t>(b[7]);
        break;
    }
    default: {
        ec = error::xerrc_t::rlp_canonsize;
        return 0;
    }
    }
    // Reject sizes < 56 (shouldn't have separate size) and sizes with
    // leading zero bytes.
    if (res_size < 56 || b[0] == 0) {
        ec = error::xerrc_t::rlp_canonsize;
        return 0;
    }

    return res_size;
}

std::size_t headsize(uint64_t size) {
    return size < 56 ? 1 : 1 + intsize(size);
}

std::size_t intsize(uint64_t i) {
    std::size_t size = 1;
    for (;; size++) {
        i >>= 8;
        if (i == 0) {
            return size;
        }
    }
}

NS_END3
