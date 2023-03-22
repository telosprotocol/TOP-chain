// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xhex.h"
#include "xbasic/xstring.h"
#include "xcommon/xeth_address.h"
#include "xcommon/common.h"
#include "xcommon/common_data.h"
#include "xevm_common/xerror/xerror.h"

#include <cassert>
#include <string>
#include <system_error>
#include <type_traits>
#include <vector>

NS_BEG2(top, evm_common)

struct xtop_function_selector {
    uint32_t method_id{0};
};
using xfunction_selector_t = xtop_function_selector;

class xtop_abi_decoder {
public:
    static constexpr std::size_t function_selector_size = 4;
    static constexpr std::size_t solidity_word_size = 32;

private:
    xbytes_t m_data;
    std::size_t m_max_data_field_number{0};
    mutable std::size_t m_decoded_count{0};

public:
    xtop_abi_decoder() = default;
    xtop_abi_decoder(xtop_abi_decoder const &) = delete;
    xtop_abi_decoder & operator=(xtop_abi_decoder const &) = delete;
    xtop_abi_decoder(xtop_abi_decoder &&) = default;
    xtop_abi_decoder & operator=(xtop_abi_decoder &&) = default;
    ~xtop_abi_decoder() = default;

private:
    explicit xtop_abi_decoder(xbytes_t input_data);

public:
    static xtop_abi_decoder build_from_hex_string(std::string const & input_hex_string, std::error_code & ec);
    static xtop_abi_decoder build_from_hex_string(std::string const & input_hex_string);
    static xtop_abi_decoder build_from(xbytes_t const & raw_data, std::error_code & ec);
    static xtop_abi_decoder build_from(xbytes_t && raw_data, std::error_code & ec);

    template <typename T>
    T extract(std::error_code & ec) const;

    template <typename T>
    T extract() const;

    bool empty() const noexcept;

    size_t size() const noexcept;

    size_t unconsumed_size() const noexcept;

    /// decode into bytes<M> with `0 < M <= 32`
    xbytes_t decode_bytes(std::size_t sz, std::error_code & ec);

    /// decode array[T] with type T
    /// Noted: always with "memory" parameter
    template <typename ElementType>
    std::vector<ElementType> decode_array(std::error_code & ec) {
        assert(!ec);
        return do_decode_array<ElementType>(0, m_decoded_count++, ec);
    }

private:
    /// decode into bool
    bool decode_bool(std::error_code & ec) const;

    /// decode address:
    common::xeth_address_t decode_address(std::error_code & ec);

    /// decode into string
    /// Noted: always with "memory" parameter
    std::string decode_string(std::error_code & ec) const;

    /// decode into bytes
    /// Noted: always with "memory" parameter
    xbytes_t decode_bytes(std::error_code & ec) const;

    /// decode into uint8/16/32/64/....
    /// or boost bigInt unsigned: u256/u160...
    template <typename UnsignedInteger>
    UnsignedInteger decode_uint(std::error_code & ec) const {
        assert(!ec);
        // assert(m_decoded_count <= m_max_data_field_number);
        if (m_decoded_count > m_max_data_field_number) {
            ec = error::xerrc_t::abi_decode_outofrange;
            return UnsignedInteger{};
        }
        static_assert(std::is_unsigned<UnsignedInteger>::value ||
                          (boost::multiprecision::is_number<UnsignedInteger>::value && boost::multiprecision::is_unsigned_number<UnsignedInteger>::value),
                      "UnsignedInteger value only");
        return decode_as_u256(m_decoded_count++).convert_to<UnsignedInteger>();
    }

