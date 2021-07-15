// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xip.h"
#include "xcommon/xnode_id.h"
#include "xdata/xelect_transaction.hpp"
#include "xrouter/xrouter_face.h"

#include <cstdint>

NS_BEG2(top, router)

class xtop_router : public xrouter_face_t {
public:
    common::xsharding_address_t sharding_address_from_account(common::xaccount_address_t const & target_account,
                                                              common::xnetwork_id_t const & nid,
                                                              common::xnode_type_t type) const override;

    common::xsharding_address_t sharding_address_from_tableindex(base::xtable_index_t const & target_tableindex,
                                                                      common::xnetwork_id_t const & nid,
                                                                      common::xnode_type_t type) const override;

    common::xsharding_address_t address_of_book_id(std::uint16_t const table_id,
                                                   common::xnode_type_t type,
                                                   common::xnetwork_id_t const & nid) const override;

    common::xsharding_address_t address_of_table_id(std::uint16_t const book_id,
                                                    common::xnode_type_t type,
                                                    common::xnetwork_id_t const & nid) const override;

private:
    common::xsharding_address_t do_address_of_book_id(std::uint16_t const book_id,
                                                      common::xnode_type_t type,
                                                      common::xnetwork_id_t const & nid) const;

    common::xsharding_address_t do_address_of_table_id(std::uint16_t const table_id,
                                                       common::xnode_type_t type,
                                                       common::xnetwork_id_t const & nid) const;
};
using xrouter_t = xtop_router;

NS_END2
