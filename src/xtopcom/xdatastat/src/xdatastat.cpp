// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdatastat/xdatastat.h"
#include "xmbus/xevent_store.h"
#include "xdata/xblock.h"
#include "xdata/xtableblock.h"
#include "xmetrics/xmetrics.h"
#include "xbase/xutl.h"

#include "xbase/xlog.h"

NS_BEG2(top, datastat)

xdatastat_t::xdatastat_t(const observer_ptr<mbus::xmessage_bus_face_t> &bus) {
    m_bus = bus;
    m_bus_listen_id = bus->add_listener(mbus::xevent_major_type_store, std::bind(&xdatastat_t::on_block_to_db_event, this, std::placeholders::_1));

    for (int i = 0; i < max_zone_index; i++) {
        m_tx_count[i] = 0;
        m_unit_count[i] = 0;
        m_tableblock_count[i] = 0;
        m_begin_tx_count[i] = 0;
        m_begin_unit_count[i] = 0;
        m_begin_tableblock_count[i] = 0;
        m_max_tx_tps[i] = 0;
        m_max_unit_tps[i] = 0;
        m_max_tableblock_tps[i] = 0;
    }
}

xdatastat_t::~xdatastat_t() {
    m_bus->remove_listener(mbus::xevent_major_type_store, m_bus_listen_id);
}

void xdatastat_t::try_calc_tps(uint64_t end_time) {
    if (m_begin_gmtime == 0) {  // init by block gmtime
        m_begin_gmtime = end_time;
    }

    // calc all tps metrics
    int pass_time = end_time - m_begin_gmtime;
    if (pass_time > tps_calc_interval_sencond) {  // calc tps per 5 mins
        std::lock_guard<std::mutex> l(m_lock);
        // double check again
        pass_time = end_time - m_begin_gmtime;
        if (pass_time > tps_calc_interval_sencond) {  // calc tps per 5 mins
            xassert(end_time > m_begin_gmtime);
            uint64_t m_tx_tps[max_zone_index];
            uint64_t m_unit_tps[max_zone_index];
            uint64_t m_tableblock_tps[max_zone_index];
            uint64_t tmp = 0;
            for (int i = 0; i < max_zone_index; i++) {
                tmp = m_tx_count[i];
                m_tx_tps[i] = (tmp - m_begin_tx_count[i]) / pass_time;
                if (m_max_tx_tps[i] < m_tx_tps[i]) {m_max_tx_tps[i] = m_tx_tps[i];}
                m_begin_tx_count[i] = tmp;

                tmp = m_unit_count[i];
                m_unit_tps[i] = (tmp - m_begin_unit_count[i]) / pass_time;
                if (m_max_unit_tps[i] < m_unit_tps[i]) {m_max_unit_tps[i] = m_unit_tps[i];}
                m_begin_unit_count[i] = tmp;

                tmp = m_tableblock_count[i];
                m_tableblock_tps[i] = (tmp - m_begin_tableblock_count[i]) / pass_time;
                if (m_max_tableblock_tps[i] < m_tableblock_tps[i]) {m_max_tableblock_tps[i] = m_tableblock_tps[i];}
                m_begin_tableblock_count[i] = tmp;
            }
            m_begin_gmtime = end_time;

            XMETRICS_PACKET_INFO("datastat_tx_tps", "tps_zone0", std::to_string(m_tx_tps[0]),
                                                    "max_zone0", std::to_string(m_max_tx_tps[0]),
                                                    "count_zone0", std::to_string(m_tx_count[0]),
                                                    "tps_zone1", std::to_string(m_tx_tps[1]),
                                                    "max_zone1", std::to_string(m_max_tx_tps[1]),
                                                    "count_zone1", std::to_string(m_tx_count[1]),
                                                    "tps_zone2", std::to_string(m_tx_tps[2]),
                                                    "max_zone2", std::to_string(m_max_tx_tps[2]),
                                                    "count_zone2", std::to_string(m_tx_count[2]));
            XMETRICS_PACKET_INFO("datastat_unit_tps", "tps_zone0", std::to_string(m_unit_tps[0]),
                                                    "max_zone0", std::to_string(m_max_unit_tps[0]),
                                                    "count_zone0", std::to_string(m_unit_count[0]),
                                                    "tps_zone1", std::to_string(m_unit_tps[1]),
                                                    "max_zone1", std::to_string(m_max_unit_tps[1]),
                                                    "count_zone1", std::to_string(m_unit_count[1]),
                                                    "tps_zone2", std::to_string(m_unit_tps[2]),
                                                    "max_zone2", std::to_string(m_max_unit_tps[2]),
                                                    "count_zone2", std::to_string(m_unit_count[2]));
            XMETRICS_PACKET_INFO("datastat_tableblock_tps", "tps_zone0", std::to_string(m_tableblock_tps[0]),
                                                    "max_zone0", std::to_string(m_max_tableblock_tps[0]),
                                                    "count_zone0", std::to_string(m_tableblock_count[0]),
                                                    "tps_zone1", std::to_string(m_tableblock_tps[1]),
                                                    "max_zone1", std::to_string(m_max_tableblock_tps[1]),
                                                    "count_zone1", std::to_string(m_tableblock_count[1]),
                                                    "tps_zone2", std::to_string(m_tableblock_tps[2]),
                                                    "max_zone2", std::to_string(m_max_tableblock_tps[2]),
                                                    "count_zone2", std::to_string(m_tableblock_count[2]));
        }
    }
}