    /// decode into int8/16/32/64/....
    /// or boost bigInt signed: s256/s160...
    template <typename SignedInteger>
    SignedInteger decode_int(std::error_code & ec) const {
        assert(!ec);
        // assert(m_decoded_count <= m_max_data_field_number);
        if (m_decoded_count > m_max_data_field_number) {
            ec = error::xerrc_t::abi_decode_outofrange;
            return SignedInteger{};
        }
        static_assert(
            std::is_signed<SignedInteger>::value || (boost::multiprecision::is_number<SignedInteger>::value && boost::multiprecision::is_signed_number<SignedInteger>::value),
            "SignedInteger value only");
        return decode_as_s256(m_decoded_count++).convert_to<SignedInteger>();
    }

private:
    template <typename VarientType, typename std::enable_if<std::is_same<VarientType, common::xeth_address_t>::value>::type * = nullptr>
    VarientType variant_decode(std::size_t start_pos, std::size_t offset, std::error_code & ec) {
        return decode_as_address(start_pos + offset);
    }

    // is unsigned intergral
    template <typename VarientType,
              typename std::enable_if<(std::is_integral<VarientType>::value || boost::multiprecision::is_number<VarientType>::value) &&
                                      (std::is_unsigned<VarientType>::value ||
                                       (boost::multiprecision::is_number<VarientType>::value && boost::multiprecision::is_unsigned_number<VarientType>::value))>::type * = nullptr>
    VarientType variant_decode(std::size_t start_pos, std::size_t offset, std::error_code & ec) {
        return decode_as_u256(start_pos + offset).convert_to<VarientType>();
    }

    // is signed intergral
    template <typename VarientType,
              typename std::enable_if<(std::is_integral<VarientType>::value || boost::multiprecision::is_number<VarientType>::value) &&
                                      (std::is_signed<VarientType>::value ||
                                       (boost::multiprecision::is_number<VarientType>::value && boost::multiprecision::is_signed_number<VarientType>::value))>::type * = nullptr>
    VarientType variant_decode(std::size_t start_pos, std::size_t offset, std::error_code & ec) {
        return decode_as_s256(start_pos + offset).convert_to<VarientType>();
    }

    // is inner array
    // return type VarientType is std::vector<T>
    template <typename VarientType, typename std::enable_if<std::is_same<VarientType, std::vector<typename VarientType::value_type>>::value>::type * = nullptr>
    VarientType variant_decode(std::size_t start_pos, std::size_t offset, std::error_code & ec) {
        typedef typename VarientType::value_type innerType;
        return do_decode_array<innerType>(start_pos, offset, ec);
    }

    // is inner bytes
    // return type VarientType is std::string
    template <typename VarientType, typename std::enable_if<std::is_same<VarientType, std::string>::value>::type * = nullptr>
    VarientType variant_decode(std::size_t start_pos, std::size_t offset, std::error_code & ec) {
        return top::to_string(do_decode_bytes(start_pos, offset, ec));
    }

    // decode as array
    template <typename ElementType>
    std::vector<ElementType> do_decode_array(std::size_t start_pos, std::size_t offset_pos, std::error_code & ec) {
        if (start_pos + offset_pos > m_max_data_field_number) {
            ec = error::xerrc_t::abi_decode_outofrange;
            return {};
        }
        auto const real_position = decode_as_real_position(start_pos + offset_pos, ec) + start_pos;
        if (ec) {
            return {};
        }
        auto const sz = decode_as_u256(real_position);
        std::vector<ElementType> res;
        for (std::size_t data_pos_offset = 0; data_pos_offset < sz; ++data_pos_offset) {
            auto r = variant_decode<ElementType>(real_position + 1, data_pos_offset, ec);
            if (ec)
                break;
            res.push_back(r);
        }
        return res;
    }

    // decode as bytes
    xbytes_t do_decode_bytes(std::size_t const start_pos, std::size_t const offset_pos, std::error_code & ec) const {
        if (start_pos + offset_pos > m_max_data_field_number) {
            ec = error::xerrc_t::abi_decode_outofrange;
            return {};
        }
        auto const real_position = decode_as_real_position(start_pos + offset_pos, ec) + start_pos;
        if (ec) {
            return {};
        }
        auto const sz = decode_as_u256(real_position);
        if (m_data.size() < function_selector_size + (real_position + 1) * solidity_word_size + sz.convert_to<std::size_t>()) {
            ec = error::xerrc_t::abi_decode_outofrange;
            return {};
        }
        return at_raw(real_position + 1, sz.convert_to<std::size_t>());
    }

