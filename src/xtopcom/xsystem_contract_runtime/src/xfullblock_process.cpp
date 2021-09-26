#include "xsystem_contract_runtime/xfullblock_process.h"

#include <iostream>

NS_BEG2(top, contract_runtime)
using namespace top::data;


xfulltableblock_statistic_accounts fulltableblock_statistic_accounts(top::data::xstatistics_data_t const& block_statistic_data, base::xvnodesrv_t * node_service) {
    xfulltableblock_statistic_accounts res;

    // process one full tableblock statistic data
    for (auto const & statistic_item: block_statistic_data.detail) {
        auto elect_statistic = statistic_item.second;
        // std::cout << "fulltableblock_statistic_accounts: group size" << elect_statistic.group_statistics_data.size() << "\n";
        xfulltableblock_group_data_t res_group_data;
        for (auto const & group_item: elect_statistic.group_statistics_data) {
            xgroup_related_statistics_data_t const& group_account_data = group_item.second;
            common::xgroup_address_t const& group_addr = group_item.first;
            xvip2_t const& group_xvip2 = top::common::xip2_t{
                group_addr.network_id(),
                group_addr.zone_id(),
                group_addr.cluster_id(),
                group_addr.group_id(),
                (uint16_t)group_account_data.account_statistics_data.size(),
                statistic_item.first
            };

            // std::cout << "fulltableblock_statistic_accounts: xvip2" << group_addr.to_string() << "\n";
            xfulltableblock_account_data_t res_account_data;
            for (std::size_t slotid = 0; slotid < group_account_data.account_statistics_data.size(); ++slotid) {
                auto account_addr = node_service->get_group(group_xvip2)->get_node(slotid)->get_account();
                res_account_data.account_data.emplace_back(std::move(account_addr));
            }

            res_group_data.group_data[group_addr] = res_account_data;
        }

        res.accounts_detail[statistic_item.first] = res_group_data;
    }

    return res;
}

void  process_fulltableblock(xblock_ptr_t const& block) {
    auto block_owner = block->get_block_owner();
    auto block_height = block->get_height();
    // xdbg("process_fulltable fullblock process, owner: %s, height: %" PRIu64, block->get_block_owner().c_str(), block_height);
    base::xauto_ptr<base::xvblock_t> full_block = base::xvchain_t::instance().get_xblockstore()->load_block_object(base::xvaccount_t{block_owner}, block_height, base::enum_xvblock_flag_committed, true);

    xfull_tableblock_t* full_tableblock = dynamic_cast<xfull_tableblock_t*>(full_block.get());
    auto fulltable_statisitc_data = full_tableblock->get_table_statistics();
    base::xvnodesrv_t * node_service_ptr = contract_runtime::system::xsystem_contract_manager_t::instance()->get_node_service();
    auto statistic_accounts = fulltableblock_statistic_accounts(fulltable_statisitc_data, node_service_ptr);

    base::xstream_t stream(base::xcontext_t::instance());
    stream << fulltable_statisitc_data;
    stream << statistic_accounts;
    stream << block_height;
    stream << block->get_pledge_balance_change_tgas();
    std::string action_params = std::string((char *)stream.data(), stream.size());


    // XMETRICS_GAUGE(metrics::xmetircs_tag_t::contract_table_fullblock_event, 1);
    // on_fulltableblock_event("table_address", "on_collect_statistic_info", action_params, block->get_timestamp(), (uint16_t)table_id);
}

NS_END2