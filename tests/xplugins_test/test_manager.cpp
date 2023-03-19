#include "xcommon/xaccount_address_fwd.h"
#include "xplugin/xplugin.h"
#include "xplugin/xplugin_manager.h"

#include <nlohmann/json.hpp>
#include <unistd.h>

#include <gtest/gtest.h>

#include <memory>

using namespace top::data;
using namespace std;

TEST(test_plugin_manager, base) {
    auto mgr = std::make_shared<xplugin_manager>();

    mgr->start();
    EXPECT_TRUE(mgr->get(AUDITX_PLUGIN) != nullptr);
    
    mgr->remove(AUDITX_PLUGIN);
    EXPECT_TRUE(mgr->get(AUDITX_PLUGIN) == nullptr);

    auto plugin = std::make_shared<xplugin>();
    mgr->add(AUDITX_PLUGIN, plugin);
    EXPECT_TRUE(mgr->get(AUDITX_PLUGIN) != nullptr);

    mgr->remove(AUDITX_PLUGIN);
    EXPECT_TRUE(mgr->get(AUDITX_PLUGIN) == nullptr);
}
