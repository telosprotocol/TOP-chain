// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_service_v2/xtxpool_service.h"

#include "xvledger/xvblock.h"
#include "xcommon/xmessage_id.h"
#include "xdata/xblocktool.h"
#include "xdata/xtableblock.h"
#include "xmetrics/xmetrics.h"
#include "xtxpool_v2/xtxpool.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xtxpool_v2/xtxpool_log.h"
#include "xverifier/xtx_verifier.h"
#include "xvnetwork/xvnetwork_error.h"

#include <cinttypes>

NS_BEG2(top, xtxpool_service_v2)

using xtxpool_v2::xtxpool_t;

enum enum_txpool_service_msg_type  // under pdu  enum_xpdu_type_consensus
{
    enum_txpool_service_msg_type_void = 0,  // reserved for old version

    enum_txpool_service_msg_type_send_receipt = 1,
    enum_txpool_service_msg_type_recv_receipt = 2,
};

class txpool_receipt_message_para_t : public top::base::xobject_t {
public:
    txpool_receipt_message_para_t(vnetwork::xvnode_address_t const & sender, vnetwork::xmessage_t const & message) : m_sender(sender), m_message(message) {
    }

private:
    ~txpool_receipt_message_para_t() override {
    }

public:
    vnetwork::xvnode_address_t m_sender;
    vnetwork::xmessage_t m_message;
};

xtxpool_service::xtxpool_service(const observer_ptr<router::xrouter_face_t> & router, const observer_ptr<xtxpool_svc_para_t> & para) : m_router(router), m_para(para) {
}

void xtxpool_service::set_params(const xvip2_t & xip, const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> & vnet_driver) {
    if (xcons_utl::xip_equals(m_xip, xip)) {
        return;
    }
    m_xip = xip;
    m_vnet_driver = vnet_driver;
    m_vnetwork_str = vnet_driver->address().to_string();

    common::xnode_address_t node_addr = xcons_utl::to_address(m_xip, m_vnet_driver->address().version());

    m_node_id = static_cast<std::uint16_t>(get_node_id_from_xip2(m_xip));
    m_shard_size = static_cast<std::uint16_t>(get_group_nodes_count_from_xip2(m_xip));
    xassert(m_node_id < m_shard_size);

    auto type = node_addr.type();
    if (common::has<common::xnode_type_t::committee>(type)) {
        m_is_send_receipt_role = true;
        m_zone_index = base::enum_chain_zone_beacon_index;
    } else if (common::has<common::xnode_type_t::zec>(type)) {
        m_is_send_receipt_role = true;
        m_zone_index = base::enum_chain_zone_zec_index;
    } else if (common::has<common::xnode_type_t::auditor>(type)) {
        m_is_send_receipt_role = true;
        m_zone_index = base::enum_chain_zone_consensus_index;
    } else if (common::has<common::xnode_type_t::validator>(type)) {
        m_is_send_receipt_role = false;
        m_zone_index = base::enum_chain_zone_consensus_index;
    } else {
        xassert(0);
    }

    std::vector<uint16_t> tables = m_vnet_driver->table_ids();
    if (tables.empty()) {
        xerror("xtxpool_service::set_params xip low addr:%" PRIx64 "node:%s, load table failed", xip.low_addr, m_vnetwork_str.c_str());
        return;
    }
    m_cover_front_table_id = (uint32_t)tables.front();
    m_cover_back_table_id = (uint32_t)tables.back();
}

bool xtxpool_service::start(const xvip2_t & xip) {
    m_vnet_driver->register_message_ready_notify(xmessage_category_txpool, std::bind(&xtxpool_service::on_message_receipt, this, std::placeholders::_1, std::placeholders::_2));

    xinfo("xtxpool_service::start node:%s, cluster_address:%s, zone:%d table:%d %d",
          m_vnetwork_str.c_str(),
          m_vnet_driver->address().cluster_address().to_string().c_str(),
          m_zone_index,
          m_cover_front_table_id,
          m_cover_back_table_id);
    m_running = true;
    return m_running;
}
bool xtxpool_service::fade(const xvip2_t & xip) {
    xinfo("xtxpool_service::fade node:%s, cluster_address:%s, zone:%d table:%d %d",
          m_vnetwork_str.c_str(),
          m_vnet_driver->address().cluster_address().to_string().c_str(),
          m_zone_index,
          m_cover_front_table_id,
          m_cover_back_table_id);
    xassert(m_running);
    m_vnet_driver->unregister_message_ready_notify(xmessage_category_txpool);
    m_running = false;

    return !m_running;
}

