#pragma once

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

#include "xvledger/xvblock.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include "xbase/xobject_ptr.h"
#include "xcommon/xaddress.h"

#include <functional>
#include <initializer_list>
#include <unordered_map>

NS_BEG2(top, contract_runtime)

class xtop_block_sniff_config {
    using data_type = std::unordered_map<common::xaccount_address_t, std::function<bool(xobject_ptr_t<base::xvblock_t>)>>;
    data_type m_data;

public:
    xtop_block_sniff_config() = default;
    xtop_block_sniff_config(xtop_block_sniff_config const &) = default;
    xtop_block_sniff_config & operator=(xtop_block_sniff_config const &) = default;
    xtop_block_sniff_config(xtop_block_sniff_config &&) = default;
    xtop_block_sniff_config & operator=(xtop_block_sniff_config &&) = default;
    ~xtop_block_sniff_config() = default;

    xtop_block_sniff_config(std::initializer_list<std::pair<common::xaccount_address_t const, std::function<bool(xobject_ptr_t<base::xvblock_t>)>>> init_list);
    xtop_block_sniff_config & operator=(std::initializer_list<std::pair<common::xaccount_address_t const, std::function<bool(xobject_ptr_t<base::xvblock_t>)>>> init_list);

    bool contains(common::xaccount_address_t const & address) const noexcept;
    std::function<bool(xobject_ptr_t<base::xvblock_t>)> const & get(common::xaccount_address_t const & address) const;
};
using xblock_sniff_config_t = xtop_block_sniff_config;

NS_END2
