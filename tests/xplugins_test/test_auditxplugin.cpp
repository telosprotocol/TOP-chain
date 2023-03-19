#include "xcommon/xaccount_address_fwd.h"
#include "xplugin/xplugin_manager.h"
#include "xplugin_audit/xaudit_plugin.h"
#include "xplugin_audit/error.h"

#include <nlohmann/json.hpp>
#include <unistd.h>

#include <gtest/gtest.h>

#include <memory>

using namespace top;
using namespace std;
// the current fule should be placed in the repository of the transaction plugin in oreder to
// prevent leakage of implementation details.
// 1. is_audit().
// 2. when the transaction hash already exists .
// 3. when the transaction expired  .
// 4. when the tarnsaciton expiration time is zero.
//  hash already exist
TEST(test_auditxplugin, hash_exist) {
    // auto mgr = std::make_shared<data::xplugin_manager>();
    // mgr->start();
    // auto auditor = &data::xaudit_pligin::instance();

    // auto async_send = [=](data::xaudit_pligin & auditor, const std::shared_ptr<top::xtxpool_v2::xtx_entry> & tx, const std::shared_ptr<top::xtxpool_v2::xtxpool_table_t> & table) -> bool{
    //     auto tx_ptr = tx->get_tx();
    //     common::xaccount_address_t xaccount{tx_ptr->get_account_addr()};
    //     auto ret = auditor.is_audit(xaccount);
    //     if (make_error_code(enum_audit_result::NotFound).value() == ret) {
    //         return false;
    //     }
    //     if (make_error_code(enum_audit_result::NullPoint).value() == ret) {
    //         xwarn("xaudit_pligin::do_work audit-tx is_audit() params nullpoint");
    //         return true;
    //     }
    //     if (make_error_code(enum_audit_result::Ok).value() != ret) {
    //         xwarn("xaudit_pligin::do_work audit-tx is_audit() fail,%d", ret);
    //         return true;
    //     }
    //     auto tx_obj = tx_ptr->get_transaction();
    //     auto audit_data = auditor.to_audit_str(*tx_obj);
    //     auto tx_hash = tx_obj->get_digest_hex_str();
    //     xdbg("xaudit_pligin::is_audit audit-tx-account:%s ", xaccount.to_string().c_str());
    //     auto f = [table, tx, tx_hash]() {
    //         xdbg("xaudit_pligin::audit_data audit-tx-callback start,tx_hash:%s ", tx_hash.c_str());
    //         if (table == nullptr) {
    //             xwarn("xaudit_pligin::audit_data audit-tx-callback fail, table is null,tx_hash:%s ", tx_hash.c_str());
    //             return;
    //         }
    //         if (tx == nullptr) {
    //             xwarn("xaudit_pligin::audit_data audit-tx-callback fail, tx is null,tx_hash:%s ", tx_hash.c_str());
    //             return;
    //         }
    //         cout << "---------------GOOOD--------------" << endl;
    //         // if (table->push_send_tx(tx) != xsuccess) {
    //         //     xwarn("xaudit_pligin::audit_data audit-tx-callback fail,tx_hash:%s ", tx_hash.c_str());
    //         // } else {
    //         //     xinfo("xaudit_pligin::audit_data audit-tx-callback success,tx_hash:%s ", tx_hash.c_str());
    //         // }
    //     };
    //     return auditor.async_audit(tx_hash, audit_data, f);
    // };

    // std::string hash = "0x626b53ca686a4540272c55b8a2415453385ddd0fe806bbab8697781a64f01000";
    // std::string account = "T2000138CQwyzFxbWZ59mNjkq3eZ3eH41t7b5midm@0";
    // top::common::xaccount_address_t xaccount{account};
    // if (0 == auditor->is_audit(xaccount)) {
    //     auto f = [&]() { cout << "account:(" << xaccount.to_string() << ") callback success" << endl; };
    //     cout << "hash:" << hash << endl;
    //     nlohmann::json js;
    //     js["expire"] = "300";
    //     js["from"] = "T2000138CQwyzFxbWZ59mNjkq3eZ3eH41t7b5midm@0";
    //     js["gas"] = "0x0000000000000000000000000000000000000000000000000000000000000000";
    //     js["gasPrice"] = "0x30";
    //     js["hash"] = hash;
    //     js["input"] = "0x";
    //     js["maxFeePerGas"] = "0x0000000000000000000000000000000000000000000000000000000000000000";
    //     js["maxPriorityFeePerGas"] = "0x0000000000000000000000000000000000000000000000000000000000000000";
    //     js["nonce"] = "0x32";
    //     js["r"] = "0x0000000000000000000000000000000000000000000000000000000000000000";
    //     js["s"] = "0x0000000000000000000000000000000000000000000000000000000000000000";
    //     js["v"] = "0x0000000000000000000000000000000000000000000000000000000000000000";
    //     js["to"] = "T2000138CQwyzFxbWZ59mNjkq3eZ3eH41t7b5midm@0";
    //     js["type"] = "0x33";
    //     js["value"] = "0x30";
    //     cout << "jsondata:" << js.dump() << endl;
    //     xaudit_pligin::instance().async_audit(hash, js.dump(), f);
    // }
    // std::shared_ptr<top::xtxpool_v2::xtx_entry> tx;
    // std::shared_ptr<top::xtxpool_v2::xtxpool_table_t> table;
    // bool ret = async_send(*auditor,tx,table);
    // EXPECT_TRUE(ret==true);
    // sleep(200);
    cout << "over" << endl;
}

