// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/common_data.h"

#include "xcommon/xerror/xerror.h"

#include <random>

// using namespace std;

namespace {
int fromHexChar(char _i) noexcept {
    if (_i >= '0' && _i <= '9')
        return _i - '0';
    if (_i >= 'a' && _i <= 'f')
        return _i - 'a' + 10;
    if (_i >= 'A' && _i <= 'F')
        return _i - 'A' + 10;
    return -1;
}
}  // namespace

namespace top {
namespace evm_common {

bool isHex(std::string const & _s) noexcept {
    auto it = _s.begin();
    if (_s.compare(0, 2, "0x") == 0)
        it += 2;
    return std::all_of(it, _s.end(), [](char c) { return fromHexChar(c) != -1; });
}

std::string escaped(std::string const & _s, bool _all) {
    static const std::map<char, char> prettyEscapes{{'\r', 'r'}, {'\n', 'n'}, {'\t', 't'}, {'\v', 'v'}};
    std::string ret;
    ret.reserve(_s.size() + 2);
    ret.push_back('"');
    for (auto i : _s)
        if (i == '"' && !_all)
            ret += "\\\"";
        else if (i == '\\' && !_all)
            ret += "\\\\";
        else if (prettyEscapes.count(i) && !_all) {
            ret += '\\';
            ret += prettyEscapes.find(i)->second;
        } else if (i < ' ' || _all) {
            ret += "\\x";
            ret.push_back("0123456789abcdef"[(uint8_t)i / 16]);
            ret.push_back("0123456789abcdef"[(uint8_t)i % 16]);
        } else
            ret.push_back(i);
    ret.push_back('"');
    return ret;
}

xbytes_t fromHex(std::string const & _s, WhenError _throw) {
    unsigned s = (_s.size() >= 2 && _s[0] == '0' && _s[1] == 'x') ? 2 : 0;
    std::vector<uint8_t> ret;
    ret.reserve((_s.size() - s + 1) / 2);

    if (_s.size() % 2) {
        int h = fromHexChar(_s[s++]);
        if (h != -1)
            ret.push_back(h);
        else if (_throw == WhenError::Throw)
            // todo
            throw("");
        // BOOST_THROW_EXCEPTION(BadHexCharacter());
        else
            return xbytes_t();
    }
    for (unsigned i = s; i < _s.size(); i += 2) {
        int h = fromHexChar(_s[i]);
        int l = fromHexChar(_s[i + 1]);
        if (h != -1 && l != -1)
            ret.push_back((xbyte_t)(h * 16 + l));
        else if (_throw == WhenError::Throw)
            // todo
            throw("");
        // BOOST_THROW_EXCEPTION(BadHexCharacter());
        else
            return xbytes_t();
    }
    return ret;
}

xbytes_t asNibbles(bytesConstRef const & _s) {
    std::vector<uint8_t> ret;
    ret.reserve(_s.size() * 2);
    for (auto i : _s) {
        ret.push_back(i / 16);
        ret.push_back(i % 16);
    }
    return ret;
}
}  // namespace evm_common
}  // namespace top

NS_BEG1(top)

template <>
xbytes_t to_bytes<evm_common::u256>(evm_common::u256 const & value) {
    xbytes_t bytes(32, 0);
    evm_common::u256 val = value;
    for (auto i = bytes.size(); i != 0; val >>= 8, i--) {
        evm_common::u256 v = val & static_cast<evm_common::u256>(0xff);
        bytes[i - 1] = v.convert_to<xbyte_t>();
    }
    return bytes;
}

template <>
evm_common::u256 from_bytes<evm_common::u256>(xbytes_t const & input, std::error_code & ec) {
    assert(input.size() >= 32);

    if (input.size() < 32) {
        ec = common::error::xerrc_t::not_enough_data;
        return 0;
    }

    evm_common::u256 r{0};
    for (size_t i = 0; i < 32; ++i) {
        r = (r << 8) | input[i];
    }

    return r;
}

template <>
std::string to_string<evm_common::u256>(evm_common::u256 const & value) {
    std::string str(32, 0);
    evm_common::u256 val = value;
    for (auto i = str.size(); i != 0; val >>= 8, i--) {
        evm_common::u256 v = val & static_cast<evm_common::u256>(0xff);
        str[i - 1] = v.convert_to<xbyte_t>();
    }
    return str;
}

template <>
evm_common::u256 from_string<evm_common::u256>(std::string const & input, std::error_code & ec) {
    assert(input.size() >= 32);

    if (input.size() < 32) {
        ec = common::error::xerrc_t::not_enough_data;
        return 0;
    }

    evm_common::u256 r{0};
    for (size_t i = 0; i < 32; ++i) {
        r = (r << 8) |= input[i];
    }

    return r;
}

template <>
xbytes_t to_bytes<evm_common::u160>(evm_common::u160 const & value) {
    xbytes_t bytes(20, 0);
    evm_common::u160 val = value;
    for (auto i = bytes.size(); i != 0; val >>= 8, i--) {
        evm_common::u160 v = val & static_cast<evm_common::u160>(0xff);
        bytes[i - 1] = v.convert_to<xbyte_t>();
    }
    return bytes;
}

template <>
evm_common::u160 from_bytes<evm_common::u160>(xbytes_t const & input, std::error_code & ec) {
    assert(input.size() >= 20);

    if (input.size() < 20) {
        ec = common::error::xerrc_t::not_enough_data;
        return 0;
    }

    evm_common::u160 r{0};
    for (size_t i = 0; i < 20; ++i) {
        r = (r << 8) | input[i];
    }

    return r;
}

template <>
std::string to_string<evm_common::u160>(evm_common::u160 const & value) {
    std::string str(20, 0);
    evm_common::u160 val = value;
    for (auto i = str.size(); i != 0; val >>= 8, i--) {
        evm_common::u160 v = val & static_cast<evm_common::u160>(0xff);
        str[i - 1] = v.convert_to<xbyte_t>();
    }
    return str;
}

template <>
evm_common::u160 from_string<evm_common::u160>(std::string const & input, std::error_code & ec) {
    assert(input.size() >= 20);

    if (input.size() < 20) {
        ec = common::error::xerrc_t::not_enough_data;
        return 0;
    }

    evm_common::u160 r{0};
    for (size_t i = 0; i < 20; ++i) {
        r = (r << 8) |= input[i];
    }

    return r;
}

NS_END1
