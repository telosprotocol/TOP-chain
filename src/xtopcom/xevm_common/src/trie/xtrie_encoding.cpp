// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/trie/xtrie_encoding.h"

NS_BEG3(top, evm_common, trie)

xbytes_t hexToCompact(xbytes_t hex) {
    auto terminator = xbyte_t{0};
    if (hasTerm(hex)) {
        terminator = 1;
        hex = {hex.begin(), hex.end() - 1};
    }

    xbytes_t buf(hex.size() / 2 + 1);
    buf[0] = terminator << 5;  // the flag byte
    if ((hex.size() & 1) == 1) {
        buf[0] |= 1 << 4;  // old flag
        buf[0] |= hex[0];  // first nibble is contained in the first byte
        hex = {hex.begin() + 1, hex.end()};
    }

    decodeNibbles(hex, buf.begin() + 1);
    return buf;
}

std::size_t hexToCompactInPlace(xbytes_t & hex) {
    auto firstByte = xbyte_t{0};
    auto hexLen = hex.size();
    if (hasTerm(hex)) {
        firstByte = 1 << 5;
        hexLen--;  // last part was the terminator, ignore that
    }
    std::size_t binLen = hexLen / 2 + 1, ni = 0, bi = 1;
    if ((hexLen & 1) == 1) {
        firstByte |= 1 << 4;  // odd flag
        firstByte |= hex[0];  // first nibble is contained in the first byte
        ni++;
    }
    for (; ni < hexLen; bi = bi + 1, ni = ni + 2) {
        hex[bi] = hex[ni] << 4 | hex[ni + 1];
    }
    hex[0] = firstByte;
    return binLen;
}

xbytes_t compactToHex(xbytes_t compact) {
    if (compact.size() == 0) {
        return compact;
    }
    auto base = keybytesToHex(compact);

    // delete terminator flag
    if (base[0] < 2) {
        base = {base.begin(), base.end() - 1};
    }

    auto chop = 2 - (base[0] & 1);
    return {base.begin() + chop, base.end()};
}

xbytes_t keybytesToHex(xbytes_t const & str) {
    std::size_t l = str.size() * 2 + 1;
    auto nibbles = xbytes_t(l);
    for (std::size_t index = 0; index < str.size(); ++index) {
        nibbles[2 * index] = str[index] / 16;
        nibbles[2 * index + 1] = str[index] % 16;
    }
    nibbles[l - 1] = 16;
    return nibbles;
}

// hexToKeybytes turns hex nibbles into key bytes.
// This can only be used for keys of even length.
xbytes_t hexToKeybytes(xbytes_t hex) {
    if (hasTerm(hex)) {
        hex = {hex.begin(), hex.end() - 1};
    }
    if ((hex.size() & 1) != 0) {
        xassert(false);
    }
    auto key = xbytes_t(hex.size() / 2);
    decodeNibbles(hex, key.begin());
    return key;
}

void decodeNibbles(xbytes_t const & nibbles, xbytes_t::iterator bytes_begin) {
    for (std::size_t bi = 0, ni = 0; ni < nibbles.size(); bi += 1, ni += 2) {
        *(bytes_begin + bi) = nibbles[ni] << 4 | nibbles[ni + 1];
    }
}

// prefixLen returns the length of the common prefix of a and b.
std::size_t prefixLen(xbytes_t const & a, xbytes_t const & b) {
    std::size_t i = 0, length = a.size();
    if (b.size() < length) {
        length = b.size();
    }
    for (; i < length; i++) {
        if (a[i] != b[i]) {
            break;
        }
    }
    return i;
}

// hasTerm returns whether a hex key has the terminator flag.
bool hasTerm(xbytes_t const & s) {
    return (!s.empty()) && (s[s.size() - 1] == 16);
}

xbytes_t compact_to_hex(gsl::span<xbyte_t const> const compact) {
    if (compact.empty()) {
        return {};
    }

    auto base = key_bytes_to_hex(compact);

    // delete terminator flag
    if (base[0] < 2) {
        base = {base.begin(), base.end() - 1};
    }

    auto const chop = 2 - (base[0] & 1);
    return {base.begin() + chop, base.end()};
}

xbytes_t key_bytes_to_hex(gsl::span<xbyte_t const> const str) {
    std::size_t const l = str.size() * 2 + 1;
    auto nibbles = xbytes_t(l);
    for (std::size_t index = 0; index < str.size(); ++index) {
        nibbles[2 * index] = str[index] / 16;
        nibbles[2 * index + 1] = str[index] % 16;
    }
    nibbles[l - 1] = 16;
    return nibbles;
}

NS_END3
