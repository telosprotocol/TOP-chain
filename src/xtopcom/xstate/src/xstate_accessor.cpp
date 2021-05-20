// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate/xstate_accessor.h"

#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xstate/xerror/xerror.h"

#include <cassert>

namespace top {
namespace state {

constexpr size_t xtop_state_accessor::property_name_max_length;
constexpr size_t xtop_state_accessor::property_name_min_length;

xtop_state_accessor::xtop_state_accessor(top::observer_ptr<top::base::xvbstate_t> bstate, xstate_access_control_data_t ac_data)
    : bstate_{ bstate }, canvas_{ make_object_ptr<base::xvcanvas_t>() }, ac_data_{ std::move(ac_data) } {
    if (bstate_ == nullptr) {
        top::error::throw_error({ error::xerrc_t::invalid_state_backend });
    }
}

static std::string token_property_name(properties::xproperty_identifier_t const & property_id, std::string const & symbol) {
    return property_id.full_name() + "_" + symbol;
}

xtoken_t xtop_state_accessor::withdraw(properties::xproperty_identifier_t const & property_id, std::string const & symbol, uint64_t const amount, std::error_code & ec) {
    assert(!ec);
    assert(bstate_ != nullptr);

    if (property_id.type() != properties::xproperty_type_t::token) {
        ec = error::xerrc_t::invalid_property_type;
        return xtoken_t{};
    }

    if (static_cast<base::vtoken_t>(amount) <= 0) {
        ec = error::xerrc_t::property_value_out_of_range;
        return xtoken_t{};
    }

    if (!write_permitted(property_id)) {
        ec = error::xerrc_t::property_access_denied;
        return xtoken_t{};
    }

    auto const & property_name = token_property_name(property_id, symbol);

    if (!bstate_->find_property(property_name)) {
        ec = error::xerrc_t::token_insufficient;
        return xtoken_t{ symbol };
    }

    auto token_property = bstate_->load_token_var(property_name);
    if (token_property == nullptr) {
        ec = error::xerrc_t::load_property_failed;
        return xtoken_t{ symbol };
    }

    auto const balance = token_property->get_balance();
    if (balance < 0 || amount > static_cast<uint64_t>(balance)) {
        ec = error::xerrc_t::token_insufficient;
        return xtoken_t{ symbol };
    }

    auto const new_balance = token_property->withdraw(static_cast<base::vtoken_t>(amount), canvas_.get());
    assert(new_balance < balance);
    return xtoken_t{ amount, symbol };
}

void xtop_state_accessor::deposit(properties::xproperty_identifier_t const & property_id, std::string const & symbol, xtoken_t & amount, std::error_code & ec) {
    assert(!ec);

    if (property_id.type() != properties::xproperty_type_t::token) {
        ec = error::xerrc_t::invalid_property_type;
        return;
    }

    if (static_cast<base::vtoken_t>(amount.value())) {
        ec = error::xerrc_t::property_value_out_of_range;
        return;
    }

    if (!write_permitted(property_id)) {
        ec = error::xerrc_t::property_access_denied;
        return;
    }

    auto const & property_name = token_property_name(property_id, symbol);

    xobject_ptr_t<base::xtokenvar_t> token_property{ nullptr };
    if (!bstate_->find_property(property_name)) {
        if (properties::system_property(property_id)) {
            token_property = bstate_->new_token_var(property_name, canvas_.get());
        } else {
            ec = error::xerrc_t::property_not_exist;
            return;
        }
    } else {
        token_property = bstate_->load_token_var(property_name);
    }

    if (token_property == nullptr) {
        ec = error::xerrc_t::load_property_failed;
        return;
    }

    auto const balance = token_property->get_balance();
    auto const new_balance = token_property->deposit(static_cast<base::vtoken_t>(amount.value()), canvas_.get());
    if (new_balance < balance) {
        ec = error::xerrc_t::property_value_out_of_range;
        return;
    }

    amount.clear();
}

uint64_t xtop_state_accessor::balance(properties::xproperty_identifier_t const & property_id, std::string const & symbol, std::error_code & ec) const {
    assert(!ec);

    if (property_id.type() != properties::xproperty_type_t::token) {
        ec = error::xerrc_t::invalid_property_type;
        return 0 ;
    }

    if (!read_permitted(property_id)) {
        ec = error::xerrc_t::property_access_denied;
        return 0;
    }

    auto const & property_name = token_property_name(property_id, symbol);
    if (!bstate_->find_property(property_name)) {
        return 0;
    }

    auto token_property = bstate_->load_token_var(property_name);
    if (token_property == nullptr) {
        ec = error::xerrc_t::load_property_failed;
        return 0;
    }

    auto const balance = token_property->get_balance();
    return balance;
}

void xtop_state_accessor::create_property(properties::xproperty_identifier_t const & property_id, std::error_code & ec) {
    assert(!ec);

    if (properties::system_property(property_id)) {
        ec = error::xerrc_t::property_access_denied;
        return;
    }

    auto const & property_name = property_id.full_name();
    if (property_name.length() < property_name_min_length || property_name.length() >= property_name_max_length) {
        ec = error::xerrc_t::property_name_out_of_range;
        return;
    }

    if (bstate_->find_property(property_name)) {
        ec = error::xerrc_t::property_already_exist;
        return;
    }

    switch (property_id.type()) {
    case properties::xproperty_type_t::string:
        do_create_string_property(property_name, ec);
        break;

    case properties::xproperty_type_t::map:
        do_create_map_property(property_name, ec);
        break;

    case properties::xproperty_type_t::vector:
        break;

    default:
        break;
    }
}

bool xtop_state_accessor::read_permitted(properties::xproperty_identifier_t const & property_id) const noexcept {
    return true;
}

bool xtop_state_accessor::write_permitted(properties::xproperty_identifier_t const & property_id) const noexcept {
    return true;
}

bool xtop_state_accessor::read_permitted(std::string const & property_full_name) const noexcept {
    return true;
}

void xtop_state_accessor::do_create_string_property(std::string const & property_name, std::error_code & ec) {
    assert(!ec);
    assert(property_name_min_length <= property_name.length() && property_name.length() < property_name_max_length);

    auto const string_property = bstate_->new_string_var(property_name, canvas_.get());
    if (string_property == nullptr) {
        ec = error::xerrc_t::create_property_failed;
        return;
    }
}

void xtop_state_accessor::do_create_map_property(std::string const & property_name, std::error_code & ec) {
    assert(!ec);
    assert(property_name_min_length <= property_name.length() && property_name.length() < property_name_max_length);

    auto const map_property = bstate_->new_string_map_var(property_name, canvas_.get());
    if (map_property == nullptr) {
        ec = error::xerrc_t::create_property_failed;
        return;
    }
}

template <>
void xstate_accessor_t::do_create_int_property<properties::xproperty_type_t::uint64>(std::string const & property_name, std::error_code & ec) {
    assert(!ec);
    assert(property_name_min_length <= property_name.length() && property_name.length() < property_name_max_length);

    auto const uint64_property = bstate_->new_uint64_var(property_name, canvas_.get());
    if (uint64_property == nullptr) {
        ec = error::xerrc_t::create_property_failed;
        return;
    }
}

template <>
void xstate_accessor_t::update_property<properties::xproperty_type_t::string>(properties::xtyped_property_identifier_t<properties::xproperty_type_t::string> const & property_id, std::string const & value, std::error_code & ec) {
    assert(!ec);
    auto const & property_name = property_id.full_name();
    xobject_ptr_t<base::xstringvar_t> string_property{ nullptr };

    if (!bstate_->find_property(property_name)) {
        if (properties::system_property(property_id)) {
            string_property = bstate_->new_string_var(property_name, canvas_.get());
        } else {
            ec = error::xerrc_t::property_not_exist;
            return;
        }
    } else {
        string_property = bstate_->load_string_var(property_name);
        if (string_property == nullptr) {
            ec = error::xerrc_t::load_property_failed;
            return;
        }
    }
    assert(string_property != nullptr);
    if (!string_property->reset(value, canvas_.get())) {
        ec = error::xerrc_t::update_property_failed;
        return;
    }
}

template <>
void xstate_accessor_t::update_property<properties::xproperty_type_t::map>(properties::xtyped_property_identifier_t<properties::xproperty_type_t::map> const & property_id, properties::xelement_type_of_t<properties::xproperty_type_t::map>::type const & value, std::error_code & ec) {
    assert(!ec);

    auto const & property_name = property_id.full_name();
    xobject_ptr_t<base::xmapvar_t<std::string>> map_property{ nullptr };

    if (!bstate_->find_property(property_name)) {
        if (properties::system_property(property_id)) {
            map_property = bstate_->new_string_map_var(property_name, canvas_.get());
        } else {
            ec = error::xerrc_t::property_not_exist;
            return;
        }
    } else {
        map_property = bstate_->load_string_map_var(property_name);
        if (map_property == nullptr) {
            ec = error::xerrc_t::load_property_failed;
            return;
        }
    }
    assert(map_property != nullptr);
    if (!map_property->insert(value.first, { std::begin(value.second), std::end(value.second) }, canvas_.get())) {
        ec = error::xerrc_t::update_property_failed;
        return;
    }
}

template <>
properties::xtype_of_t<properties::xproperty_type_t::string>::type xstate_accessor_t::get_property<properties::xproperty_type_t::string>(properties::xtyped_property_identifier_t<properties::xproperty_type_t::string> const & property_id, std::error_code & ec) const {
    assert(!ec);
    assert(bstate_ != nullptr);

    auto const & peroperty_name = property_id.full_name();
    auto string_property = bstate_->load_string_var(peroperty_name);
    if (string_property == nullptr) {
        if (!properties::system_property(property_id)) {
            ec = error::xerrc_t::property_not_exist;
        }

        return {};
    }

    return string_property->query();
}

template <>
properties::xtype_of_t<properties::xproperty_type_t::map>::type xstate_accessor_t::get_property<properties::xproperty_type_t::map>(properties::xtyped_property_identifier_t<properties::xproperty_type_t::map> const & property_id, std::error_code & ec) const {
    assert(!ec);
    assert(bstate_ != nullptr);

    auto const & peroperty_name = property_id.full_name();
    auto map_property = bstate_->load_string_map_var(peroperty_name);
    if (map_property == nullptr) {
        if (!properties::system_property(property_id)) {
            ec = error::xerrc_t::property_not_exist;
        }

        return {};
    }

    auto map = map_property->query();
    properties::xtype_of_t<properties::xproperty_type_t::map>::type ret;
    for (auto & pair : map) {
        ret.insert({ std::move(pair.first), {std::begin(pair.second), std::end(pair.second)} });
    }
    return ret;
}

#define INCREASE_INT_PROPERTY(INT_TYPE)\
    template <>\
    void xstate_accessor_t::increase_int_property<properties::xproperty_type_t::INT_TYPE>(properties::xtyped_property_identifier_t<properties::xproperty_type_t::INT_TYPE> const & property_id, std::make_unsigned<properties::xtype_of_t<properties::xproperty_type_t::INT_TYPE>::type>::type const change_amount, std::error_code & ec) {\
        if (change_amount == 0) {\
            ec = error::xerrc_t::property_not_changed;\
            return;\
        }\
        xobject_ptr_t<base::xvintvar_t<INT_TYPE ## _t>> int_property = bstate_->load_ ## INT_TYPE ## _var(property_id.full_name());\
        if (int_property == nullptr) {\
            if (properties::system_property<properties::xproperty_type_t::INT_TYPE>(property_id)) {\
                int_property = bstate_->new_ ## INT_TYPE ## _var(property_id.full_name(), canvas_.get());\
            } else {\
                ec = error::xerrc_t::property_not_exist;\
                return;\
            }\
        } else {\
            auto const value = int_property->get();\
            auto const diff = std::numeric_limits<INT_TYPE ## _t>::max() - value;\
            if (value + change_amount <= value) {\
                ec = error::xerrc_t::property_value_out_of_range;\
                return;\
            }\
            if (int_property->set(value + change_amount, canvas_.get()) == false) {\
                ec = error::xerrc_t::update_property_failed;\
                return;\
            }\
        }\
    }

INCREASE_INT_PROPERTY(uint64)

#undef INCREASE_INT_PROPERTY

#define DECREASE_INT_PROPERTY(INT_TYPE)\
    template <>\
    void xstate_accessor_t::decrease_int_property<properties::xproperty_type_t::INT_TYPE>(properties::xtyped_property_identifier_t<properties::xproperty_type_t::INT_TYPE> const & property_id, std::make_unsigned<properties::xtype_of_t<properties::xproperty_type_t::INT_TYPE>::type>::type const change_amount, std::error_code & ec) {\
        if (change_amount == 0) {\
            ec = error::xerrc_t::property_not_changed;\
            return;\
        }\
        xobject_ptr_t<base::xvintvar_t<INT_TYPE ## _t>> int_property = bstate_->load_ ## INT_TYPE ## _var(property_id.full_name());\
        if (int_property == nullptr) {\
            if (properties::system_property<properties::xproperty_type_t::INT_TYPE>(property_id)) {\
                int_property = bstate_->new_ ## INT_TYPE ## _var(property_id.full_name(), canvas_.get());\
            } else {\
                ec = error::xerrc_t::property_not_exist;\
                return;\
            }\
        } else {\
            auto const value = int_property->get();\
            if (value - change_amount >= value) {\
                ec = error::xerrc_t::property_value_out_of_range;\
                return;\
            }\
            if (int_property->set(value + change_amount, canvas_.get()) == false) {\
                ec = error::xerrc_t::update_property_failed;\
                return;\
            }\
        }\
    }

DECREASE_INT_PROPERTY(uint64)

#undef DECREASE_INT_PROPERTY

}
}
