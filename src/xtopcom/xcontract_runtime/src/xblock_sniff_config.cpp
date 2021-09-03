#include "xbasic/xutility.h"
#include "xcontract_runtime/xblock_sniff_config.h"

NS_BEG2(top, contract_runtime)

xtop_block_sniff_config::xtop_block_sniff_config(std::initializer_list<std::pair<common::xaccount_address_t const, xblock_sniff_config_data_t>> init_list)
  : m_data{std::move(init_list)} {
}

xtop_block_sniff_config & xtop_block_sniff_config::operator=(std::initializer_list<std::pair<common::xaccount_address_t const, xblock_sniff_config_data_t>> init_list) {
    m_data = std::move(init_list);
    return *this;
}

bool xtop_block_sniff_config::contains(common::xaccount_address_t const & address) const noexcept {
    return m_data.find(address) != std::end(m_data);
}

std::function<bool(base::xvblock_ptr_t)> const & xtop_block_sniff_config::get_action(common::xaccount_address_t const & address) const {
    static std::function<bool(base::xvblock_ptr_t)> empty;
    auto const it = m_data.find(address);
    if (it != std::end(m_data)) {
        return top::get<data_type::mapped_type>(*it).action;
    }

    return empty;
}
uint32_t xtop_block_sniff_config::get_interval(common::xaccount_address_t const & address) const {
    auto const it = m_data.find(address);
    if (it != std::end(m_data)) {
        return top::get<data_type::mapped_type>(*it).interval;
    }
    return 0;
}

NS_END2