void xdatastat_t::on_block_to_db_event(mbus::xevent_ptr_t e) {
    if (e->minor_type != mbus::xevent_store_t::type_block_to_db) {
        return;
    }

    mbus::xevent_store_block_to_db_ptr_t block_event = std::static_pointer_cast<mbus::xevent_store_block_to_db_t>(e);
    base::xvblock_t*                 block = block_event->block.get();
    xassert(block->check_block_flag(base::enum_xvblock_flag_committed));

    try_calc_tps(block->get_cert()->get_gmtime());

    const auto & address = block->get_account();
    base::enum_vaccount_addr_type addrtype = base::xvaccount_t::get_addrtype_from_account(address);
    if (addrtype == base::enum_vaccount_addr_type_block_contract) {
        do_tableblock_stat(block);

        if (block->get_header()->get_block_class() == base::enum_xvblock_class_light) {
            auto tableblock = dynamic_cast<data::xtable_block_t*>(block);
            xassert(tableblock != nullptr);

            const auto & units = tableblock->get_tableblock_units(true);
            xassert(!units.empty());
            for (auto & v : units) {
                do_unitblock_stat(v.get());
                do_tx_stat_with_unit(v.get());
            }
        }
    }
}

void xdatastat_t::do_tableblock_stat(base::xvblock_t* block) {
    const auto & address = block->get_account();
    base::xvaccount_t vaccount{address};
    std::string zone = std::to_string(vaccount.get_zone_index());

    // chain level stat
    if (block->get_header()->get_block_class() == base::enum_xvblock_class_nil) {
        XMETRICS_COUNTER_INCREMENT("datastat_tableblock_empty_num_chain", 1);
    } else {
        XMETRICS_COUNTER_INCREMENT("datastat_tableblock_nonempty_num_chain", 1);
    }
    // zone level stat
    XMETRICS_FLOW_COUNT("datastat_tableblock_flow_zone" + zone, 1);
    m_tableblock_count[vaccount.get_zone_index()]++;
    // table level stat
    XMETRICS_COUNTER_SET("datastat_tableblock_height_"+address, block->get_height());
}