bool xtxpool_service::is_running() {
    return m_running;
}

void xtxpool_service::get_service_table_boundary(base::enum_xchain_zone_index & zone_id, uint32_t & fount_table_id, uint32_t & back_table_id) {
    zone_id = m_zone_index;
    fount_table_id = m_cover_front_table_id;
    back_table_id = m_cover_back_table_id;
}

#define receipt_resend_interval (0x3F)  // every 64 seconds resend once
#define shifting_for_receipt_resend_interval (6)

bool xtxpool_service::is_resend_node_for_talbe(uint64_t now, uint32_t table_id, uint16_t shard_size, uint16_t self_node_id) {
    // different table resend at different time by different advance node
    uint64_t random_num = now + (uint64_t)table_id;
    bool is_time_for_resend = ((random_num & receipt_resend_interval) == 0);
    uint16_t resend_node_pos = ((now >> shifting_for_receipt_resend_interval) + (uint64_t)table_id) % shard_size;
    xinfo("random_num:0x%x,receipt_resend_interval:0x%x,random_num|receipt_resend_interval:0x%x", random_num, receipt_resend_interval, random_num | receipt_resend_interval);
    xinfo("is_resend_node_for_talbe, table:%d,now:%llu,is_time_for_resend:%d,resend_node_pos:%d,self_node_id:%d", table_id, now, is_time_for_resend, resend_node_pos, self_node_id);
    return (is_time_for_resend && resend_node_pos == self_node_id);
}

void xtxpool_service::resend_receipts(uint64_t now) {
    if (m_running && m_is_send_receipt_role) {
        for (uint32_t table_id = m_cover_front_table_id; table_id <= m_cover_back_table_id; table_id++) {
            if (!is_resend_node_for_talbe(now, table_id, m_shard_size, m_node_id)) {
                continue;
            }

            std::vector<xcons_transaction_ptr_t> recv_txs = m_para->get_txpool()->get_resend_txs(m_zone_index, table_id, now);
            for (auto recv_tx : recv_txs) {
                xassert(recv_tx->is_recv_tx());
                // filter out txs witch has already in txpool, just not consensused and committed.
                auto tx = m_para->get_txpool()->query_tx(recv_tx->get_source_addr(), recv_tx->get_transaction()->digest());
                if (tx != nullptr && tx->get_tx()->is_confirm_tx()) {
                    continue;
                }
                send_receipt(recv_tx, false);
            }
        }
    }
}

bool xtxpool_service::is_belong_to_service(xtable_id_t tableid) {
    if (tableid.get_zone_index() == m_zone_index && (tableid.get_subaddr() >= m_cover_front_table_id && tableid.get_subaddr() <= m_cover_back_table_id)) {
        return true;
    }
    return false;
}

bool xtxpool_service::table_boundary_equal_to(std::shared_ptr<xtxpool_service_face> & service) {
    base::enum_xchain_zone_index zone_id;
    uint32_t fount_table_id;
    uint32_t back_table_id;
    service->get_service_table_boundary(zone_id, fount_table_id, back_table_id);
    if (zone_id == this->m_zone_index && fount_table_id == m_cover_front_table_id && back_table_id == m_cover_back_table_id) {
        return true;
    }
    return false;
}

void xtxpool_service::on_message_receipt(vnetwork::xvnode_address_t const & sender, vnetwork::xmessage_t const & message) {
    if (!m_running || m_para->get_dispatcher() == nullptr) {
        return;
    }
    (void)sender;
    if (message.id() == xtxpool_msg_send_receipt || message.id() == xtxpool_msg_recv_receipt) {
        auto handler = [this](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
            txpool_receipt_message_para_t * para = dynamic_cast<txpool_receipt_message_para_t *>(call.get_param1().get_object());
            this->on_message_unit_receipt(para->m_sender, para->m_message);
            return true;
        };

        if (m_para->get_dispatcher()->is_mailbox_over_limit()) {
            xwarn("xtxpool_service::on_message_receipt txpool mailbox limit,drop receipt");
            return;
        }
        base::xauto_ptr<txpool_receipt_message_para_t> para = new txpool_receipt_message_para_t(sender, message);
        base::xcall_t asyn_call(handler, para.get());
        m_para->get_dispatcher()->dispatch(asyn_call);
    } else {
        xassert(0);
    }
}

