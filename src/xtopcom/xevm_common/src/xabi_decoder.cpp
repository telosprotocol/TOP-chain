// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/xabi_decoder.h"

#include "xbasic/endianness.h"
#include "xbasic/xerror/xerror.h"

#include <endian.h>

NS_BEG2(top, evm_common)

xtop_abi_decoder::xtop_abi_decoder(xbytes_t input_data) : m_data{std::move(input_data)} {
    if (m_data.size() < FS_BYTES_SIZE) {
        top::error::throw_error(error::xerrc_t::abi_data_length_error, "wrong method id");
    }

    if (m_data.size() % BYTES_WIDTH != FS_BYTES_SIZE) {
        top::error::throw_error(error::xerrc_t::abi_data_length_error, "wrong parameters");
    }

    m_max_data_field_number = m_data.size() / BYTES_WIDTH;
}

xtop_abi_decoder::xtop_abi_decoder(xbytes_t input_data, std::error_code & ec) : m_data{input_data} {
    assert(!ec);
    // assert(m_data.size() % 32 == FS_BYTES_SIZE);
    if (m_data.size() % BYTES_WIDTH != FS_BYTES_SIZE) {
        ec = error::xerrc_t::abi_data_length_error;
    }
    m_max_data_field_number = m_data.size() / BYTES_WIDTH;
}

xtop_abi_decoder xtop_abi_decoder::build_from_hex_string(std::string const & input_hex_string, std::error_code & ec) {
    assert(!ec);
    auto hex_data = top::from_hex(input_hex_string, ec);
    if (!ec) {
        return xtop_abi_decoder{hex_data, ec};
    }
    return {};
}

xtop_abi_decoder xtop_abi_decoder::build_from(xbytes_t const & input, std::error_code & ec) {
    try {
        return xtop_abi_decoder{input};
    } catch (top::error::xtop_error_t const & eh) {
        ec = eh.code();
    }
    return {};
}

bool xtop_abi_decoder::empty() const noexcept {
    return m_decoded_count >= m_max_data_field_number;
}

size_t xtop_abi_decoder::size() const noexcept {
    return m_max_data_field_number;
}

size_t xtop_abi_decoder::unconsumed_size() const noexcept {
    if (m_decoded_count < m_max_data_field_number) {
        return m_max_data_field_number - m_decoded_count;
    }

    return 0;
}

bool xtop_abi_decoder::decode_bool(std::error_code & ec) {
    assert(!ec);
    // assert(m_decoded_count <= m_max_data_field_number);
    if (m_decoded_count > m_max_data_field_number) {
        ec = error::xerrc_t::abi_decode_outofrange;
        return false;
    }
    return decode_as_u256(m_decoded_count++).convert_to<bool>();
}

/// decode address:
common::xeth_address_t xtop_abi_decoder::decode_address(std::error_code & ec) {
    assert(!ec);
    if (m_decoded_count > m_max_data_field_number) {
        ec = error::xerrc_t::abi_decode_outofrange;
        return {};
    }
    return decode_as_address(m_decoded_count++);
}

/// decode into string
/// Noted: always with "memory" parameter
std::string xtop_abi_decoder::decode_string(std::error_code & ec) {
    assert(!ec);
    auto bytes_res = decode_bytes(ec);
    if (ec) {
        return "";
    }
    return top::to_string(bytes_res);
}

/// decode into bytes
/// Noted: always with "memory" parameter
xbytes_t xtop_abi_decoder::decode_bytes(std::error_code & ec) {
    assert(!ec);
    return do_decode_bytes(0, m_decoded_count++, ec);
}

/// decode into bytes<M> with `0 < M <= 32`
xbytes_t xtop_abi_decoder::decode_bytes(std::size_t sz, std::error_code & ec) {
    if (sz == 0 || sz > BYTES_WIDTH) {
        ec = error::xerrc_t::abi_data_length_error;
        return {};
    }
    if (m_decoded_count > m_max_data_field_number) {
        ec = error::xerrc_t::abi_decode_outofrange;
        return {};
    }
    if (m_data.size() < FS_BYTES_SIZE + m_decoded_count * BYTES_WIDTH + sz) {
        ec = error::xerrc_t::abi_decode_outofrange;
        return {};
    }
    return at_raw(m_decoded_count++, sz);
}

template <>
evm_common::xfunction_selector_t xtop_abi_decoder::extract<evm_common::xfunction_selector_t>(std::error_code & ec) {
    assert(!ec);

    assert(m_data.size() >= FS_BYTES_SIZE);
    xfunction_selector_t func_selector;

    std::memcpy(std::addressof(func_selector.method_id), m_data.data(), 4);
    func_selector.method_id = be32toh(func_selector.method_id);

    return func_selector;
}

template <>
common::xeth_address_t xtop_abi_decoder::extract<common::xeth_address_t>(std::error_code & ec) {
    assert(!ec);

    if (empty()) {
        ec = error::xerrc_t::abi_decode_outofrange;
        return {};
    }

    std::array<uint8_t, 20> addr_data;
    auto value_u256 = extract<evm_common::u256>(ec);
    if (ec) {
        return {};
    }
    auto max_u160 = std::numeric_limits<evm_common::u160>::max();
    if (value_u256 > static_cast<evm_common::u256>(max_u160)) {
        ec = error::xerrc_t::abi_data_value_error;
        return {};
    }

    auto value_u160 = value_u256.convert_to<evm_common::u160>();
    auto bytes20 = top::to_bytes(value_u160);

    return common::xeth_address_t::build_from(bytes20, ec);
}

template <>
bool xtop_abi_decoder::extract<bool>(std::error_code & ec) {
    assert(!ec);

    return decode_bool(ec);
}

template <>
int8_t xtop_abi_decoder::extract<int8_t>(std::error_code & ec) {
    assert(!ec);

    return decode_int<int8_t>(ec);
}

template <>
int16_t xtop_abi_decoder::extract<int16_t>(std::error_code & ec) {
    assert(!ec);

    return decode_int<int16_t>(ec);
}

template <>
uint16_t xtop_abi_decoder::extract<uint16_t>(std::error_code & ec) {
    assert(!ec);

    return decode_uint<uint16_t>(ec);
}

template <>
uint32_t xtop_abi_decoder::extract<uint32_t>(std::error_code & ec) {
    assert(!ec);

    return decode_uint<uint32_t>(ec);
}

template <>
uint64_t xtop_abi_decoder::extract<uint64_t>(std::error_code & ec) {
    assert(!ec);

    return decode_uint<uint64_t>(ec);
}

template <>
s256 xtop_abi_decoder::extract<s256>(std::error_code & ec) {
    assert(!ec);

    return decode_int<s256>(ec);
}

template <>
evm_common::u256 xtop_abi_decoder::extract<evm_common::u256>(std::error_code & ec) {
    assert(!ec);

    if (empty()) {
        ec = error::xerrc_t::abi_decode_outofrange;
        return {};
    }

    return top::from_bytes<evm_common::u256>(at(m_decoded_count++), ec);
}

template <>
std::string xtop_abi_decoder::extract<std::string>(std::error_code & ec) {
    assert(!ec);

    return decode_string(ec);
}

template <>
xbytes_t xtop_abi_decoder::extract<xbytes_t>(std::error_code & ec) {
    assert(!ec);

    return decode_bytes(ec);
}

NS_END2
