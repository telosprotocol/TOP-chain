#include "CLI11.hpp"
#include "../xdb_tool.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xrootblock.h"

#include <iostream>
#include <functional>

class xtop_hash_t : public top::base::xhashplugin_t {
public:
    xtop_hash_t()
      : top::base::xhashplugin_t(-1)  //-1 = support every hash types
    {
    }

private:
    xtop_hash_t(const xtop_hash_t &) = delete;
    xtop_hash_t & operator=(const xtop_hash_t &) = delete;

public:
    virtual ~xtop_hash_t(){};
    virtual const std::string hash(const std::string & input, enum_xhash_type type) override {
        xassert(type == enum_xhash_type_sha2_256);
        auto hash = top::utl::xsha2_256_t::digest(input);
        return std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    }
};



int main(int argc, char** argv) {
    auto hash_plugin = new xtop_hash_t();
    top::config::config_register.get_instance().set(top::config::xchain_name_configuration_t::name, std::string{top::config::chain_name_mainnet});
    top::data::xrootblock_para_t para;
    top::data::xrootblock_t::init(para);
    xdb_tool tool_instance;


    std::string db_path;
    CLI::App dbtool_app{"db tool to get info directly from db."};
    dbtool_app.add_option("db_path", db_path, "db file path.")->required();
    dbtool_app.parse_complete_callback(std::bind(&xdb_tool::init_xdb_tool, &tool_instance, std::ref(db_path)));

    std::string block_addr;
    // get block latest height
    auto blockheight_cmd =  dbtool_app.add_subcommand("blockheight", "get latest blockheight");
    blockheight_cmd->add_option("block_addr", block_addr, "the block address.")->required();
    blockheight_cmd->callback(std::bind(&xdb_tool::get_blockheight, &tool_instance, std::ref(block_addr)));

    // get blockVoteInfo
    auto blockVoteInfo_cmd =  dbtool_app.add_subcommand("blockVoteInfo", "get voteinfo from block");
    uint64_t start_height;
    uint64_t end_height;
    blockVoteInfo_cmd->add_option("block_addr", block_addr, "the block address.")->required();
    blockVoteInfo_cmd->add_option("start_height", start_height, "the start height.")->required();
    blockVoteInfo_cmd->add_option("end_height", end_height, "the end height.")->required();
    blockVoteInfo_cmd->callback(std::bind(&xdb_tool::get_voteinfo_from_block, &tool_instance, std::ref(block_addr), std::ref(start_height), std::ref(end_height)));

    // get electInfo by height
    auto conselectInfo_cmd = dbtool_app.add_subcommand("consElectInfoByHeight", "get consenesus elect info by height");
    uint64_t elect_height;
    bool print = true;
    conselectInfo_cmd->add_option("elect_height", elect_height, "the height of elcet block")->required();
    conselectInfo_cmd->callback(std::bind(&xdb_tool::cons_electinfo_by_height, &tool_instance, std::ref(elect_height), std::ref(print)));

    // get all electInfo
    auto all_conselectInfo_cmd = dbtool_app.add_subcommand("allConsElectInfo", "get all consenesus elect info");
    all_conselectInfo_cmd->callback(std::bind(&xdb_tool::all_cons_electinfo, &tool_instance));


    // get all need info for credit problem
    auto credit_data_cmd = dbtool_app.add_subcommand("credit_data", "get credit_data from db");
    uint16_t table_id = 0;
    credit_data_cmd->add_option("table_id", table_id, "the credit_data of  tableblock addr");
    credit_data_cmd->callback(std::bind(&xdb_tool::credit_data, &tool_instance, std::ref(table_id)));

    // specific clock height range data for credit problem
    auto clock_height_credit_data_cmd = dbtool_app.add_subcommand("gmttime_range_credit_data", "get gmttime range credit data from db");
    uint64_t start_gmttime = 1;
    uint64_t end_gmttime = 1;
    clock_height_credit_data_cmd->add_option("start_gmttime", start_gmttime, "the start gmttime")->required();
    clock_height_credit_data_cmd->add_option("end_gmttime", end_gmttime, "the end gmttime")->required();
    clock_height_credit_data_cmd->callback(std::bind(&xdb_tool::specific_clockheight, &tool_instance, std::ref(start_gmttime), std::ref(end_gmttime)));

    // get db statistic
    auto db_statistic_cmd = dbtool_app.add_subcommand("get_fulltable_statistic", "get db statistic info");
    std::string table_address;
    uint64_t height;
    db_statistic_cmd->add_option("table_address", table_address, "the table address")->required();
    db_statistic_cmd->add_option("height", height, "the height of the fulltableblock")->required();
    db_statistic_cmd->callback(std::bind(&xdb_tool::get_fulltable_statistic, &tool_instance, std::ref(table_address), std::ref(height)));

    CLI11_PARSE(dbtool_app, argc, argv);
    return 0;
}
