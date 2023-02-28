#include "xcommon/xaccount_address_fwd.h"
#include "xplugin/xplugin_manager.h"
#include "xplugin_audit/xaudit_plugin.h"

#include <nlohmann/json.hpp>
#include <unistd.h>

#include <gtest/gtest.h>

#include <memory>

using namespace top::data;
using namespace std;

// test  xaudit_pligin::instance()
TEST(test_auditxplugin, instance) {
    srand(time(NULL));
    int randomNumber = rand() % 1000 + 1000;
    // xaudit_pligin & auditor = xaudit_pligin::instance();
    std::shared_ptr<xaudit_pligin> plugin(&xaudit_pligin::instance());
    auto auditor = std::move(plugin);
    auditor->load();
    auditor->run();
    std::string hash = "0x626b53ca686a4540272c55b8a2415453385ddd0fe806bbab8697781a64f0" + std::to_string(randomNumber);
    std::string account = "T2000138CQwyzFxbWZ59mNjkq3eZ3eH41t7b5midm@0";
    top::common::xaccount_address_t xaccount{account};
    auto count = 0;
    if (0 == xaudit_pligin::instance().is_audit(xaccount)) {
        auto f = [&]() { count += 100; };
        cout << "hash:" << hash << endl;
        nlohmann::json js;
        js["expire"] = "300";
        js["from"] = "T2000138CQwyzFxbWZ59mNjkq3eZ3eH41t7b5midm@0";
        js["gas"] = "0x0000000000000000000000000000000000000000000000000000000000000000";
        js["gasPrice"] = "0x30";
        js["hash"] = hash;
        js["input"] = "0x";
        js["maxFeePerGas"] = "0x0000000000000000000000000000000000000000000000000000000000000000";
        js["maxPriorityFeePerGas"] = "0x0000000000000000000000000000000000000000000000000000000000000000";
        js["nonce"] = "0x32";
        js["r"] = "0x0000000000000000000000000000000000000000000000000000000000000000";
        js["s"] = "0x0000000000000000000000000000000000000000000000000000000000000000";
        js["v"] = "0x0000000000000000000000000000000000000000000000000000000000000000";
        js["to"] = "T2000138CQwyzFxbWZ59mNjkq3eZ3eH41t7b5midm@0";
        js["type"] = "0x33";
        js["value"] = "0x30";
        cout << "jsondata:" << js.dump() << endl;
        xaudit_pligin::instance().async_audit(hash, js.dump(), f);
    }

    sleep(20);
    cout << "over" << endl;
}

// //  hash already exist
// TEST(test_auditxplugin, hash_exist) {
//     std::shared_ptr<xaudit_pligin> plugin(&xaudit_pligin::instance());
//     auto auditor = std::move(plugin);
//     auditor->load();
//     auditor->run();
//     std::string hash = "0x626b53ca686a4540272c55b8a2415453385ddd0fe806bbab8697781a64f01000";
//     std::string account = "T2000138CQwyzFxbWZ59mNjkq3eZ3eH41t7b5midm@0";
//     top::common::xaccount_address_t xaccount{account};

//     if (0 == auditor->is_audit(xaccount)) {
//         auto f = [&]() { cout << "account:(" << xaccount.to_string() << ") callback success" << endl; };
//         cout << "hash:" << hash << endl;
//         nlohmann::json js;
//         js["expire"] = "300";
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

