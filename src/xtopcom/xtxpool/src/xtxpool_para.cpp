// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool/xtxpool_para.h"
#include "xconfig/xconfig_register.h"

NS_BEG2(top, xtxpool)

#define TIMER_HEIGHT_PER_DAY (24 * 360)

xtxpool_resources::xtxpool_resources(const observer_ptr<store::xstore_face_t> & store,
                                     const xobject_ptr_t<base::xvblockstore_t> & blockstore,
                                     const observer_ptr<mbus::xmessage_bus_face_t> &bus,
                                     const xobject_ptr_t<base::xvcertauth_t> & certauth,
                                     const observer_ptr<time::xchain_time_face_t> & clock,
                                     enum_xtxpool_order_strategy strategy)
  : m_store(store)
  , m_blockstore(blockstore)
  , m_bus(bus)
  , m_certauth(certauth)
  , m_clock(clock)
  , m_strategy(strategy) {
    auto const windowday = XGET_CONFIG(recv_tx_cache_window);
    m_receipt_valid_window = windowday * TIMER_HEIGHT_PER_DAY;
  }

xtxpool_resources::~xtxpool_resources() {}

store::xstore_face_t * xtxpool_resources::get_store() const {
    return m_store.get();
}
base::xvblockstore_t * xtxpool_resources::get_vblockstore() const {
    return m_blockstore.get();
}
mbus::xmessage_bus_face_t * xtxpool_resources::get_mbus() const {
    return m_bus.get();
}
base::xvcertauth_t * xtxpool_resources::get_certauth() const {
    return m_certauth.get();
}
time::xchain_time_face_t * xtxpool_resources::get_chain_timer() const {
    return m_clock.get();
}
enum_xtxpool_order_strategy xtxpool_resources::get_order_strategy() const {
    return m_strategy;
}

uint64_t xtxpool_resources::get_receipt_valid_window() const {
    return m_receipt_valid_window;
}

void xtxpool_resources::set_receipt_valid_window_day(uint64_t day) {
    m_receipt_valid_window = day * TIMER_HEIGHT_PER_DAY;
}

NS_END2
