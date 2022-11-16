// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xbase/xns_macro.h"
#include "xbasic/xbyte_buffer.h"

#include <gsl/span>

#include <system_error>
#include <tuple>

NS_BEG3(top, evm_common, rlp)

enum class xrlp_elem_kind : std::size_t {
    Byte = 0,
    String = 1,
    List = 2,
};

// ListSize returns the encoded size of an RLP list with the given
// content size.
uint64_t ListSize(uint64_t contentSize);

// IntSize returns the encoded size of the integer x.
std::size_t IntSize(uint64_t x);

// Split returns the content of first RLP value and any
// bytes after the value as subslices of b.
std::tuple<xrlp_elem_kind, xbytes_t, xbytes_t> Split(xbytes_t const & b, std::error_code & ec);
std::tuple<xrlp_elem_kind, gsl::span<xbyte_t const>, gsl::span<xbyte_t const>> split(gsl::span<xbyte_t const> b, std::error_code & ec);

// SplitString splits b into the content of an RLP string
// and any remaining bytes after the string.
std::pair<xbytes_t, xbytes_t> SplitString(xbytes_t const & b, std::error_code & ec);
std::pair<gsl::span<xbyte_t const>, gsl::span<xbyte_t const>> split_string(gsl::span<xbyte_t const> b, std::error_code & ec);

// SplitUint64 decodes an integer at the beginning of b.
// It also returns the remaining data after the integer in 'rest'.
std::pair<uint64_t, xbytes_t> SplitUint64(xbytes_t const & b, std::error_code & ec);

// SplitList splits b into the content of a list and any remaining
// bytes after the list.
std::pair<xbytes_t, xbytes_t> SplitList(xbytes_t const & b, std::error_code & ec);
std::pair<gsl::span<xbyte_t const>, gsl::span<xbyte_t const>> split_list(gsl::span<xbyte_t const> b, std::error_code & ec);

// CountValues counts the number of encoded values in b.
std::size_t CountValue(xbytes_t const & b, std::error_code & ec);
std::size_t count_value(gsl::span<xbyte_t const> b, std::error_code & ec);

std::tuple<xrlp_elem_kind, uint64_t, uint64_t> readKind(xbytes_t const & buf, std::error_code & ec);
std::tuple<xrlp_elem_kind, uint64_t, uint64_t> read_kind(gsl::span<xbyte_t const> buf, std::error_code & ec);

uint64_t readSize(xbytes_t const & b, uint8_t slen, std::error_code & ec);
uint64_t read_size(gsl::span<xbyte_t const> b, uint8_t slen, std::error_code & ec);

// headsize returns the size of a list or string header
// for a value of the given size.
std::size_t headsize(uint64_t size);

// intsize computes the minimum number of bytes required to store i.
std::size_t intsize(uint64_t i);

NS_END3