void xtxpool_service::on_message_unit_receipt(vnetwork::xvnode_address_t const & sender, vnetwork::xmessage_t const & message) {
    (void)sender;
    base::xstream_t stream(top::base::xcontext_t::instance(), (uint8_t *)message.payload().data(), (uint32_t)message.payload().size());
    data::xcons_transaction_ptr_t receipt = make_object_ptr<data::xcons_transaction_t>();
    int32_t ret;
    if (message.id() == xtxpool_msg_recv_receipt) {
        xobject_ptr_t<xtxpool_confirm_receipt_msg_t> confirm_receipt_msg = make_object_ptr<xtxpool_confirm_receipt_msg_t>();
        ret = confirm_receipt_msg->serialize_from(stream);
        if (ret <= 0) {
            xerror("xtxpool_service::on_message_unit_receipt confirm receipt serialize_from fail ret:%d", ret);
            return;
        }
        auto & account_addr = confirm_receipt_msg->get_source_addr();
        auto hash = confirm_receipt_msg->get_receipt()->get_tx_info()->get_tx_hash_256();
        auto tx = m_para->get_txpool()->get_unconfirm_tx(account_addr, hash);
        if (tx == nullptr) {
            xwarn("xtxpool_service::on_message_unit_receipt unconfirm tx not found:source addr:%s, hash:%s",
                  account_addr.c_str(),
                  confirm_receipt_msg->get_receipt()->get_tx_info()->get_tx_hex_hash().c_str());
            return;
        }
        receipt = make_object_ptr<data::xcons_transaction_t>(tx->get_transaction(), confirm_receipt_msg->get_receipt());
    } else {
        ret = receipt->serialize_from(stream);
        if (ret <= 0) {
            xerror("xtxpool_service::on_message_unit_receipt receipt serialize_from fail ret:%d", ret);
            return;
        }
    }

    xinfo("xtxpool_service::on_message_unit_receipt receipt=%s,from_vnode:%s,at_node:%s", receipt->dump().c_str(), sender.to_string().c_str(), m_vnetwork_str.c_str());

    xtxpool_v2::xtx_para_t para;
    std::shared_ptr<xtxpool_v2::xtx_entry> tx_ent = std::make_shared<xtxpool_v2::xtx_entry>(receipt, para);
    ret = m_para->get_txpool()->push_receipt(tx_ent);
    // push success means may be not consensused. duplicate means already committed, and need response recv receipt
    if (ret == xtxpool_v2::xtxpool_error_tx_duplicate) {
        check_and_response_recv_receipt(receipt);
    }
    if (ret == 0) {
        auditor_forward_receipt_to_shard(receipt, message);
    }
}

