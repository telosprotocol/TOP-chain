#include "gtest/gtest.h"

#include <stdio.h>
#include <string>
#include <fstream>
#include "xconfig/xconfig_register.h"
#include "xverifier/xwhitelist_verifier.h"
#include "xverifier/xverifier_utl.h"
#include "json/json.h"

using namespace top::xverifier;
using namespace top::config;


class xwhitelist_utl_test: public  top::xverifier::xwhitelist_utl {
public:

    // wl test function
    static bool add_to_whitelist(std::string const & addr) {
        wl.emplace_back(addr);
        return true;
    }

    static bool delelte_from_whitelist(std::string const & addr) {
        auto iter = std::find_if(wl.begin(), wl.end(), [&addr](std::string const& w) { return w == addr; });
        if (iter != wl.end()) {
            xwarn("[global_trace][xwhitelist_utl][delelte_from_whitelist][fail], cannot found!");
            return false;
        }

        wl.erase(iter);
        return true;
    }

    static void set_whitelist_to_config() {
        // base::xstream_t stream(base::xcontext_t::instance());
        // VECTOR_OBJECT_SERIALIZE(stream, wl);
        // auto res = std::string(reinterpret_cast<char*>(stream.data()), stream.size());

        auto res = generate_wl_string();

        if (!top::config::xconfig_register_t::get_instance().set(top::config::xtop_whitelist_onchain_goverance_parameter::name, res)) {
                assert(0);
        }
    }

    static std::string generate_wl_string() {
        if (xwhitelist_utl::wl.empty()) return "";

        std::string res = wl[0];
        for (size_t i = 1; i < wl.size(); ++i) {
            res = res + "," + wl[i];
        }
        return res;
    }

    static void print_whitelist() {
        xinfo("whitelist content begin:");
        xinfo("value: %s", generate_wl_string().c_str());

        xinfo("whitelist content end!");
    }

    static bool load_bwlist_content(std::string const& config_file, std::map<std::string, std::string>& result) {
        if (config_file.empty()) {
            xwarn("load local black/white list error!");
            return false;
        }

        std::ifstream in(config_file);
        if (!in.is_open()) {
            xwarn("open local black/white list file %s error", config_file.c_str());
            return false;
        }
        std::ostringstream oss;
        oss << in.rdbuf();
        in.close();
        auto json_content = std::move(oss.str());
        if (json_content.empty()) {
            xwarn("black/white list config file %s empty", config_file.c_str());
            return false;
        }


        xJson::Value  json_root;
        xJson::Reader  reader;
        bool ret = reader.parse(json_content, json_root);
        if (!ret) {
            xwarn("parse config file %s failed", config_file.c_str());
            return false;
        }


        auto const members = json_root.getMemberNames();
        for(auto const& member: members) {
            if (json_root[member].isArray()) {
                for (unsigned int i = 0; i < json_root[member].size(); ++i) {
                    try {
                        if ( top::xverifier::xverifier_success != top::xverifier::xtx_utl::address_is_valid(json_root[member][i].asString())) return false;
                    } catch (...) {
                        xwarn("parse config file %s failed", config_file.c_str());
                        return false;
                    }
                    result[member] += json_root[member][i].asString() + ",";
                }
            } else {
                xwarn("parse config file %s failed", config_file.c_str());
                return false;
            }


        }

        return true;
    }
};

class test_whitelist : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(test_whitelist, whitelist) {
    printf("## test whitelist begin --------------\n");
    // init
    xinfo("init:\n");
    xwhitelist_utl_test::print_whitelist();
    ASSERT_TRUE(top::config::xconfig_register_t::get_instance().set(top::config::xtop_whitelist_onchain_goverance_parameter::name, std::string{""}));
    auto res = XGET_ONCHAIN_GOVERNANCE_PARAMETER(whitelist);
    ASSERT_TRUE(res.empty());

    xinfo("add:\n");
    std::string test_addr = "T000001FiHkC1L9z6VftMEB3uke7MUJDy9ZcePhw";
    for (auto i = 0; i < 2; ++i) {
        auto tmp_addr = test_addr + std::to_string(i);

        xwhitelist_utl_test::add_to_whitelist(tmp_addr);
        xwhitelist_utl_test::print_whitelist();
        auto wl_str = xwhitelist_utl_test::generate_wl_string();

        xwhitelist_utl_test::set_whitelist_to_config();
        res = XGET_ONCHAIN_GOVERNANCE_PARAMETER(whitelist);
        xinfo("onchain config value: %zu %s", res.size(), res.c_str());
        auto wl_config = xwhitelist_utl::get_whitelist_from_config();
        xwhitelist_utl_test::print_whitelist();
        auto wl_config_str = xwhitelist_utl_test::generate_wl_string();

        ASSERT_EQ(wl_str, wl_config_str);
    }

    printf("## test whitelist end --------------\n");

}


TEST_F(test_whitelist, load_local_whitelist) {
    std::ofstream file_json;
    file_json.open("bwlist.json");
    int count = 4500;

    xJson::Value root;

    //whitelist
    std::string target_whitelist_str = "";
    std::string account_addr = "T00000LMxyqFyLC5ZWhH9AWdgFcK4bbL1kxyw11W";
    xJson::Value white_arr(xJson::arrayValue);
    for (auto i = 0; i < count; ++i) {
        white_arr.append(xJson::Value(account_addr));
        target_whitelist_str += account_addr + ",";
    }
    root["local_whitelist"] = white_arr;

    //blacklist
    std::string target_blacklist_str = "";
    account_addr = "T00000LhXXoXe5KD6furgsEKJRpT3SuZLuN1MaCf";
    xJson::Value black_arr(xJson::arrayValue);
    for (auto i = 0; i < count; ++i) {
        black_arr.append(xJson::Value(account_addr));
        target_blacklist_str += account_addr + ",";
    }
    root["local_blacklist"] = black_arr;

    xJson::StyledWriter writer;
    file_json << writer.write(root);
    file_json.close();


    std::map<std::string, std::string> result;
    ASSERT_TRUE(xwhitelist_utl_test::load_bwlist_content("bwlist.json", result));
    ASSERT_EQ(target_whitelist_str, result["local_whitelist"]);
    ASSERT_EQ(target_blacklist_str, result["local_blacklist"]);

}

TEST_F(test_whitelist, error_config) {

    std::ofstream file_json;
    file_json.open("bwlist.json");
    int count = 2;

    xJson::Value root;

    //whitelist
    std::string target_whitelist_str = "";
    std::string account_addr = "T00000LhXXoXe5KD6furgsEKJRpT3SuZLuN1MaCf100";
    xJson::Value white_arr(xJson::arrayValue);
    for (auto i = 0; i < count; ++i) {
        white_arr.append(xJson::Value(account_addr + std::to_string(i)));
        target_whitelist_str += (account_addr + std::to_string(i)) + ",";
    }

    root["local_whitelist"] = white_arr;

    xJson::StyledWriter writer;
    file_json << writer.write(root);
    file_json.close();
    std::map<std::string, std::string> result;
    ASSERT_FALSE(xwhitelist_utl_test::load_bwlist_content("bwlist.json", result));

}