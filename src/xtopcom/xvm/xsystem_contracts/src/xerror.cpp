#include "xvm/xsystem_contracts/xerror/xerror.h"

#include <string>
#include <type_traits>

NS_BEG4(top, xvm, system_contracts, error)

static char const * errc_to_message(int const errc) noexcept {
    auto ec = static_cast<error::xsystem_contract_errc_t>(errc);
    switch (ec) {
    case xsystem_contract_errc_t::successful:
        return "successful";

    case xsystem_contract_errc_t::serialization_error:
        return "serialization error";

    case xsystem_contract_errc_t::deserialization_error:
        return "deserialization error";

    case xsystem_contract_errc_t::rec_registration_node_info_not_found:
        return "rec registraction constract: node info not found";

    default:
        return "unknown error";
    }
}

class xtop_system_contract_category : public std::error_category {
public:
    const char * name() const noexcept override {
        return "system_contract";
    }

    std::string message(int errc) const override {
        return errc_to_message(errc);
    }
};
using xsystem_contract_category_t = xtop_system_contract_category;

std::error_code make_error_code(xsystem_contract_errc_t errc) noexcept {
    return std::error_code(static_cast<int>(errc), system_contract_category());
}

std::error_condition make_error_condition(xsystem_contract_errc_t errc) noexcept {
    return std::error_condition(static_cast<int>(errc), system_contract_category());
}

std::error_category const & system_contract_category() {
    static xsystem_contract_category_t category;
    return category;
}

xtop_system_contract_execution_error::xtop_system_contract_execution_error(xsystem_contract_errc_t errc) : xtop_system_contract_execution_error{make_error_code(errc)} {
}

xtop_system_contract_execution_error::xtop_system_contract_execution_error(xsystem_contract_errc_t errc, std::string extra_msg) : xtop_system_contract_execution_error{make_error_code(errc), std::move(extra_msg)} {
}

xtop_system_contract_execution_error::xtop_system_contract_execution_error(std::error_code ec) : std::runtime_error{ec.message()}, m_ec{std::move(ec)} {
}

xtop_system_contract_execution_error::xtop_system_contract_execution_error(std::error_code ec, std::string extra_msg)
  : std::runtime_error{extra_msg += ":" + ec.message()}, m_ec{std::move(ec)} {
}

std::error_code const & xtop_system_contract_execution_error::code() const noexcept {
    return m_ec;
}

NS_END4

NS_BEG1(std)

size_t hash<top::xvm::system_contracts::error::xsystem_contract_errc_t>::operator()(top::xvm::system_contracts::error::xsystem_contract_errc_t errc) const noexcept {
    return static_cast<size_t>(static_cast<std::underlying_type<top::xvm::system_contracts::error::xsystem_contract_errc_t>::type>(errc));
}

NS_END1
