// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstatestore/xstatestore_resource.h"

NS_BEG2(top, statestore)

xstatestore_resources_t::xstatestore_resources_t(){
}

xstatestore_resources_t::~xstatestore_resources_t(){

}

void xstatestore_resources_t::set_prune_dispatcher(const observer_ptr<statestore_prune_dispatcher_t> & prune_dispatcher) {
    m_prune_dispatcher = prune_dispatcher;
}
statestore_prune_dispatcher_t * xstatestore_resources_t::get_prune_dispatcher() {
    return m_prune_dispatcher.get();
}

NS_END2
