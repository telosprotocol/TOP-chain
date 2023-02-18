// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xspan.h"

NS_BEG3(top, evm_common, trie)

// xbytes_t hexToCompact(xbytes_t hex);
// hexToCompactInPlace places the compact key in input buffer, returning the length
// needed for the representation
// std::size_t hexToCompactInPlace(xbytes_t & hex);
// xbytes_t compactToHex(xbytes_t compact);
// xbytes_t keybytesToHex(xbytes_t const & str);
// xbytes_t hexToKeybytes(xbytes_t hex);
// void decodeNibbles(xbytes_t const & nibbles, xbytes_t::iterator bytes_begin);
// std::size_t prefixLen(xbytes_t const & a, xbytes_t const & b);
// bool hasTerm(xbytes_t const & s);

xbytes_t hex_to_compact(xspan_t<xbyte_t const> hex);
std::size_t hex_to_compact_inplace(xspan_t<xbyte_t> hex);
xbytes_t compact_to_hex(xspan_t<xbyte_t const> compact);
xbytes_t key_bytes_to_hex(xspan_t<xbyte_t const> bytes);
xbytes_t hex_to_key_bytes(xspan_t<xbyte_t const> hex);
void decode_nibbles(xspan_t<xbyte_t const> nibbles, xspan_t<xbyte_t> out);
std::size_t prefix_len(xspan_t<xbyte_t const> a, xspan_t<xbyte_t const> b);
bool has_terminator(xspan_t<xbyte_t const> bytes);

NS_END3