    xbytes_t at(std::size_t const pos) const {
        auto const begin = std::next(std::begin(m_data), static_cast<ptrdiff_t>(function_selector_size + pos * solidity_word_size));
        auto const end = std::next(begin, solidity_word_size);

        return xbytes_t{begin, end};
    }

    xbytes_t at_raw(std::size_t const start_pos, std::size_t const sz) const {
        auto const begin = std::next(std::begin(m_data), static_cast<ptrdiff_t>(function_selector_size + start_pos * solidity_word_size));
        auto const end = std::next(begin, static_cast<ptrdiff_t>(sz));

        return xbytes_t{begin, end};
    }

    std::size_t decode_as_real_position(std::size_t const field_num, std::error_code & ec) const {
        auto const bytes_position = decode_as_u256(field_num).convert_to<std::size_t>();
        if (bytes_position % solidity_word_size != 0) {
            ec = error::xerrc_t::abi_data_value_error;
            return 0;
        }
        assert(!ec);
        assert(bytes_position % solidity_word_size == 0);

        if (bytes_position / solidity_word_size > m_max_data_field_number) {
            ec = error::xerrc_t::abi_decode_outofrange;
            return 0;
        }

        return bytes_position / solidity_word_size;
    }

    u256 decode_as_u256(std::size_t const field_num) const {
        return fromBigEndian<u256>(at(field_num));
    }

    s256 decode_as_s256(std::size_t const field_num) const {
        return u2s(fromBigEndian<u256>(at(field_num)));
    }

    common::xeth_address_t decode_as_address(std::size_t field_num) {
        std::array<uint8_t, 20> addr_data;
        auto u160_value = decode_as_u256(field_num).convert_to<u160>();
        auto bytes20 = toBigEndian(u160_value);
        std::copy(std::begin(bytes20), std::end(bytes20), std::begin(addr_data));
        return common::xeth_address_t{addr_data};
    }

    // template <class T, class _In>
    // inline T fromBigEndian(_In const & _bytes) {
    //     T ret = (T)0;
    //     for (auto i : _bytes)
    //         ret = (T)((ret << 8) | (byte)(typename std::make_unsigned<decltype(i)>::type)i);
    //     return ret;
    // }
};
using xabi_decoder_t = xtop_abi_decoder;

template <>
xfunction_selector_t xtop_abi_decoder::extract<xfunction_selector_t>(std::error_code & ec) const;

template <>
xfunction_selector_t xtop_abi_decoder::extract<xfunction_selector_t>() const;

template <>
common::xeth_address_t xtop_abi_decoder::extract<common::xeth_address_t>(std::error_code & ec) const;

template <>
bool xtop_abi_decoder::extract<bool>(std::error_code & ec) const;

template <>
int8_t xtop_abi_decoder::extract<int8_t>(std::error_code & ec) const;

template <>
int16_t xtop_abi_decoder::extract<int16_t>(std::error_code & ec) const;

template <>
uint16_t xtop_abi_decoder::extract<uint16_t>(std::error_code & ec) const;

template <>
uint32_t xtop_abi_decoder::extract<uint32_t>(std::error_code & ec) const;

template <>
uint64_t xtop_abi_decoder::extract<uint64_t>(std::error_code & ec) const;

//template <>
//s256 xtop_abi_decoder::extract<s256>(std::error_code & ec) const;

template <>
u256 xtop_abi_decoder::extract<u256>(std::error_code & ec) const;

template <>
std::string xtop_abi_decoder::extract<std::string>(std::error_code & ec) const;

template <>
std::string xtop_abi_decoder::extract<std::string>() const;

template <>
xbytes_t xtop_abi_decoder::extract<xbytes_t>(std::error_code & ec) const;

NS_END2
