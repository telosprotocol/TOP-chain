// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xplugin/xplugin_manager.h"
#include "xpreprocess/xpreprocess.h"

using namespace std;

NS_BEG2(top, data)

xpreprocess & xpreprocess::instance() {
    static xpreprocess ins;
    return ins;
}

xpreprocess::xpreprocess() {
    m_handler = top::make_unique<xplugin_manager>();
}

bool xpreprocess::send(xmessage_t & msg) {
    return m_handler->handle(msg);
}

NS_END2