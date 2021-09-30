#include "xvnode/xcomponents/xblock_process/xfulltableblock_process.h"

NS_BEG3(top, vnode, components)

xvblock_class_t xfulltableblock_process_t::block_class(xobject_ptr_t<base::xvblock_t> const & vblock) {
    return vblock->get_block_class();
}

xvblock_level_t xfulltableblock_process_t::block_level(xobject_ptr_t<base::xvblock_t> const & vblock) {
    return vblock->get_block_level();
}

void  xfulltableblock_process_t::process_block(xobject_ptr_t<base::xvblock_t> const & vblock) {
    assert(vblock->get_block_class() == xvblock_class_t::enum_xvblock_class_full &&
            vblock->get_block_level() == xvblock_level_t::enum_xvblock_level_table);

    return;
}

data::xfulltableblock_statistic_accounts xfulltableblock_process_t::fulltableblock_statistic_accounts(data::xstatistics_data_t const& block_statistic_data, base::xvnodesrv_t * node_service) {
    using namespace top::data;

    xfulltableblock_statistic_accounts res;

    // process one full tableblock statistic data
    for (auto const & statistic_item: block_statistic_data.detail) {
        auto elect_statistic = statistic_item.second;
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

NS_END3