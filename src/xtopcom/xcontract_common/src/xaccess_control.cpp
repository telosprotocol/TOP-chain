/*
#include "xcontract_common/xproperties/xaccess_control.h"

NS_BEG3(top, contract_common, properties)

xtop_access_control::xtop_access_control(observer_ptr<base::xvbstate_t> state) noexcept : m_state{state} {
}

xtop_access_control::xtop_access_control(observer_ptr<base::xvbstate_t> state, xproperty_access_control_data_t data) noexcept
  : m_state{state}, m_ac_data{std::move(data)} {
}

void xtop_access_control::load_access_control_data(std::string const & json, std::error_code & ec) {
}

void xtop_access_control::load_access_control_data(xproperty_access_control_data_t data) {
    m_ac_data = std::move(data);
}

void xtop_access_control::rebind(observer_ptr<base::xvbstate_t> state) noexcept {
    m_state = state;
}

void xtop_access_control::create_property(common::xaccount_address_t const & writer, xproperty_identifier_t const & property_id, std::error_code & ec) {
    if (!write_permitted(writer, property_id)) {
        ec = error::xerrc_t::permission_not_allowed;
        return;
    }

    if (has_property(writer, property_id)) {
        return;
    }

    switch (property_id.type()) {
    case xproperty_type_t::token:
        m_state->new_token_var(property_id.full_name());
    default:
        assert(false);
        ec = error::xerrc_t::permission_not_allowed;
        break;
    }
}

bool xtop_access_control::has_property(common::xaccount_address_t const & reader, xproperty_identifier_t const & property_id, std::error_code & ec) const noexcept {
    if (!read_permitted(reader, property_id)) {
        ec = error::xerrc_t::permission_not_allowed;
        return false;
    }

    return m_state->find_property(property_id.full_name());
}

bool xtop_access_control::has_property(common::xaccount_address_t const & reader, xproperty_identifier_t const & property_id) const noexcept {
    std::error_code ec;
    return has_property(reader, property_id, ec);
}

std::string xtop_access_control::src_code(xproperty_identifier_t const & property_id, std::error_code & ec) const {
    assert(m_state != nullptr);
    auto code_var = m_state->load_code_var(property_id.full_name());
    return code_var->get_code();
}

void xtop_access_control::src_code(xproperty_identifier_t const & property_id, std::string code, std::error_code & ec) {
    assert(m_state != nullptr);
    auto code_var = m_state->load_code_var(property_id.full_name());
    if (!code_var->deploy_code(code)) {
        ec = error::xerrc_t::deploy_code_failed;
        return;
    }
}

bool xtop_access_control::read_permitted(common::xaccount_address_t const &,
                                         xproperty_identifier_t const & ) const noexcept {
    return true;
}

bool xtop_access_control::write_permitted(common::xaccount_address_t const & writer,
                                          xproperty_identifier_t const & property_id) const noexcept {
    auto const & perimeters = m_ac_data.control_list(property_id.category(), property_id.type());
    if (perimeters.empty()) {
        return true;
    }

    return static_cast<bool>(perimeters.count(writer));
}

NS_END3
*/