void xtxpool_service::check_and_response_recv_receipt(const xcons_transaction_ptr_t & cons_tx) {
    if (!cons_tx->is_recv_tx()) {
        return;
    }
    if (!m_is_send_receipt_role) {
        return;
    }
    xdbg("xtxpool_service::check_and_response_recv_receipt receipt=%s at_node:%ld", cons_tx->dump().c_str(), m_xip.low_addr);
    const xlightunit_output_entity_t * info = cons_tx->get_tx_info();

    if (!info->is_send_tx()) {
        xassert(0);
    }

    xtransaction_t * tx = cons_tx->get_transaction();
    base::xvtransaction_store_ptr_t tx_store = m_para->get_vblockstore()->query_tx(tx->get_digest_str(), base::enum_transaction_subtype_recv);
    // first time consensus transaction has been stored, so it can be found
    // in the second consensus, need check the m_recv_unit_height

    // build recv receipt and send out
    if (tx_store != nullptr) {
        xassert(tx_store->get_recv_unit_height() != 0);
        xdbg("xtxpool_service::check_and_response_recv_receipt send tx receipt has been consensused, txhash:%s", tx->get_digest_hex_str().c_str());
        base::xauto_ptr<base::xvblock_t> blockobj = m_para->get_vblockstore()->load_block_object(base::xvaccount_t(tx->get_target_addr()), tx_store->get_recv_unit_height(), base::enum_xvblock_flag_committed, true);
        if (blockobj != nullptr) {
            xblock_t * block = dynamic_cast<xblock_t *>(blockobj.get());
            xassert(block->is_lightunit());
            xlightunit_block_t * lightunit = dynamic_cast<xlightunit_block_t *>(block);
            auto recv_tx_receipt = lightunit->create_one_txreceipt(tx);
            xassert(recv_tx_receipt->is_confirm_tx());

            send_receipt(recv_tx_receipt, true);
        } else {
            xerror("xtxpool_service::check_and_response_recv_receipt recv tx unit not exist txhash:%s block_height:%ld",
                   tx->get_digest_hex_str().c_str(),
                   tx_store->get_recv_unit_height());
        }
    } else {
        // TODO(jimmy) sync invoke
        xwarn("xtxpool_service::check_and_response_recv_receipt recv tx not found txhash:%s",
                tx->get_digest_hex_str().c_str());
    }
}

bool xtxpool_service::is_receipt_sender(const xtable_id_t & tableid, const uint256_t & hash) {
    return m_running && m_is_send_receipt_role && is_belong_to_service(tableid);
}

bool xtxpool_service::set_commit_prove(data::xcons_transaction_ptr_t & cons_tx) {
    if (!cons_tx->is_commit_prove_cert_set()) {
        std::string account_addr = (cons_tx->is_recv_tx()) ? cons_tx->get_source_addr() : cons_tx->get_target_addr();
        std::string table_account = account_address_to_block_address(common::xaccount_address_t(account_addr));
        uint64_t justify_table_height = cons_tx->get_unit_cert()->get_parent_block_height() + 2;
        // try load table block first.
        base::xvaccount_t table_vaccount(table_account);
        base::xauto_ptr<base::xvblock_t> justify_table_block = m_para->get_vblockstore()->load_block_object(table_vaccount, justify_table_height, base::enum_xvblock_flag_authenticated, false);
        if (justify_table_block != nullptr) {
            cons_tx->set_commit_prove_with_parent_cert(justify_table_block->get_cert());
        } else {
            uint64_t justify_unit_height = cons_tx->get_unit_height() + 2;
            base::xvaccount_t unit_vaccount(account_addr);
            base::xauto_ptr<base::xvblock_t> justify_unit_block = m_para->get_vblockstore()->load_block_object(unit_vaccount, justify_unit_height, base::enum_xvblock_flag_authenticated, false);
            if (justify_unit_block == nullptr) {
                xwarn("xtxpool_service::set_commit_prove can not load justify tableblock and unit block .tx=%s,account=%s,table height=%ld,unit height=%ld",
                      cons_tx->dump().c_str(),
                      table_account.c_str(),
                      justify_table_height,
                      justify_unit_height);
                return false;
            }
            cons_tx->set_commit_prove_with_self_cert(justify_unit_block->get_cert());
        }
        xassert(cons_tx->get_receipt()->is_valid());
    } else {
        xdbg("xtxpool_service::set_commit_prove receipt already set commit prove. tx=%s", cons_tx->dump().c_str());
    }
    return true;
}

void xtxpool_service::send_receipt(data::xcons_transaction_ptr_t & cons_tx, bool first_send) {
    if (!m_running) {
        return;
    }
    if (first_send && !has_receipt_right(cons_tx->get_transaction()->digest())) {
        return;
    }
    if (!set_commit_prove(cons_tx)) {
        return;
    }
    send_receipt_real(cons_tx);
    if (first_send) {
        XMETRICS_COUNTER_INCREMENT("txpool_receipt_first_send", 1);
    } else {
        XMETRICS_COUNTER_INCREMENT("txpool_receipt_retry_send", 1);
    }
}

