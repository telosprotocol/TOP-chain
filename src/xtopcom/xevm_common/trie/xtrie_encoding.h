// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"

NS_BEG3(top, evm_common, trie)

xbytes_t hexToCompact(xbytes_t hex);
xbytes_t compactToHex(xbytes_t compact);
xbytes_t keybytesToHex(xbytes_t const & str);
xbytes_t hexToKeybytes(xbytes_t hex);
void decodeNibbles(xbytes_t const & nibbles, xbytes_t::iterator bytes_begin);
bool hasTerm(xbytes_t const & s);

NS_END3