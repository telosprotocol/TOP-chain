// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>

#include "xcommon/xnode_id.h"
#include "xbase/xutl.h"
#include "xcrypto/xckey.h"
#include "xverifier_errors.h"
#include "xdata/xtransaction.h"


NS_BEG2(top, xverifier)

/// the length defintion of the private key
XINLINE_CONSTEXPR std::size_t PRIKEY_LEN = 32;
/// the uncompressed & compressed length definition of the public key
XINLINE_CONSTEXPR std::size_t UNCOMPRESSED_PUBKEY_LEN = 65;
XINLINE_CONSTEXPR std::size_t COMPRESSED_PUBKEY_LEN = 33;


class xtx_utl {
public:
    /**
     * @brief judge address valid
     *
     * @param addr  the address to verify(string format)
     * @return int32_t  see xverifier_errors definition
     */

    static int32_t  address_is_valid(const std::string & addr, bool isTransaction = false);
    /**
     * @brief judge address valid
     *
     * @param addr  the address to verify(xnode_id_t format)
     * @return int32_t  see xverifier_errors definition
     */
    static int32_t  address_is_valid(common::xnode_id_t const& addr);

    /**
     * @brief check private key and public key whether matched
     *
     * @param privkey  the private key
     * @param pubkey  the public key
     * @return int32_t  see xverifier_errors definition
     */
    static int32_t  privkey_pubkey_is_match(std::string const& privkey, std::string const& pubkey);

    /**
     * @brief second format of the gmt time
     *
     * @return uint64_t  the seconds
     */
    static uint64_t get_gmttime_s();

    /**
     * @brief judge sendtx whether by normal contract(T-3)
     *
     * @param tx_ptr  the transaction to verify
     * @return int32_t  see xverifier_errors definition
     */
    static int32_t  judge_normal_contract_sendtx(data::xtransaction_ptr_t const& tx_ptr);

    static bool is_valid_hex_format(std::string const& str);
};


NS_END2
