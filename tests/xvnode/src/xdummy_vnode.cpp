// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xvnode/xdummy_vnode.h"

NS_BEG3(top, tests, vnode)

#define XDUMMY_VNODE_CONSTRUCTOR(TYPE)  xdummy_vnode<top::common::xnode_type_t::TYPE>::xdummy_vnode(top::common::xnode_address_t address) : address_{std::move(address)} {}

XDUMMY_VNODE_CONSTRUCTOR(committee)
XDUMMY_VNODE_CONSTRUCTOR(zec)
XDUMMY_VNODE_CONSTRUCTOR(storage_archive)
XDUMMY_VNODE_CONSTRUCTOR(edge)
XDUMMY_VNODE_CONSTRUCTOR(consensus_auditor)
XDUMMY_VNODE_CONSTRUCTOR(consensus_validator)

#undef XDUMMY_VNODE_CONSTRUCTOR

NS_END3