void xtxpool_service::send_receipt_real(const data::xcons_transaction_ptr_t & cons_tx) {
    try {
        std::string target_addr = cons_tx->get_receipt_target_account();

        top::base::xautostream_t<4096> stream(top::base::xcontext_t::instance());
        vnetwork::xmessage_t msg;
        if (cons_tx->is_recv_tx()) {
            cons_tx->serialize_to(stream);
            msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xtxpool_msg_send_receipt);
        } else {
            xtxpool_confirm_receipt_msg_t confirm_receipt_msg(cons_tx->get_source_addr(), cons_tx->get_receipt());
            confirm_receipt_msg.serialize_to(stream);
            msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xtxpool_msg_recv_receipt);
        }
        // TODO(jimmy)  first get target account's table id, then get network addr by table id
        auto receiver_cluster_addr =
            m_router->sharding_address_from_account(common::xaccount_address_t{target_addr}, m_vnet_driver->network_id(), common::xnode_type_t::consensus_auditor);
        assert(common::has<common::xnode_type_t::consensus_auditor>(receiver_cluster_addr.type()) || common::has<common::xnode_type_t::committee>(receiver_cluster_addr.type()) ||
               common::has<common::xnode_type_t::zec>(receiver_cluster_addr.type()));

        // for (auto & v : receipt->m_receipt_txs) {
        //     for (auto & one_tx : v.m_txs_info) {
        //         xkinfo("[global_trace][xtxpool_service][send receipt to network] retry:%d msg_size:%d owner:%s height:%ld tx_info:%s %s -> %s",
        //             receipt->m_retry_count, stream.size(), v.m_unit_header.m_block_owner.c_str(), v.m_unit_header.m_prev_height + 1,
        //             one_tx.m_tx_info.to_detail_string().c_str(), m_vnetwork_str.c_str(), receiver_cluster_addr.to_string().c_str());
        //     }
        // }

        xassert(!common::has<common::xnode_type_t::consensus_validator>(m_vnet_driver->type()));
        xassert(m_is_send_receipt_role);
        if (m_vnet_driver->address().cluster_address() == receiver_cluster_addr) {
            xinfo("xtxpool_service::send_receipt_real broadcast receipt=%s,vnode:%ld", cons_tx->dump().c_str(), m_vnetwork_str.c_str());
            m_vnet_driver->broadcast(msg);
            on_message_unit_receipt(m_vnet_driver->address(), msg);
        } else {
            xinfo("xtxpool_service::send_receipt_real forward receipt=%s,from_vnode:%s,to_vnode:%s",
                  cons_tx->dump().c_str(),
                  m_vnetwork_str.c_str(),
                  receiver_cluster_addr.to_string().c_str());
            m_vnet_driver->forward_broadcast_message(msg, vnetwork::xvnode_address_t{std::move(receiver_cluster_addr)});
        }
    } catch (vnetwork::xvnetwork_error_t const & eh) {
        xwarn("xtxpool_service::send_receipt_real xvnetwork_error_t exception caught: %s; error code: %d", eh.what(), eh.code().value());
    } catch (const std::exception & eh) {
        xwarn("xtxpool_service::send_receipt_real std exception caught: %s;", eh.what());
    }
}

void xtxpool_service::auditor_forward_receipt_to_shard(const xcons_transaction_ptr_t & cons_tx, vnetwork::xmessage_t const & message) {
    common::xnode_address_t node_addr = xcons_utl::to_address(m_xip, m_vnet_driver->address().version());
    if (!common::has<common::xnode_type_t::auditor>(node_addr.type())) {
        return;
    }

    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    bool has_right = has_receipt_right(cons_tx->get_transaction()->digest());
    if (has_right) {
        const std::string & target_address = cons_tx->get_receipt_target_account();

        auto cluster_addr =
            m_router->sharding_address_from_account(common::xaccount_address_t{target_address}, m_vnet_driver->network_id(), common::xnode_type_t::consensus_validator);
        xassert(common::has<common::xnode_type_t::consensus_validator>(cluster_addr.type()));

        vnetwork::xvnode_address_t vaddr{std::move(cluster_addr)};
        forward_broadcast_message(vaddr, message);
    }
}

