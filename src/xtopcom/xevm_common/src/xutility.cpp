// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "xevm_common/xcrosschain/xbsc/xutility.h"
#include "xevm_common/xerror/xerror.h"
#include "xcommon/rlp.h"

NS_BEG4(top, evm, crosschain, bsc)

auto get_validator_bytes_from_header(evm_common::xeth_header_t const & header,
                                     xchain_config_t const & chain_config,
                                     xparlia_config_t const & parlia_config,
                                     std::error_code & ec) -> xbytes_t{
    assert(!ec);

    if (header.extra.size() <= EXTRA_VANITY + EXTRA_SEAL) {
        ec = evm_common::error::xerrc_t::invalid_bsc_extra_data;
        return {};
    }

    if (!chain_config.is_luban(header.number)) {
        ec = evm_common::error::xerrc_t::bsc_header_before_luban;
        return {};
    }

    if (header.number % parlia_config.epoch != 0) {
        ec = evm_common::error::xerrc_t::bsc_not_epoch_header;
        return {};
    }

    auto const num = static_cast<int32_t>(header.extra[EXTRA_VANITY]);
    if (num == 0 || header.extra.size() <= EXTRA_VANITY + EXTRA_SEAL + num * VALIDATOR_BYTES_LENGTH) {
        ec = evm_common::error::xerrc_t::invalid_bsc_extra_data;
        return {};
    }

    constexpr size_t start = EXTRA_VANITY + VALIDATOR_NUMBER_SIZE;
    size_t const end = start + num * VALIDATOR_BYTES_LENGTH;

    return xbytes_t{std::next(header.extra.begin(), static_cast<ptrdiff_t>(start)), std::next(header.extra.begin(), static_cast<ptrdiff_t>(end))};
}

auto get_vote_attestation_from_header(evm_common::xeth_header_t const & header,
                                      xchain_config_t const & chain_config,
                                      xparlia_config_t const & parlia_config,
                                      std::error_code & ec) -> xvote_attestation_t {
    assert(!ec);
    if (header.extra.size() <= EXTRA_VANITY + EXTRA_SEAL) {
        ec = evm_common::error::xerrc_t::invalid_bsc_extra_data;
        return {};
    }

    if (!chain_config.is_luban(header.number)) {
        ec = evm_common::error::xerrc_t::bsc_header_before_luban;
        return {};
    }

    xbytes_t attesttion_bytes;
    if (header.number % parlia_config.epoch != 0) {
        auto const b = std::next(std::begin(header.extra), static_cast<ptrdiff_t>(EXTRA_VANITY));
        auto const e = std::next(std::end(header.extra), -static_cast<ptrdiff_t>(EXTRA_SEAL));
        attesttion_bytes = xbytes_t{b, e};
    } else {
        auto const num = static_cast<int32_t>(header.extra[EXTRA_VANITY]);
        if (header.extra.size() <= EXTRA_VANITY + EXTRA_SEAL + VALIDATOR_NUMBER_SIZE + num * VALIDATOR_BYTES_LENGTH) {
            ec = evm_common::error::xerrc_t::invalid_bsc_extra_data;
            return {};
        }

        auto const b = std::next(std::begin(header.extra), static_cast<ptrdiff_t>(EXTRA_VANITY + VALIDATOR_NUMBER_SIZE + num * VALIDATOR_BYTES_LENGTH));
        auto const e = std::next(std::end(header.extra), -static_cast<ptrdiff_t>(EXTRA_SEAL));
        attesttion_bytes = xbytes_t{b, e};
    }

    // auto const & decoded_item = evm_common::RLP::decode_list(attesttion_bytes, ec);
    xvote_attestation_t attestation;
    assert(false);
    return attestation;
}

NS_END4
