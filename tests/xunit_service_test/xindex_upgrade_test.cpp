#include "gtest/gtest.h"
#include "xunit_service/xindex_upgrade.h"

#include <functional>
namespace top {
using namespace xunit_service;

class xindex_upgrade_test : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

public:
};

void check_table_upgrade_turn(xindex_upgrade_t & index_upgrade, std::vector<std::string> tables) {
    for (uint32_t i = 0; i < tables.size(); i++) {
        bool is_turn = index_upgrade.is_turn_to_upgrade(tables[i]);
        xassert(is_turn == (i == 0));
    }

    for (uint32_t i = 0; i < tables.size() - 1; i++) {
        bool is_turn = index_upgrade.is_turn_to_upgrade(tables[i + 1]);
        xassert(is_turn == false);
        index_upgrade.set_upgrade_succ(tables[i]);
        is_turn = index_upgrade.is_turn_to_upgrade(tables[i + 1]);
        xassert(is_turn == true);
    }
}

TEST_F(xindex_upgrade_test, basic) {
    xindex_upgrade_t index_upgrade;
    bool is_turn = false;
    std::vector<std::string> shard0_tables = {
        "Ta0000@0",
        "Ta0000@1",
        "Ta0000@2",
        "Ta0000@3",
        "Ta0000@4",
        "Ta0000@5",
        "Ta0000@6",
        "Ta0000@7",
        "Ta0000@8",
        "Ta0000@9",
        "Ta0000@10",
        "Ta0000@11",
        "Ta0000@12",
        "Ta0000@13",
        "Ta0000@14",
        "Ta0000@15"
    };
    std::vector<std::string> shard1_tables = {
        "Ta0000@16",
        "Ta0000@17",
        "Ta0000@18",
        "Ta0000@19",
        "Ta0000@20",
        "Ta0000@21",
        "Ta0000@22",
        "Ta0000@23",
        "Ta0000@24",
        "Ta0000@25",
        "Ta0000@26",
        "Ta0000@27",
        "Ta0000@28",
        "Ta0000@29",
        "Ta0000@30",
        "Ta0000@31"
    };
    std::vector<std::string> shard2_tables = {
        "Ta0000@32",
        "Ta0000@33",
        "Ta0000@34",
        "Ta0000@35",
        "Ta0000@36",
        "Ta0000@37",
        "Ta0000@38",
        "Ta0000@39",
        "Ta0000@40",
        "Ta0000@41",
        "Ta0000@42",
        "Ta0000@43",
        "Ta0000@44",
        "Ta0000@45",
        "Ta0000@46",
        "Ta0000@47"
    };
    std::vector<std::string> shard3_tables = {
        "Ta0000@48",
        "Ta0000@49",
        "Ta0000@50",
        "Ta0000@51",
        "Ta0000@52",
        "Ta0000@53",
        "Ta0000@54",
        "Ta0000@55",
        "Ta0000@56",
        "Ta0000@57",
        "Ta0000@58",
        "Ta0000@59",
        "Ta0000@60",
        "Ta0000@61",
        "Ta0000@62",
        "Ta0000@63"
    };
    std::vector<std::string> other_tables1 = {
        "Ta0002@0"
    };
    std::vector<std::string> other_tables2 = {
        "Ta0002@1"
    };
    std::vector<std::string> other_tables3 = {
        "Ta0002@2"
    };
    std::vector<std::string> other_tables4 = {
        "Ta0001@0"
    };
    std::vector<std::string> other_tables5 = {
        "Ta0004@0"
    };
    std::vector<std::string> other_tables6 = {
        "Ta0005@0"
    };

    check_table_upgrade_turn(index_upgrade, shard0_tables);
    check_table_upgrade_turn(index_upgrade, shard1_tables);
    check_table_upgrade_turn(index_upgrade, shard2_tables);
    check_table_upgrade_turn(index_upgrade, shard3_tables);
    check_table_upgrade_turn(index_upgrade, other_tables1);
    check_table_upgrade_turn(index_upgrade, other_tables2);
    check_table_upgrade_turn(index_upgrade, other_tables3);
    check_table_upgrade_turn(index_upgrade, other_tables4);
    check_table_upgrade_turn(index_upgrade, other_tables5);
    check_table_upgrade_turn(index_upgrade, other_tables6);
}

}  // namespace top