// // expire
// TEST(test_auditxplugin, expire) {
//     srand(time(NULL));
//     int randomNumber = rand() % 1000 + 1000;
//     // xaudit_pligin & auditor = xaudit_pligin::instance();
//     std::shared_ptr<xaudit_pligin> plugin(&xaudit_pligin::instance());
//     auto auditor = std::move(plugin);
//     auditor->load();
//     auditor->run();
//     std::string hash = "0x626b53ca686a4540272c55b8a2415453385ddd0fe806bbab8697781a64f0" + std::to_string(randomNumber);
//     std::string account = "T2000138CQwyzFxbWZ59mNjkq3eZ3eH41t7b5midm@0";
//     top::common::xaccount_address_t xaccount{account};

//     if (0 == xaudit_pligin::instance().is_audit(xaccount)) {
//         auto f = [&]() { cout << "account:(" << xaccount.to_string() << ") callback success" << endl; };
//         cout << "hash:" << hash << endl;
//         nlohmann::json js;
//         js["expire"] = "30";
//         js["from"] = "T2000138CQwyzFxbWZ59mNjkq3eZ3eH41t7b5midm@0";
//         js["gas"] = "0x0000000000000000000000000000000000000000000000000000000000000000";
//         js["gasPrice"] = "0x30";
//         js["hash"] = hash;
//         js["input"] = "0x";
//         js["maxFeePerGas"] = "0x0000000000000000000000000000000000000000000000000000000000000000";
//         js["maxPriorityFeePerGas"] = "0x0000000000000000000000000000000000000000000000000000000000000000";
//         js["nonce"] = "0x32";
//         js["r"] = "0x0000000000000000000000000000000000000000000000000000000000000000";
//         js["s"] = "0x0000000000000000000000000000000000000000000000000000000000000000";
//         js["v"] = "0x0000000000000000000000000000000000000000000000000000000000000000";
//         js["to"] = "T2000138CQwyzFxbWZ59mNjkq3eZ3eH41t7b5midm@0";
//         js["type"] = "0x33";
//         js["value"] = "0x30";
//         cout << "jsondata:" << js.dump() << endl;
//         xaudit_pligin::instance().async_audit(hash, js.dump(), f);
//     }

//     sleep(20);
//     cout << "over" << endl;
// }