bool xtxpool_service::has_receipt_right(const uint256_t & hash) const {
    // select 2 auditor to send the receipt, select 1 for resend.
    uint32_t select_num = 1;
    // use tx hash to generate random number
    std::string hash_str{reinterpret_cast<char *>(hash.data()), static_cast<size_t>(hash.size())};
    // calculate a random position that means which node is selected to send the receipt
    // the random position change by resend_time for rotate the selected node, to avoid same node is selected continuously.
    uint32_t rand_pos = (base::xhash32_t::digest(hash_str)) % m_shard_size;
    bool ret = is_selected_sender(m_node_id, rand_pos, select_num, m_shard_size);
    xinfo("xtxpool_service::has_receipt_right ret:%d hash:%s rand_pos:%u select_num:%u node_id:%u shard_size:%u",
          ret,
          to_hex_str(hash).c_str(),
          rand_pos,
          select_num,
          m_node_id,
          m_shard_size);
    return ret;
}

void xtxpool_service::forward_broadcast_message(const vnetwork::xvnode_address_t & addr, const vnetwork::xmessage_t & message) {
    try {
        m_vnet_driver->forward_broadcast_message(message, addr);
    } catch (vnetwork::xvnetwork_error_t const & eh) {
        xwarn("xtxpool_service::forward_broadcast_message xvnetwork_error_t exception caught: %s; error code: %d", eh.what(), eh.code().value());
    } catch (const std::exception & eh) {
        xwarn("xtxpool_service::forward_broadcast_message std exception caught: %s;", eh.what());
    }
}

int32_t xtxpool_service::request_transaction_consensus(const data::xtransaction_ptr_t & tx, bool local) {
    xdbg("xtxpool_service::request_transaction_consensus in, tx:source:%s target:%s hash:%s",
         tx->get_source_addr().c_str(),
         tx->get_target_addr().c_str(),
         tx->get_digest_hex_str().c_str());
    if (!m_running) {
        xwarn(
            "[xtxpool_service]not running, tx dropped:source:%s target:%s hash:%s", tx->get_source_addr().c_str(), tx->get_target_addr().c_str(), tx->get_digest_hex_str().c_str());
        return false;
    }

    int32_t ret = xverifier::xtx_verifier::verify_send_tx_source(tx.get(), local);
    if (ret) {
        xwarn("[global_trace][xtxpool_service]tx=%s,account=%s verify send tx source fail", tx->get_digest_hex_str().c_str(), tx->get_source_addr().c_str());
        return ret;
    }

    auto tableid = data::account_map_to_table_id(common::xaccount_address_t{tx->get_source_addr()});
    if (!is_belong_to_service(tableid)) {
        xerror("[global_trace][xtxpool_service]%s %s zone%d table%d not match this network driver",
               tx->get_digest_hex_str().c_str(),
               tx->get_source_addr().c_str(),
               tableid.get_zone_index(),
               tableid.get_subaddr());
        return xtxpool_v2::xtxpool_error_transaction_not_belong_to_this_service;
    }

    if (is_sys_sharding_contract_address(common::xaccount_address_t{tx->get_target_addr()})) {
        tx->adjust_target_address(tableid.get_subaddr());
    }

    xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(tx.get());
    xtxpool_v2::xtx_para_t para;
    std::shared_ptr<xtxpool_v2::xtx_entry> tx_ent = std::make_shared<xtxpool_v2::xtx_entry>(cons_tx, para);
    return m_para->get_txpool()->push_send_tx(tx_ent);
}

bool xtxpool_service::is_selected_sender(uint32_t pos, uint32_t rand_pos, uint32_t select_num, uint32_t size) {
    bool ret = false;
    xassert((select_num < size) && (pos < size));
    if (pos >= rand_pos) {
        return pos < rand_pos + select_num;
    } else {
        return (rand_pos + select_num > size) && (pos < (rand_pos + select_num) % size);
    }
}

int32_t xtxpool_confirm_receipt_msg_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    stream << m_source_addr;
    DEFAULT_SERIALIZE_PTR(m_receipt);
    return CALC_LEN();
}

int32_t xtxpool_confirm_receipt_msg_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    stream >> m_source_addr;
    DEFAULT_DESERIALIZE_PTR(m_receipt, xtx_receipt_t);
    return CALC_LEN();
}

NS_END2
