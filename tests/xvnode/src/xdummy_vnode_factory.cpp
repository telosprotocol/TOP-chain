// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xvnode/xdummy_vnode_factory.h"

#include "tests/xvnode/xdummy_vhost.h"

NS_BEG3(top, tests, vnode)

xdummy_vnode_factory dummy_vnode_factory{top::make_observer(&xdummy_vhost)};

NS_END3
