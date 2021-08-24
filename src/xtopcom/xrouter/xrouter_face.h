// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#pragma once

#include <memory>
#include <string>

#include "xvledger/xvaccount.h"
#include "xcommon/xaddress.h"
#include "xcommon/xnode_type.h"

NS_BEG2(top, router)

/**
 * @brief Map account address, table id or book id to its corresponding sharding address
 *
 */
struct xtop_router_face {
    /**
     * @brief Map account to its corresponding sharding address. If the account is a contract account or table account, type is not considered.
     *
     * @param target_account                The account address.
     * @param nid                           The network id.
     * @param type                          The type hint for calculating the sharding address. It is used when the target_account is a user account and the type is only common::xnode_type_t::consensus_auditor or common::xnode_type_t::consensus_validator
     * @return common::xsharding_address_t  The sharding address the target_account belongs to.
     */
    virtual common::xsharding_address_t sharding_address_from_account(common::xaccount_address_t const & target_account,
                                                                      common::xnetwork_id_t const & nid,
                                                                      common::xnode_type_t type) const = 0;

    /**
     * @brief Map account to its corresponding sharding address. If the account is a contract account or table account, type is not considered.
     *
     * @param target_account                The account address.
     * @param nid                           The network id.
     * @param type                          The type hint for calculating the sharding address. It is used when the target_account is a user account and the type is only common::xnode_type_t::consensus_auditor or common::xnode_type_t::consensus_validator
     * @return common::xsharding_address_t  The sharding address the target_account belongs to.
     */
    virtual common::xsharding_address_t sharding_address_from_tableindex(base::xtable_index_t const & target_tableindex,
                                                                      common::xnetwork_id_t const & nid,
                                                                      common::xnode_type_t type) const = 0;


    /**
     * @brief Map table id to its corresponding sharding address.
     *
     * @param table_id                      Table id.
     * @param type                          The sharding type.
     * @param nid                           The network id.
     * @return common::xsharding_address_t  The sharding address the table id belongs to.
     */
    virtual common::xsharding_address_t address_of_table_id(std::uint16_t const table_id,
                                                            common::xnode_type_t type,
                                                            common::xnetwork_id_t const & nid) const = 0;

    /**
     * @brief Map book id to its corresponding sharding address.
     *
     * @param book_id                       The book id.
     * @param type                          The sharding type.
     * @param nid                           The network id.
     * @return common::xsharding_address_t  The sharding address the book id belongs to.
     */
    virtual common::xsharding_address_t address_of_book_id(std::uint16_t const book_id,
                                                           common::xnode_type_t type,
                                                           common::xnetwork_id_t const & nid) const = 0;
};
using xrouter_face_t = xtop_router_face;

NS_END2
