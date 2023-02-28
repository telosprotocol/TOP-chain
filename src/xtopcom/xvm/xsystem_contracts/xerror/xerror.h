#pragma once

#include "xbase/xns_macro.h"

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <system_error>
#include <type_traits>

NS_BEG4(top, xvm, system_contracts, error)

enum class xenum_system_contract_errc {
    successful,
    serialization_error,
    deserialization_error,
    rec_registration_node_info_not_found,
};
using xsystem_contract_errc_t = xenum_system_contract_errc;

std::error_code make_error_code(xsystem_contract_errc_t errc) noexcept;
std::error_condition make_error_condition(xsystem_contract_errc_t errc) noexcept;

std::error_category const & system_contract_category();

class xtop_system_contract_execution_error : public std::runtime_error {
public:
    xtop_system_contract_execution_error(xsystem_contract_errc_t const error_code);
    xtop_system_contract_execution_error(xsystem_contract_errc_t const error_code, std::string extra_msg);

    std::error_code const & code() const noexcept;

private:
    xtop_system_contract_execution_error(std::error_code ec);
    xtop_system_contract_execution_error(std::error_code ec, std::string extra_msg);

private:
    std::error_code m_ec;
};
using xsystem_contract_execution_error_t = xtop_system_contract_execution_error;


NS_END4

NS_BEG1(std)

#if !defined(XCXX14)

template <>
struct hash<top::xvm::system_contracts::error::xsystem_contract_errc_t> final {
    size_t operator()(top::xvm::system_contracts::error::xsystem_contract_errc_t errc) const noexcept;
};

template <>
struct is_error_code_enum<top::xvm::system_contracts::error::xsystem_contract_errc_t> : std::true_type {};

template <>
struct is_error_condition_enum<top::xvm::system_contracts::error::xsystem_contract_errc_t> : std::true_type {};

#endif


NS_END1
