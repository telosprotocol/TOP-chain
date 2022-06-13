// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject_ptr.h"
#include "xdata/xtransaction.h"
#include "xdata/xrelay_block.h"
#include "xstore/xstore_face.h"
#include "xvledger/xvblockstore.h"
#include "xmbus/xmessage_bus.h"

NS_BEG2(top, xrelay_chain)

class xrelay_chain_resources  {
public:
    xrelay_chain_resources(const observer_ptr<store::xstore_face_t> & store,
                           const observer_ptr<base::xvblockstore_t> & blockstore,
                           const observer_ptr<mbus::xmessage_bus_face_t> & bus);

public:
    store::xstore_face_t * get_store() const;
    base::xvblockstore_t * get_vblockstore() const;
    mbus::xmessage_bus_face_t * get_bus() const;

private:
    observer_ptr<store::xstore_face_t> m_store;
    observer_ptr<base::xvblockstore_t> m_blockstore;
    observer_ptr<mbus::xmessage_bus_face_t> m_bus;
};

class xrelay_chain_mgr_dispatcher_t;

struct xcross_txs_t {
    std::vector<data::xtransaction_ptr_t> m_txs;
    std::vector<data::xeth_store_receipt_t> m_tx_results;
};

class xcross_tx_cache_t {
public:
    xcross_tx_cache_t(const std::shared_ptr<xrelay_chain_resources> & para);

public:
    void on_evm_db_event(data::xblock_t * block);
    bool get_tx_cache_leader(uint64_t & upper_height, std::map<uint64_t, xcross_txs_t> & cross_tx_map) const;
    bool get_tx_cache_backup(uint64_t upper_height, std::map<uint64_t, xcross_txs_t> & cross_tx_map) const;
    void recover_cache();
    void update_last_proc_evm_height(uint64_t last_proc_evm_height);

private:
    void process_block(data::xblock_t * block);

private:
    std::shared_ptr<xrelay_chain_resources> m_para;
    uint64_t m_last_proc_evm_height{0};
    uint64_t m_cached_evm_lower_height{0};
    uint64_t m_cached_evm_upper_height{0};
    uint32_t m_tx_num{0};
    std::map<uint64_t, xcross_txs_t> m_cross_tx_map;
};

class xrelay_elect_cache_t {
public:
    xrelay_elect_cache_t(const std::shared_ptr<xrelay_chain_resources> & para);

public:
    void on_elect_db_event(data::xblock_t * block);
    bool get_elect_cache(uint64_t elect_height, std::vector<data::xrelay_election_node_t> & reley_election);
    void recover_cache();
    void update_last_proc_elect_height(uint64_t last_proc_height);
    static bool get_relay_elections_by_height(const base::xvaccount_t & vaccount, uint64_t height, std::vector<data::xrelay_election_node_t> & relay_elections);

private:
    bool get_genesis_elect_cache(std::vector<data::xrelay_election_node_t> & reley_election);
    bool process_election_by_height(uint64_t height);

private:
    std::shared_ptr<xrelay_chain_resources> m_para;
    uint64_t m_last_proc_height{0};
    std::map<uint64_t, std::vector<data::xrelay_election_node_t>> m_elect_info_map;
    std::vector<data::xrelay_election_node_t> m_genesis_elect_info;
};

// todo(nathan):xvblock as base class like table block.
class xrelay_block_data_t {
public:
    xrelay_block_data_t(base::enum_xvblock_flag flag, uint64_t height, std::string mock_data) : m_flag(flag), m_height(height), m_mock_data(mock_data) {}
    xrelay_block_data_t(const xrelay_block_data_t & relay_block_data) {
        m_flag = relay_block_data.m_flag;
        m_height = relay_block_data.m_height;
        m_mock_data = relay_block_data.m_mock_data;
    }
    xrelay_block_data_t() {}
    base::enum_xvblock_flag get_flag() const {return m_flag;}
    uint64_t get_height() const {return m_height;}
    const std::string & get_mock_data() const {return m_mock_data;}
    const std::string to_string() {
        base::xstream_t stream(base::xcontext_t::instance());
        uint8_t flag = (uint8_t)m_flag;
        stream << flag;
        stream << m_height;
        stream << m_mock_data;
        return std::string((char *)stream.data(), stream.size());
    }
    void from_string(const std::string & str) {
        base::xstream_t stream{base::xcontext_t::instance(), (uint8_t*)str.data(), static_cast<uint32_t>(str.size())};
        uint8_t flag;
        stream >> flag;
        m_flag = (base::enum_xvblock_flag)flag;
        stream >> m_height;
        stream >> m_mock_data;
    }
private:
    base::enum_xvblock_flag m_flag;
    uint64_t m_height;
    std::string m_mock_data;
};


class xwrap_block_convertor {
public:
    static bool convert_to_relay_block(std::vector<data::xblock_ptr_t> wrap_blocks, std::shared_ptr<data::xrelay_block> & relay_block);
};

class xrelay_chain_mgr_t {
public:
    xrelay_chain_mgr_t(const observer_ptr<store::xstore_face_t> & store,
                       const observer_ptr<base::xvblockstore_t> & blockstore,
                       const observer_ptr<mbus::xmessage_bus_face_t> & bus);

public:
    void start(int32_t thread_id);
    void stop();
    bool get_tx_cache_leader(uint64_t lower_height, uint64_t & upper_height, std::map<uint64_t, xcross_txs_t> & cross_tx_map);
    bool get_tx_cache_backup(uint64_t lower_height, uint64_t upper_height, std::map<uint64_t, xcross_txs_t> & cross_tx_map);
    bool get_elect_cache(uint64_t elect_height, std::vector<data::xrelay_election_node_t> & reley_election);
    void on_timer();

private:
    void on_block_to_db_event(mbus::xevent_ptr_t e);
    void on_evm_db_event(data::xblock_ptr_t evm_block);
    void on_wrap_db_event(data::xblock_ptr_t wrap_block);
    void on_relay_elect_db_event(data::xblock_ptr_t relay_elect_block);

private:
    std::shared_ptr<xrelay_chain_resources> m_para;
    xcross_tx_cache_t m_cross_tx_cache;
    std::vector<data::xblock_ptr_t> m_wrap_blocks;
    // todo(nathan):pack elect data first.
    xrelay_elect_cache_t m_relay_elect_cache;
    uint64_t m_last_relay_elect_height{0};
    uint32_t m_bus_listen_id;
    int32_t m_thread_id;
    xrelay_chain_mgr_dispatcher_t * m_dispatcher;
    mutable std::mutex m_mutex;
    uint32_t m_on_timer_count{0};
};

class xrelay_chain_mgr_dispatcher_t : public base::xiobject_t {
public:
    xrelay_chain_mgr_dispatcher_t(base::xcontext_t & _context, int32_t thread_id, xrelay_chain_mgr_t * relay_chain_mgr)
      : base::xiobject_t(_context, thread_id, base::enum_xobject_type_woker), m_relay_chain_mgr(relay_chain_mgr) {
    }

    void dispatch(base::xcall_t & call);

    bool is_mailbox_over_limit();

protected:
    ~xrelay_chain_mgr_dispatcher_t() override {
    }

protected:
    observer_ptr<xrelay_chain_mgr_t> m_relay_chain_mgr;
};

NS_END2
