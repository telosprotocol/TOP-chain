// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xdata.h"
#include "xbasic/xmemory.hpp"
#include "xstore/xstore_face.h"
#include "xtxpool_service_v2/xtxpool_service_face.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xvledger/xvtxstore.h"
#include "xvledger/xvledger.h"
#include <atomic>
#include <string>

NS_BEG2(top, xtxpool_service_v2)

class xtxpool_svc_para_t : public base::xobject_t {
public:
    xtxpool_svc_para_t(const observer_ptr<store::xstore_face_t> & store,
                       const observer_ptr<base::xvblockstore_t> & blockstore,
                       const observer_ptr<xtxpool_v2::xtxpool_face_t> & txpool,
                       const observer_ptr<mbus::xmessage_bus_face_t> & bus)
      : m_store(store), m_blockstore(blockstore), m_txpool(txpool), m_bus(bus) {
    }

public:
    void set_dispatchers(const observer_ptr<xtxpool_service_dispatcher_t> & fast_dispatcher, const observer_ptr<xtxpool_service_dispatcher_t> & slow_dispatcher) {
        m_fast_dispatcher = fast_dispatcher;
        m_slow_dispatcher = slow_dispatcher;
    };
    store::xstore_face_t * get_store() const {
        return m_store.get();
    }
    base::xvblockstore_t * get_vblockstore() const {
        return m_blockstore.get();
    }
    xtxpool_v2::xtxpool_face_t * get_txpool() const {
        return m_txpool.get();
    }
    xtxpool_service_dispatcher_t * get_fast_dispatcher() const {
        return m_fast_dispatcher.get();
    }
    xtxpool_service_dispatcher_t * get_slow_dispatcher() const {
        return m_slow_dispatcher.get();
    }
    mbus::xmessage_bus_face_t * get_bus() const {
        return m_bus.get();
    }
    base::xvtxstore_t * get_vtxstore() const {
        return base::xvchain_t::instance().get_xtxstore();
    }
private:
    observer_ptr<store::xstore_face_t> m_store{nullptr};
    observer_ptr<base::xvblockstore_t> m_blockstore{nullptr};
    observer_ptr<xtxpool_v2::xtxpool_face_t> m_txpool{nullptr};
    observer_ptr<xtxpool_service_dispatcher_t> m_fast_dispatcher{nullptr};
    observer_ptr<xtxpool_service_dispatcher_t> m_slow_dispatcher{nullptr};
    observer_ptr<mbus::xmessage_bus_face_t> m_bus{nullptr};
};

NS_END2
