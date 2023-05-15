// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/rlp.h"

#include "xcommon/data.h"
#include "xcommon/xerror/xerror.h"
#include "xcommon/xeth_address.h"

#include <algorithm>
#include <cinttypes>

// using namespace std;


namespace top {
namespace evm_common {

xbytes_t RLPNull = xrlp("");
xbytes_t RLPEmptyList = rlpList();


RLP::RLP(bytesConstRef _d, Strictness _s):
    m_data(_d)
{
    if ((_s & FailIfTooBig) && actualSize() < _d.size())
    {
        if (_s & ThrowOnFail)
           assert(true);
        else
            m_data.reset();
    }
    if ((_s & FailIfTooSmall) && actualSize() > _d.size())
    {
        if (_s & ThrowOnFail)
            assert(true);
        else
            m_data.reset();
    }
}

RLP::iterator& RLP::iterator::operator++()
{
    if (m_remaining)
    {
        m_currentItem.retarget(m_currentItem.next().data(), m_remaining);
        m_currentItem = m_currentItem.cropped(0, sizeAsEncoded(m_currentItem));
        m_remaining -= std::min<size_t>(m_remaining, m_currentItem.size());
    }
    else
        m_currentItem.retarget(m_currentItem.next().data(), 0);
    return *this;
}

RLP::iterator::iterator(RLP const& _parent, bool _begin)
{
    if (_begin && _parent.isList())
    {
        auto pl = _parent.payload();
        m_currentItem = pl.cropped(0, sizeAsEncoded(pl));
        m_remaining = pl.size() - m_currentItem.size();
    }
    else
    {
        m_currentItem = _parent.data().cropped(_parent.data().size());
        m_remaining = 0;
    }
}

RLP RLP::operator[](size_t _i) const
{
    if (_i < m_lastIndex)
    {
        // Get size of 0th item
        m_lastEnd = sizeAsEncoded(payload());
        // Set m_lastItem to 0th item data
        m_lastItem = payload().cropped(0, m_lastEnd);
        m_lastIndex = 0;
    }
    for (; m_lastIndex < _i && m_lastItem.size(); ++m_lastIndex)
    {
        // Get chunk of payload data starting right after m_lastItem
        // This will be empty when we're out of bounds
        m_lastItem = payload().cropped(m_lastEnd);
        // Crop it to get the data of the next item
        m_lastItem = m_lastItem.cropped(0, sizeAsEncoded(m_lastItem));
        // Point m_lastEnd to next item
        m_lastEnd += m_lastItem.size();
    }
    return RLP(m_lastItem, ThrowOnFail | FailIfTooSmall);
}

size_t RLP::actualSize() const
{
    if (isNull())
        return 0;
    if (isSingleByte())
        return 1;
    if (isData() || isList())
        return payloadOffset() + length();
    return 0;
}

void RLP::requireGood() const
{
    if (isNull())
        assert(true);
    xbyte_t n = m_data[0];
    if (n != c_rlpDataImmLenStart + 1)
        return;
    if (m_data.size() < 2)
        assert(true);
    if (m_data[1] < c_rlpDataImmLenStart)
        assert(true);
}

bool RLP::isInt() const
{
    if (isNull())
        return false;
    requireGood();
    xbyte_t n = m_data[0];
    if (n < c_rlpDataImmLenStart)
        return !!n;
    else if (n == c_rlpDataImmLenStart)
        return true;
    else if (n <= c_rlpDataIndLenZero)
    {
        if (m_data.size() <= 1)
            assert(true);
        return m_data[1] != 0;
    }
    else if (n < c_rlpListStart)
    {
        if (m_data.size() <= size_t(1 + n - c_rlpDataIndLenZero))
            assert(true);
        return m_data[1 + n - c_rlpDataIndLenZero] != 0;
    }
    else
        return false;
    return false;
}

size_t RLP::length() const
{
    if (isNull())
        return 0;
    requireGood();
    size_t ret = 0;
    xbyte_t const n = m_data[0];
    if (n < c_rlpDataImmLenStart)
        return 1;
    else if (n <= c_rlpDataIndLenZero)
        return n - c_rlpDataImmLenStart;
    else if (n < c_rlpListStart)
    {
        if (m_data.size() <= size_t(n - c_rlpDataIndLenZero))
            assert(true);
        if (m_data.size() > 1)
            if (m_data[1] == 0)
                assert(true);
        unsigned lengthSize = n - c_rlpDataIndLenZero;
        if (lengthSize > sizeof(ret))
            // We did not check, but would most probably not fit in our memory.
            assert(true);
        // No leading zeroes.
        if (!m_data[1])
            assert(true);
        for (unsigned i = 0; i < lengthSize; ++i)
            ret = (ret << 8) | m_data[i + 1];
        // Must be greater than the limit.
        if (ret < c_rlpListStart - c_rlpDataImmLenStart - c_rlpMaxLengthBytes)
            assert(true);
    }
    else if (n <= c_rlpListIndLenZero)
        return n - c_rlpListStart;
    else
    {
        unsigned lengthSize = n - c_rlpListIndLenZero;
        if (m_data.size() <= lengthSize)
            assert(true);
        if (m_data.size() > 1)
            if (m_data[1] == 0)
                assert(true);
        if (lengthSize > sizeof(ret))
            // We did not check, but would most probably not fit in our memory.
            assert(true);
        if (!m_data[1])
            assert(true);
        for (unsigned i = 0; i < lengthSize; ++i)
            ret = (ret << 8) | m_data[i + 1];
        if (ret < 0x100 - c_rlpListStart - c_rlpMaxLengthBytes)
            assert(true);
    }
    // We have to be able to add payloadOffset to length without overflow.
    // This rejects roughly 4GB-sized RLPs on some platforms.
    if (ret >= std::numeric_limits<size_t>::max() - 0x100)
        assert(true);
    return ret;
}

size_t RLP::items() const
{
    if (isList())
    {
        bytesConstRef d = payload();
        size_t i = 0;
        for (; d.size(); ++i)
            d = d.cropped(sizeAsEncoded(d));
        return i;
    }
    return 0;
}


xbytes_t RLP::encode(u256 const & value) noexcept {
    using boost::multiprecision::cpp_int;

    xbytes_t bytes;
    export_bits(value, std::back_inserter(bytes), 8);

    if (bytes.empty() || ((bytes.size() == 1) && (bytes[0] == 0))) {
        return {0x80};
    }

    return encode(bytes);
}

xbytes_t RLP::encodeList(xbytes_t const & encoded) noexcept {
    auto result = encodeHeader(encoded.size(), 0xc0, 0xf7);
    result.reserve(result.size() + encoded.size());
    result.insert(result.end(), encoded.begin(), encoded.end());
    return result;
}

//xbytes_t RLP::encode(const xbytes_t & data) {
//    if (data.size() == 1 && data[0] <= 0x7f) {
//        // Fits in single byte, no header
//        return data;
//    }
//
//    auto encoded = encodeHeader(data.size(), 0x80, 0xb7);
//    encoded.insert(encoded.end(), data.begin(), data.end());
//    return encoded;
//}

xbytes_t RLP::encode(xspan_t<xbyte_t const> const bytes) {
    if (bytes.size() == 1 && bytes[0] <= 0x7f) {
        // Fits in single byte, no header
        return xbytes_t{bytes[0]};
    }

    auto encoded = encodeHeader(bytes.size(), 0x80, 0xb7);
    encoded.insert(encoded.end(), std::begin(bytes), std::end(bytes));
    return encoded;
}

xbytes_t RLP::encodeHeader(uint64_t size, uint8_t smallTag, uint8_t largeTag) noexcept {
    if (size < 56) {
        return {static_cast<uint8_t>(smallTag + size)};
    }

    auto const sizeData = putVarInt(size);

    auto header = xbytes_t();
    header.reserve(1 + sizeData.size());
    header.push_back(largeTag + static_cast<uint8_t>(sizeData.size()));
    header.insert(header.end(), sizeData.begin(), sizeData.end());
    return header;
}

xbytes_t RLP::putVarInt(uint64_t i) {
    xbytes_t bytes;  // accumulate bytes here, in reverse order
    do {
        // take LSB byte, append
        bytes.push_back(i & 0xff);
        i = i >> 8;
    } while (i);
    if (!(bytes.size() >= 1 && bytes.size() <= 8))
    { 
        throw std::invalid_argument("invalid length length");
    }
    std::reverse(bytes.begin(), bytes.end());
    return bytes;
}

uint64_t RLP::parseVarInt(size_t size, xbytes_t const & data, size_t index) {
    if (size < 1 || size > 8) {
        throw std::invalid_argument("invalid length length");
    }
    if (data.size() - index < size) {
        throw std::invalid_argument("Not enough data for varInt");
    }
    if (size >= 2 && data[index] == 0) {
        throw std::invalid_argument("multi-byte length must have no leading zero");
    }
    uint64_t val = 0;
    for (size_t i = 0; i < size; ++i) {
        val = val << 8;
        val += data[index + i];
    }
    return static_cast<size_t>(val);
}

uint64_t RLP::parse_variant_int(size_t const size, xbytes_t const & data, size_t const index, std::error_code & ec) {
    assert(!ec);

    if (size < 1 || size > 8) {
        xwarn("RLP::parse_variant_int failed, invalid length of length");
        ec = common::error::xerrc_t::rlp_invalid_size_of_length_field;
        return 0;
    }

    if (data.size() - index < size) {
        xwarn("RLP::parse_variant_int failed, not enough data for varInt");
        ec = common::error::xerrc_t::rlp_not_enough_data;
        return 0;
    }

    if (size >= 2 && data[index] == 0) {
        xwarn("RLP::parse_variant_int failed, multi-byte length must have no leading zero");
        ec = common::error::xerrc_t::rlp_invalid_encoded_data;
        return 0;
    }

    uint64_t val = 0;
    for (size_t i = 0; i < size; ++i) {
        val = val << 8;
        val += data[index + i];
    }
    return static_cast<size_t>(val);
}

RLP::DecodedItem RLP::decodeList(xbytes_t const & input) {
    RLP::DecodedItem item;
    auto remainder = input;
    while (true) {
        auto listItem = RLP::decode(remainder);
        if (!listItem.decoded.empty()) {
            for (auto const & decoded : listItem.decoded) {
                item.decoded.push_back(decoded);
            }
        } else {
            item.decoded.push_back(xbytes_t());
        }
        if (listItem.remainder.size() == 0) {
            break;
        } else {
            remainder = listItem.remainder;
        }
    }
    return item;
}

RLP::DecodedItem RLP::decode(xbytes_t const & input) {
    if (input.empty()) {
        throw std::invalid_argument("can't decode empty rlp data");
    }
    RLP::DecodedItem item;
    auto const input_len = input.size();
    auto const prefix = input[0];
    if (prefix <= 0x7f) {
        // 00--7f: a single byte whose value is in the [0x00, 0x7f] range, that byte is its own RLP encoding.
        item.decoded.push_back(xbytes_t{input[0]});
        item.remainder = sub_data(input, 1);
        return item;
    }
    if (prefix <= 0xb7) {
        // 80--b7: short string
        // string is 0-55 bytes long. A single byte with value 0x80 plus the length of the string followed by the string
        // The range of the first byte is [0x80, 0xb7]

        // empty string
        if (prefix == 0x80) {
            item.decoded.emplace_back();
            item.remainder = sub_data(input, 1);
            return item;
        }

        size_t const str_len = prefix - 0x80;
        if (str_len == 1 && input[1] <= 0x7f) {
            throw std::invalid_argument("single byte below 128 must be encoded as itself");
        }

        if (input_len < (1 + str_len)) {
            throw std::invalid_argument(std::string("invalid short string, length ") + std::to_string(str_len));
        }
        item.decoded.push_back(sub_data(input, 1, str_len));
        item.remainder = sub_data(input, 1 + str_len);

        return item;
    }
    if (prefix <= 0xbf) {
        // b8--bf: long string
        auto lenOfStrLen = size_t(prefix - 0xb7);
        auto strLen = static_cast<size_t>(parseVarInt(lenOfStrLen, input, 1));
        if (input_len < lenOfStrLen || input_len < (1 + lenOfStrLen + strLen)) {
            throw std::invalid_argument(std::string("Invalid rlp encoding length, length ") + std::to_string(strLen));
        }
        auto data = sub_data(input, 1 + lenOfStrLen, strLen);
        item.decoded.push_back(data);
        item.remainder = sub_data(input, 1 + lenOfStrLen + strLen);
        return item;
    }
    if (prefix <= 0xf7) {
        // c0--f7: a list between  0-55 bytes long
        auto listLen = size_t(prefix - 0xc0);
        if (input_len < (1 + listLen)) {
            throw std::invalid_argument(std::string("Invalid rlp string length, length ") + std::to_string(listLen));
        }
        // empty list
        if (listLen == 0) {
            item.remainder = sub_data(input, 1);
            return item;
        }

        // decode list
        auto listItem = decodeList(sub_data(input, 1, listLen));
        for (auto & data : listItem.decoded) {
            item.decoded.push_back(data);
        }
        item.remainder = sub_data(input, 1 + listLen);
        return item;
    }
    // f8--ff
    auto lenOfListLen = size_t(prefix - 0xf7);
    auto listLen = static_cast<size_t>(parseVarInt(lenOfListLen, input, 1));
    if (listLen < 56) {
        throw std::invalid_argument("length below 56 must be encoded in one byte");
    }
    if (input_len < lenOfListLen || input_len < (1 + lenOfListLen + listLen)) {
        throw std::invalid_argument(std::string("Invalid rlp list length, length ") + std::to_string(listLen));
    }

    // decode list
    auto listItem = decodeList(sub_data(input, 1 + lenOfListLen, listLen));
    for (auto & data : listItem.decoded) {
        item.decoded.push_back(data);
    }
    item.remainder = sub_data(input, 1 + lenOfListLen + listLen);
    return item;
}

RLP::DecodedItem RLP::decode_once(xbytes_t const & input) {
    if (input.empty()) {
        throw std::invalid_argument("can't decode empty rlp data");
    }
    RLP::DecodedItem item;
    auto inputLen = input.size();
    auto prefix = input[0];
    if (prefix <= 0x7f) {
        // 00--7f: a single byte whose value is in the [0x00, 0x7f] range, that byte is its own RLP encoding.
        item.decoded.push_back(xbytes_t{input[0]});
        item.remainder = sub_data(input, 1);
        return item;
    }
    if (prefix <= 0xb7) {
        // 80--b7: short string
        // string is 0-55 bytes long. A single byte with value 0x80 plus the length of the string followed by the string
        // The range of the first byte is [0x80, 0xb7]

        // empty string
        if (prefix == 0x80) {
            item.decoded.emplace_back(xbytes_t());
            item.remainder = sub_data(input, 1);
            return item;
        }

        size_t strLen = prefix - 0x80;
        if (strLen == 1 && input[1] <= 0x7f) {
            throw std::invalid_argument("single byte below 128 must be encoded as itself");
        }

        if (inputLen < (1 + strLen)) {
            throw std::invalid_argument(std::string("invalid short string, length ") + std::to_string(strLen));
        }
        item.decoded.push_back(sub_data(input, 1, strLen));
        item.remainder = sub_data(input, 1 + strLen);

        return item;
    }
    if (prefix <= 0xbf) {
        // b8--bf: long string
        auto lenOfStrLen = size_t(prefix - 0xb7);
        auto strLen = static_cast<size_t>(parseVarInt(lenOfStrLen, input, 1));
        if (inputLen < lenOfStrLen || inputLen < (1 + lenOfStrLen + strLen)) {
            throw std::invalid_argument(std::string("Invalid rlp encoding length, length ") + std::to_string(strLen));
        }
        auto data = sub_data(input, 1 + lenOfStrLen, strLen);
        item.decoded.push_back(data);
        item.remainder = sub_data(input, 1 + lenOfStrLen + strLen);
        return item;
    }
    if (prefix <= 0xf7) {
        // c0--f7: a list between  0-55 bytes long
        auto listLen = size_t(prefix - 0xc0);
        if (inputLen < (1 + listLen)) {
            throw std::invalid_argument(std::string("Invalid rlp string length, length ") + std::to_string(listLen));
        }
        // empty list
        if (listLen == 0) {
            item.remainder = sub_data(input, 1);
            return item;
        }
        auto data = sub_data(input, 1, listLen);
        item.decoded.push_back(data);
        item.remainder = sub_data(input, 1 + listLen);
        return item;
    } else {
        // f8--ff
        auto lenOfListLen = size_t(prefix - 0xf7);
        auto listLen = static_cast<size_t>(parseVarInt(lenOfListLen, input, 1));
        if (listLen < 56) {
            throw std::invalid_argument("length below 56 must be encoded in one byte");
        }
        if (inputLen < lenOfListLen || inputLen < (1 + lenOfListLen + listLen)) {
            throw std::invalid_argument(std::string("Invalid rlp list length, length ") + std::to_string(listLen));
        }
        auto data = sub_data(input, 1 + lenOfListLen, listLen);
        item.decoded.push_back(data);
        item.remainder = sub_data(input, 1 + lenOfListLen + listLen);
        return item;
    }
}

RLP::DecodedItem RLP::decode_list(xbytes_t const & input, std::error_code & ec) {
    RLP::DecodedItem item;
    auto remainder = input;
    while (true) {
        auto list_item = RLP::decode(remainder, ec);
        if (ec) {
            xwarn("RLP::decode_list failed, error code: %s", ec.message().c_str());
            break;
        }

        if (!list_item.decoded.empty()) {
            for (auto & decoded : list_item.decoded) {
                item.decoded.emplace_back(std::move(decoded));
            }
        } else {
            item.decoded.emplace_back();
        }

        if (list_item.remainder.empty()) {
            break;
        }

        remainder = std::move(list_item.remainder);
    }
    return item;
}

RLP::DecodedItem RLP::decode(xbytes_t const & input, std::error_code & ec) {
    assert(!ec);

    if (input.empty()) {
        xwarn("RLP::decode failed, can't decode empty rlp data");
        ec = common::error::xerrc_t::rlp_input_empty;
        return {};
    }

    RLP::DecodedItem item;
    auto const input_len = input.size();
    auto const prefix = input[0];
    if (prefix <= 0x7f) {
        // 00--7f: a single byte whose value is in the [0x00, 0x7f] range, that byte is its own RLP encoding.
        item.decoded.push_back(xbytes_t{input[0]});
        item.remainder = sub_data(input, 1);
        return item;
    }

    if (prefix <= 0xb7) {
        // 80--b7: short string
        // string is 0-55 bytes long. A single byte with value 0x80 plus the length of the string followed by the string
        // The range of the first byte is [0x80, 0xb7]

        // empty string
        if (prefix == 0x80) {
            item.decoded.emplace_back();
            item.remainder = sub_data(input, 1);
            return item;
        }

        size_t const str_len = prefix - 0x80;
        if (str_len == 1 && input[1] <= 0x7f) {
            ec = common::error::xerrc_t::rlp_invalid_encoded_data;
            xwarn("RLP::decode failed, single byte below 128 must be encoded as itself");
            return {};
        }

        if (input_len < (1 + str_len)) {
            ec = common::error::xerrc_t::rlp_invalid_encoded_data;
            xwarn("RLP::decode failed, invalid short string, length %" PRIu64, str_len);
            return {};
        }
        item.decoded.push_back(sub_data(input, 1, str_len));
        item.remainder = sub_data(input, 1 + str_len);

        return item;
    }
    if (prefix <= 0xbf) {
        // b8--bf: long string
        auto const len_of_str_len = static_cast<size_t>(prefix - 0xb7);
        auto const str_len = static_cast<size_t>(parse_variant_int(len_of_str_len, input, 1, ec));
        if (ec) {
            return {};
        }

        if (input_len < len_of_str_len || input_len < (1 + len_of_str_len + str_len)) {
            ec = common::error::xerrc_t::rlp_invalid_encoded_data;
            xwarn("Invalid rlp encoding length, length %zu", str_len);
            return {};
        }
        auto data = sub_data(input, 1 + len_of_str_len, str_len);
        item.decoded.emplace_back(std::move(data));
        item.remainder = sub_data(input, 1 + len_of_str_len + str_len);
        return item;
    }
    if (prefix <= 0xf7) {
        // c0--f7: a list between  0-55 bytes long
        auto const list_len = static_cast<size_t>(prefix - 0xc0);
        if (input_len < (1 + list_len)) {
            ec = common::error::xerrc_t::rlp_invalid_encoded_data;
            xwarn("Invalid rlp string length, length %zu", list_len);
            return {};
        }

        // empty list
        if (list_len == 0) {
            item.remainder = sub_data(input, 1);
            return item;
        }

        // decode list
        auto const list_item = decode_list(sub_data(input, 1, list_len), ec);
        if (ec) {
            xwarn("RLP::decode failed, decode list failed %s", ec.message().c_str());
            return {};
        }

        for (auto & data : list_item.decoded) {
            item.decoded.emplace_back(data);
        }
        item.remainder = sub_data(input, 1 + list_len);
        return item;
    }
    // f8--ff
    auto const len_of_list_len = static_cast<size_t>(prefix - 0xf7);
    auto const list_len = static_cast<size_t>(parse_variant_int(len_of_list_len, input, 1, ec));
    if (ec) {
        xwarn("RLP::decode failed, parse variant int failed %s", ec.message().c_str());
        return {};
    }

    if (list_len < 56) {
        ec = common::error::xerrc_t::rlp_invalid_encoded_data;
        xwarn("RLP::decode failed, length below 56 must be encoded in one byte");
        return {};
    }

    if (input_len < len_of_list_len || input_len < (1 + len_of_list_len + list_len)) {
        ec = common::error::xerrc_t::rlp_invalid_encoded_data;
        xwarn("Invalid rlp list length, length %zu", list_len);
        return {};
    }

    // decode list
    auto const list_item = decode_list(sub_data(input, 1 + len_of_list_len, list_len), ec);
    if (ec) {
        xwarn("RLP::decode failed, decode list failed %s", ec.message().c_str());
        return {};
    }

    for (auto & data : list_item.decoded) {
        item.decoded.push_back(data);
    }
    item.remainder = sub_data(input, 1 + len_of_list_len + list_len);
    return item;
}

RLPStream& RLPStream::appendRaw(bytesConstRef _s, size_t _itemCount)
{
    m_out.insert(m_out.end(), _s.begin(), _s.end());
    noteAppended(_itemCount);
    return *this;
}

void RLPStream::noteAppended(size_t _itemCount)
{
    if (!_itemCount)
        return;
	////std::cout << "noteAppended(" << _itemCount << ")" <<std::endl;
    while (m_listStack.size())
    {
        if (m_listStack.back().first < _itemCount)
            assert(true);
        m_listStack.back().first -= _itemCount;
        if (m_listStack.back().first)
            break;
        else
        {
            auto p = m_listStack.back().second;
            m_listStack.pop_back();
            size_t s = m_out.size() - p;		// list size
            auto brs = bytesRequired(s);
            unsigned encodeSize = s < c_rlpListImmLenCount ? 1 : (1 + brs);
			////std::cout << "s: " << s << ", p: " << p << ", m_out.size(): " << m_out.size() << ", encodeSize: " << encodeSize << " (br: " << brs << ")" <<std::endl;
            auto os = m_out.size();
            m_out.resize(os + encodeSize);
            memmove(m_out.data() + p + encodeSize, m_out.data() + p, os - p);
            if (s < c_rlpListImmLenCount)
                m_out[p] = (xbyte_t)(c_rlpListStart + s);
            else if (c_rlpListIndLenZero + brs <= 0xff)
            {
                m_out[p] = (xbyte_t)(c_rlpListIndLenZero + brs);
                xbyte_t* b = &(m_out[p + brs]);
                for (; s; s >>= 8)
                    *(b--) = (xbyte_t)s;
            }
            else
                assert(true);
        }
        _itemCount = 1;	// for all following iterations, we've effectively appended a single item only since we completed a list.
    }
}

RLPStream& RLPStream::appendList(size_t _items)
{
	//std::cout << "appendList(" << _items << ")" << std::endl;
    if (_items)
        m_listStack.push_back(std::make_pair(_items, m_out.size()));
    else
        appendList(xbytes_t());
    return *this;
}

RLPStream& RLPStream::appendList(bytesConstRef _rlp)
{
    if (_rlp.size() < c_rlpListImmLenCount)
        m_out.push_back((xbyte_t)(_rlp.size() + c_rlpListStart));
    else
        pushCount(_rlp.size(), c_rlpListIndLenZero);
    appendRaw(_rlp, 1);
    return *this;
}

RLPStream& RLPStream::append(bytesConstRef _s, bool _compact)
{
    size_t s = _s.size();
    xbyte_t const* d = _s.data();
    if (_compact)
        for (size_t i = 0; i < _s.size() && !*d; ++i, --s, ++d) {}

    if (s == 1 && *d < c_rlpDataImmLenStart)
        m_out.push_back(*d);
    else
    {
        if (s < c_rlpDataImmLenCount)
            m_out.push_back((xbyte_t)(s + c_rlpDataImmLenStart));
        else
            pushCount(s, c_rlpDataIndLenZero);
        appendRaw(bytesConstRef(d, s), 0);
    }
    noteAppended();
    return *this;
}

RLPStream& RLPStream::append(bigint _i)
{
    if (!_i)
        m_out.push_back(c_rlpDataImmLenStart);
    else if (_i < c_rlpDataImmLenStart)
        m_out.push_back((xbyte_t)_i);
    else
    {
        unsigned br = bytesRequired(_i);
        if (br < c_rlpDataImmLenCount)
            m_out.push_back((xbyte_t)(br + c_rlpDataImmLenStart));
        else
        {
            auto brbr = bytesRequired(br);
            if (c_rlpDataIndLenZero + brbr > 0xff)
                assert(true);
            m_out.push_back((xbyte_t)(c_rlpDataIndLenZero + brbr));
            pushInt(br, brbr);
        }
        pushInt(_i, br);
    }
    noteAppended();
    return *this;
}

RLPStream & RLPStream::append(common::xeth_address_t const & address) {
    return append(address.to_bytes());
}

void RLPStream::pushCount(size_t _count, xbyte_t _base)
{
    auto br = bytesRequired(_count);
    if (int(br) + _base > 0xff)
        assert(true);
    m_out.push_back((xbyte_t)(br + _base));	// max 8 bytes.
    pushInt(_count, br);
}

static void streamOut(std::ostream& _out, RLP const& _d, unsigned _depth = 0)
{
    if (_depth > 64)
        _out << "<max-depth-reached>";
    else if (_d.isNull())
        _out << "null";
    else if (_d.isInt())
        _out << std::showbase << std::hex << std::nouppercase << _d.toInt<bigint>(RLP::LaissezFaire) << std::dec;
    else if (_d.isData())
        _out << escaped(_d.toString(), false);
    else if (_d.isList())
    {
        _out << "[";
        int j = 0;
        for (auto i: _d)
        {
            _out << (j++ ? ", " : " ");
            streamOut(_out, i, _depth + 1);
        }
        _out << " ]";
    }
}

std::ostream& operator<<(std::ostream& _out, RLP const& _d)
{
    streamOut(_out, _d);
    return _out;
}


}
}