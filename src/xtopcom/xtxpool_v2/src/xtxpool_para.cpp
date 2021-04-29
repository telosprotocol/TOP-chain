// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xtxpool_para.h"

#include "xconfig/xconfig_register.h"

NS_BEG2(top, xtxpool_v2)

#define TIMER_HEIGHT_PER_DAY (24 * 360)

xtxpool_resources::xtxpool_resources(const observer_ptr<store::xstore_face_t> & store,
                                     const observer_ptr<base::xvblockstore_t> & blockstore,
                                     const observer_ptr<base::xvcertauth_t> & certauth,
                                     const observer_ptr<store::xindexstorehub_t> & indexstorehub,
                                     const observer_ptr<mbus::xmessage_bus_face_t> & bus)
  : m_store(store), m_blockstore(blockstore), m_certauth(certauth), m_indexstorehub(indexstorehub), m_bus(bus) {
}

xtxpool_resources::~xtxpool_resources() {
}

store::xstore_face_t * xtxpool_resources::get_store() const {
    return m_store.get();
}
base::xvblockstore_t * xtxpool_resources::get_vblockstore() const {
    return m_blockstore.get();
}
base::xvcertauth_t * xtxpool_resources::get_certauth() const {
    return m_certauth.get();
}
store::xindexstorehub_t * xtxpool_resources::get_indexstorehub() const {
    return m_indexstorehub.get();
}
mbus::xmessage_bus_face_t * xtxpool_resources::get_bus() const {
    return m_bus.get();
}

NS_END2