void xdatastat_t::do_unitblock_stat(base::xvblock_t* block) {
    const auto & address = block->get_account();
    base::xvaccount_t vaccount{address};
    std::string zone = std::to_string(vaccount.get_zone_index());
    base::enum_vaccount_addr_type addrtype = base::xvaccount_t::get_addrtype_from_account(address);

    // chain level stat none
    // zone level stat
    XMETRICS_FLOW_COUNT("datastat_unit_flow_zone" + zone, 1);
    m_unit_count[vaccount.get_zone_index()]++;
    if (block->get_header()->get_block_class() == base::enum_xvblock_class_light) {
        XMETRICS_COUNTER_INCREMENT("datastat_unit_light_num_zone" + zone, 1);
    } else if (block->get_header()->get_block_class() == base::enum_xvblock_class_full) {
        XMETRICS_COUNTER_INCREMENT("datastat_unit_full_num_zone" + zone, 1);
    } else if (block->get_header()->get_block_class() == base::enum_xvblock_class_nil) {
        XMETRICS_COUNTER_INCREMENT("datastat_unit_empty_num_zone" + zone, 1);
    } else {
        xassert(false);
    }

    switch (addrtype) {
        case base::enum_vaccount_addr_type_secp256k1_user_account:
            XMETRICS_COUNTER_INCREMENT("datastat_unit_user_account_num_zone" + zone, 1);
            break;
        case base::enum_vaccount_addr_type_secp256k1_user_sub_account:
            XMETRICS_COUNTER_INCREMENT("datastat_unit_user_sub_account_num_zone" + zone, 1);
            break;
        case base::enum_vaccount_addr_type_native_contract:
            XMETRICS_COUNTER_INCREMENT("datastat_unit_native_contract_account_num_zone" + zone, 1);
            break;
        case base::enum_vaccount_addr_type_custom_contract:
            XMETRICS_COUNTER_INCREMENT("datastat_unit_custom_contract_account_num_zone" + zone, 1);
            break;
        default:
            xassert(0);
            break;
    }
}

void xdatastat_t::do_tx_stat_with_unit(base::xvblock_t* block) {
    const auto & address = block->get_account();
    base::xvaccount_t vaccount{address};
    std::string zone = std::to_string(vaccount.get_zone_index());
    base::enum_vaccount_addr_type addrtype = base::xvaccount_t::get_addrtype_from_account(address);
    data::xblock_t* block_ptr = dynamic_cast<data::xblock_t*>(block);
    const std::vector<data::xlightunit_tx_info_ptr_t> & txs = block_ptr->get_txs();

    // chan & zone level stat
    if (txs.size() != 0) {
        XMETRICS_FLOW_COUNT("datastat_tx_flow_zone" + zone, txs.size());

        uint64_t selftx_num = 0;
        uint64_t confirmtx_num = 0;
        uint64_t sendtx_num = 0;
        uint64_t recvtx_num = 0;
        for (auto & tx : txs) {
            if (tx->is_self_tx()) {
                selftx_num++;
            } else if (tx->is_send_tx()) {
                sendtx_num++;
            } else if (tx->is_recv_tx()) {
                recvtx_num++;
            } else if (tx->is_confirm_tx()) {
                confirmtx_num++;
            }
        }
        if (selftx_num != 0) {
            XMETRICS_COUNTER_INCREMENT("datastat_tx_self_num_zone" + zone, selftx_num);
        }
        if (sendtx_num != 0) {
            XMETRICS_COUNTER_INCREMENT("datastat_tx_send_num_zone" + zone, sendtx_num);
        }
        if (recvtx_num != 0) {
            XMETRICS_COUNTER_INCREMENT("datastat_tx_recv_num_zone" + zone, recvtx_num);
        }
        if (confirmtx_num != 0) {
            XMETRICS_COUNTER_INCREMENT("datastat_tx_confirm_num_zone" + zone, confirmtx_num);
        }
        if ((confirmtx_num + selftx_num) != 0) {
            m_tx_count[vaccount.get_zone_index()]++;
            XMETRICS_FLOW_COUNT("datastat_tx_self_confirm_flow_zone" + zone, confirmtx_num + selftx_num);
            XMETRICS_FLOW_COUNT("datastat_tx_self_confirm_flow_chain", confirmtx_num + selftx_num);
        }
    }

    // table level stat

    // address level stat just for native contract
    if (addrtype == base::enum_vaccount_addr_type_native_contract) {
        for (auto & tx : txs) {
            if (tx->is_self_tx()) {
                XMETRICS_COUNTER_INCREMENT("datastat_tx_native_contract_self_" + address, 1);
            } else if (tx->is_send_tx()) {
                XMETRICS_COUNTER_INCREMENT("datastat_tx_native_contract_send_" + address, 1);
            } else if (tx->is_recv_tx()) {
                XMETRICS_COUNTER_INCREMENT("datastat_tx_native_contract_recv_" + address, 1);
            } else if (tx->is_confirm_tx()) {
                XMETRICS_COUNTER_INCREMENT("datastat_tx_native_contract_confirm_" + address, 1);
            }
        }
    }
}

NS_END2
