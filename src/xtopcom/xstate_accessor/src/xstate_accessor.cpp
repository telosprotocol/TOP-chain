// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_accessor/xstate_accessor.h"

#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xstate_accessor/xerror/xerror.h"

#include <cassert>

namespace top {
namespace state_accessor {

constexpr size_t xtop_state_accessor::property_name_max_length;
constexpr size_t xtop_state_accessor::property_name_min_length;

xtop_state_accessor::xtop_state_accessor(top::observer_ptr<top::base::xvbstate_t> bstate, xstate_access_control_data_t ac_data)
    : bstate_{ bstate }, canvas_{ make_object_ptr<base::xvcanvas_t>() }, ac_data_{ std::move(ac_data) } {
    if (bstate_ == nullptr) {
        top::error::throw_error({ error::xerrc_t::invalid_state_backend });
    }
}

uint64_t xtop_state_accessor::nonce(properties::xproperty_identifier_t const & property_id, std::error_code & ec) const {
    assert(!ec);
    assert(bstate_ != nullptr);

    auto const & property_name = property_id.full_name();
    xobject_ptr_t<base::xnoncevar_t> nonce_property = bstate_->load_nonce_var(property_name);
    if (nonce_property == nullptr) {
        if (system_property(property_id)) {
            nonce_property = bstate_->new_nonce_var(property_name, canvas_.get());
        } else {
            ec = error::xerrc_t::property_not_exist;
            return 0;
        }
    }
    assert(nonce_property != nullptr);
    return nonce_property->get_nonce();
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

    case properties::xproperty_type_t::deque:
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

void xstate_accessor_t::clear_property(properties::xproperty_identifier_t const & property_id, std::error_code & ec) {
    assert(!ec);
    assert(bstate_ != nullptr);

    if (!write_permitted(property_id)) {
        ec = error::xerrc_t::property_access_denied;
        return;
    }

    switch (property_id.type()) {
    case properties::xproperty_type_t::map:
    {
        auto map_property = bstate_->load_string_map_var(property_id.full_name());
        if (map_property == nullptr) {
            ec = error::xerrc_t::property_not_exist;
            return;
        }

        if (!map_property->clear(canvas_.get())) {
            ec = error::xerrc_t::update_property_failed;
            return;
        }
        break;
    }

    case properties::xproperty_type_t::string:
    {
        auto string_property = bstate_->load_string_var(property_id.full_name());
        if (string_property == nullptr) {
            ec = error::xerrc_t::property_not_exist;
            return;
        }

        if (!string_property->clear(canvas_.get())) {
            ec = error::xerrc_t::update_property_failed;
            return;
        }
        break;
    }

    default:
    {
        assert(false);
        // add more clear operation by demand.
        break;
    }
    }
}

size_t xstate_accessor_t::property_size(properties::xproperty_identifier_t const & property_id, std::error_code & ec) const {
    assert(!ec);
    assert(bstate_ != nullptr);

    switch (property_id.type()) {
    case properties::xproperty_type_t::string:
    {
        auto string_property = bstate_->load_string_var(property_id.full_name());
        if (string_property == nullptr) {
            ec = error::xerrc_t::property_not_exist;
            return 0;
        }

        return string_property->query().size();
    }

    case properties::xproperty_type_t::map:
    {
        auto map_property = bstate_->load_string_map_var(property_id.full_name());
        if (map_property == nullptr) {
            ec = error::xerrc_t::property_not_exist;
            return 0;
        }

        return map_property->query().size();
    }

    default:
    {
        assert(false);
        // add more get size oprations by demand.
        return 0;
    }
    }
}

xbyte_buffer_t xtop_state_accessor::bin_code(properties::xproperty_identifier_t const & property_id, std::error_code & ec) const {
    assert(!ec);
    assert(bstate_ != nullptr);

    if (!read_permitted(property_id)) {
        ec = error::xerrc_t::property_access_denied;
        return {};
    }

    auto const & property_name = property_id.full_name();
    auto code_property = bstate_->load_code_var(property_name);
    if (code_property == nullptr) {
        ec = error::xerrc_t::property_not_exist;
        return {};
    }

    auto const & bin_code = code_property->query();
    return { std::begin(bin_code), std::end(bin_code) };
}

xbyte_buffer_t xtop_state_accessor::bin_code(properties::xproperty_identifier_t const & property_id) const {
    std::error_code ec;
    auto bin_code = this->bin_code(property_id, ec);
    top::error::throw_error(ec);
    return bin_code;
}

void xtop_state_accessor::deploy_bin_code(properties::xproperty_identifier_t const & property_id, xbyte_buffer_t const & bin_code, std::error_code & ec) {
    assert(!ec);
    assert(bstate_ != nullptr);

    if (!write_permitted(property_id)) {
        ec = error::xerrc_t::property_access_denied;
        return;
    }

    if (bstate_->find_property(property_id.full_name())) {
        ec = error::xerrc_t::property_already_exist;
        return;
    }

    auto code_property = bstate_->new_code_var(property_id.full_name(), canvas_.get());
    if (code_property == nullptr) {
        ec = error::xerrc_t::create_property_failed;
        return;
    }

    if (!code_property->deploy_code({ std::begin(bin_code), std::end(bin_code) }, canvas_.get())) {
        ec = error::xerrc_t::update_property_failed;
        return;
    }
}

void xtop_state_accessor::deploy_bin_code(properties::xproperty_identifier_t const & property_id, xbyte_buffer_t const & bin_code) {
    std::error_code ec;
    deploy_bin_code(property_id, bin_code, ec);
    top::error::throw_error(ec);
}

bool xtop_state_accessor::property_exist(properties::xproperty_identifier_t const & property_id, std::error_code & ec) const {
    assert(!ec);
    if (read_permitted(property_id)) {
        return bstate_->find_property(property_id.full_name());
    } else {
        ec = error::xerrc_t::property_access_denied;
        return false;
    }
}

bool xtop_state_accessor::property_exist(properties::xproperty_identifier_t const & property_id) const {
    std::error_code ec;
    auto ret = property_exist(property_id, ec);
    top::error::throw_error(ec);
    return ret;
}

common::xaccount_address_t xtop_state_accessor::account_address() const {
    assert(bstate_ != nullptr);
    return common::xaccount_address_t{ bstate_->get_address() };
}

uint64_t xtop_state_accessor::state_height() const {
    assert(bstate_ != nullptr);
    return bstate_->get_block_height();
}

template <>
properties::xtype_of_t<properties::xproperty_type_t::int64>::type xstate_accessor_t::get_property<properties::xproperty_type_t::int64>(properties::xtypeless_property_identifier_t const & property_id, std::error_code & ec) const {
    assert(!ec);
    assert(bstate_ != nullptr);

    auto const & peroperty_name = property_id.full_name();
    auto int_property = bstate_->load_int64_var(peroperty_name);
    if (int_property == nullptr) {
        if (!properties::system_property(property_id)) {
            ec = error::xerrc_t::property_not_exist;
        }

        return {};
    }

    return int_property->get();
}

template <>
properties::xtype_of_t<properties::xproperty_type_t::uint64>::type xstate_accessor_t::get_property<properties::xproperty_type_t::uint64>(properties::xtypeless_property_identifier_t const & property_id, std::error_code & ec) const {
    assert(!ec);
    assert(bstate_ != nullptr);

    auto const & peroperty_name = property_id.full_name();
    auto int_property = bstate_->load_uint64_var(peroperty_name);
    if (int_property == nullptr) {
        if (!properties::system_property(property_id)) {
            ec = error::xerrc_t::property_not_exist;
        }

        return {};
    }

    return int_property->get();
}

template <>
properties::xtype_of_t<properties::xproperty_type_t::string>::type xstate_accessor_t::get_property<properties::xproperty_type_t::string>(properties::xtypeless_property_identifier_t const & property_id, std::error_code & ec) const {
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
properties::xtype_of_t<properties::xproperty_type_t::map>::type xstate_accessor_t::get_property<properties::xproperty_type_t::map>(properties::xtypeless_property_identifier_t const & property_id, std::error_code & ec) const {
    assert(!ec);
    assert(bstate_ != nullptr);

    auto const & peroperty_name = property_id.full_name();
    auto map_property = bstate_->load_string_map_var(peroperty_name);
    if (map_property == nullptr) {
        assert(!properties::system_property(property_id));

        ec = error::xerrc_t::property_not_exist;
        return {};
    }

    auto map = map_property->query();
    properties::xtype_of_t<properties::xproperty_type_t::map>::type ret;
    for (auto & pair : map) {
        ret.insert({ std::move(pair.first), {std::begin(pair.second), std::end(pair.second)} });
    }
    return ret;
}

template <>
properties::xtype_of_t<properties::xproperty_type_t::deque>::type xstate_accessor_t::get_property<properties::xproperty_type_t::deque>(properties::xtypeless_property_identifier_t const & property_id, std::error_code & ec) const {
    assert(!ec);
    assert(bstate_ != nullptr);

    auto const & peroperty_name = property_id.full_name();
    auto deque_property = bstate_->load_string_deque_var(peroperty_name);
    if (deque_property == nullptr) {
        assert(!properties::system_property(property_id));

        ec = error::xerrc_t::property_not_exist;
        return {};
    }

    auto deque = deque_property->query();
    properties::xtype_of_t<properties::xproperty_type_t::deque>::type ret;
    ret.resize(deque.size());
    for (auto i = 0u; i < deque.size(); ++i) {
        ret[i] = { std::begin(deque[i]), std::end(deque[i]) };
    }
    return ret;
}

template <>
void xstate_accessor_t::set_property<properties::xproperty_type_t::int64>(properties::xtypeless_property_identifier_t const & property_id,
                                                                          properties::xtype_of_t<properties::xproperty_type_t::int64>::type const & value,
                                                                          std::error_code & ec) {
    assert(!ec);
    assert(bstate_ != nullptr);

    auto const & property_name = property_id.full_name();
    xobject_ptr_t<base::xvintvar_t<int64_t>> int_property = bstate_->load_int64_var(property_name);
    if (int_property == nullptr) {
        if (properties::system_property(property_id)) {
            int_property = bstate_->new_int64_var(property_name, canvas_.get());
        } else {
            ec = error::xerrc_t::property_not_exist;
            return;
        }
    }

    assert(int_property != nullptr);
    if (!int_property->set(value, canvas_.get())) {
        ec = error::xerrc_t::update_property_failed;
        return;
    }
}

template <>
void xstate_accessor_t::set_property<properties::xproperty_type_t::uint64>(properties::xtypeless_property_identifier_t const & property_id,
                                                                           properties::xtype_of_t<properties::xproperty_type_t::uint64>::type const & value,
                                                                           std::error_code & ec) {
    assert(!ec);
    assert(bstate_ != nullptr);

    auto const & property_name = property_id.full_name();
    xobject_ptr_t<base::xvintvar_t<uint64_t>> int_property = bstate_->load_uint64_var(property_name);
    if (int_property == nullptr) {
        if (properties::system_property(property_id)) {
            int_property = bstate_->new_uint64_var(property_name, canvas_.get());
        } else {
            ec = error::xerrc_t::property_not_exist;
            return;
        }
    }

    assert(int_property != nullptr);
    if (!int_property->set(value, canvas_.get())) {
        ec = error::xerrc_t::update_property_failed;
        return;
    }
}

template <>
void xstate_accessor_t::set_property<properties::xproperty_type_t::string>(properties::xtypeless_property_identifier_t const & property_id,
                                                                           properties::xtype_of_t<properties::xproperty_type_t::string>::type const & value,
                                                                           std::error_code & ec) {
    assert(!ec);
    auto const & property_name = property_id.full_name();
    xobject_ptr_t<base::xstringvar_t> string_property = bstate_->load_string_var(property_name);
    if (string_property == nullptr) {
        if (properties::system_property(property_id)) {
            string_property = bstate_->new_string_var(property_name, canvas_.get());
        } else {
            ec = error::xerrc_t::property_not_exist;
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
void xstate_accessor_t::set_property_cell_value<properties::xproperty_type_t::map>(properties::xtypeless_property_identifier_t const & property_id,
                                                                                   properties::xkey_type_of_t<properties::xproperty_type_t::map>::type const & key,
                                                                                   properties::xvalue_type_of_t<properties::xproperty_type_t::map>::type const & value,
                                                                                   std::error_code & ec) {
    assert(!ec);
    assert(bstate_ != nullptr);
    assert(canvas_ != nullptr);

    assert(!properties::system_property(property_id));

    auto const & property_name = property_id.full_name();
    auto map_property = bstate_->load_string_map_var(property_name);
    if (map_property == nullptr) {
        ec = error::xerrc_t::property_not_exist;
        return;
    }

    assert(map_property != nullptr);
    if (!map_property->insert(key, { std::begin(value), std::end(value) }, canvas_.get())) {
        ec = error::xerrc_t::update_property_failed;
        return;
    }
}

template <>
void xstate_accessor_t::set_property_cell_value<properties::xproperty_type_t::deque>(properties::xtypeless_property_identifier_t const & property_id,
                                                                                     properties::xkey_type_of_t<properties::xproperty_type_t::deque>::type const & key,
                                                                                     properties::xvalue_type_of_t<properties::xproperty_type_t::deque>::type const & value,
                                                                                     std::error_code & ec) {
    assert(!ec);
    assert(bstate_ != nullptr);
    assert(canvas_ != nullptr);

    assert(!properties::system_property(property_id));

    auto const & property_name = property_id.full_name();
    auto deque_property = bstate_->load_string_deque_var(property_name);
    if (deque_property == nullptr) {
        ec = error::xerrc_t::property_not_exist;
        return;
    }



    assert(deque_property != nullptr);
    if (!deque_property->update(key, { std::begin(value), std::end(value) }, canvas_.get())) {
        ec = error::xerrc_t::update_property_failed;
        return;
    }
}

template <>
properties::xvalue_type_of_t<properties::xproperty_type_t::map>::type
xstate_accessor_t::get_property_cell_value<properties::xproperty_type_t::map>(properties::xtypeless_property_identifier_t const & property_id,
                                                                              properties::xkey_type_of_t<properties::xproperty_type_t::map>::type const & key,
                                                                              std::error_code & ec) const {
    assert(!ec);
    assert(bstate_ != nullptr);
    assert(canvas_ != nullptr);

    assert(!properties::system_property(property_id));

    auto const & property_name = property_id.full_name();
    auto map_property = bstate_->load_string_map_var(property_name);
    if (map_property == nullptr) {
        ec = error::xerrc_t::property_not_exist;
        return{};
    }

    assert(map_property != nullptr);
    if (!map_property->find(key)) {
        ec = error::xerrc_t::property_key_not_exist;
        return{};
    }

    auto string = map_property->query(key);
    return { std::begin(string), std::end(string) };
}

template <>
properties::xvalue_type_of_t<properties::xproperty_type_t::deque>::type
xstate_accessor_t::get_property_cell_value<properties::xproperty_type_t::deque>(properties::xtypeless_property_identifier_t const & property_id,
                                                                                properties::xkey_type_of_t<properties::xproperty_type_t::deque>::type const & key,
                                                                                std::error_code & ec) const {
    assert(!ec);
    assert(bstate_ != nullptr);
    assert(canvas_ != nullptr);

    assert(!properties::system_property(property_id));

    auto const & property_name = property_id.full_name();
    auto deque_property = bstate_->load_string_deque_var(property_name);
    if (deque_property == nullptr) {
        ec = error::xerrc_t::property_not_exist;
        return{};
    }

    assert(deque_property != nullptr);
    if (key >= deque_property->query().size()) {
        ec = error::xerrc_t::property_key_not_exist;
        return{};
    }

    auto string = deque_property->query(key);
    return { std::begin(string), std::end(string) };
}

template <>
void xstate_accessor_t::remove_property_cell<properties::xproperty_type_t::map>(properties::xtypeless_property_identifier_t const & property_id, typename properties::xkey_type_of_t<properties::xproperty_type_t::map>::type const & key, std::error_code & ec) {
    assert(!ec);
    assert(bstate_ != nullptr);

    if (!write_permitted({ property_id, properties::xproperty_type_t::map })) {
        ec = error::xerrc_t::property_access_denied;
        return;
    }

    auto map_property = bstate_->load_string_map_var(property_id.full_name());
    if (map_property == nullptr) {
        ec = error::xerrc_t::property_not_exist;
        return;
    }

    if (!map_property->find(key)) {
        ec = error::xerrc_t::property_key_not_exist;
        return;
    }

    if (!map_property->erase(key, canvas_.get())) {
        ec = error::xerrc_t::update_property_failed;
        return;
    }
}

template <>
void xstate_accessor_t::remove_property_cell<properties::xproperty_type_t::deque>(properties::xtypeless_property_identifier_t const & property_id, typename properties::xkey_type_of_t<properties::xproperty_type_t::deque>::type const & key, std::error_code & ec) {
    assert(!ec);
    assert(bstate_ != nullptr);

    if (!write_permitted({ property_id, properties::xproperty_type_t::map })) {
        ec = error::xerrc_t::property_access_denied;
        return;
    }

    auto deque_property = bstate_->load_string_deque_var(property_id.full_name());
    if (deque_property == nullptr) {
        ec = error::xerrc_t::property_not_exist;
        return;
    }

    auto const size = deque_property->query().size();
    if (size == 0) {
        ec = error::xerrc_t::property_key_out_of_range;
        return;
    }

    if (key == 0) {
        if (!deque_property->pop_front(canvas_.get())) {
            ec = error::xerrc_t::update_property_failed;
            return;
        }
    } else if (key >= size - 1) {
        if (!deque_property->pop_back(canvas_.get())) {
            ec = error::xerrc_t::update_property_failed;
            return;
        }
    } else {
        ec = error::xerrc_t::property_key_out_of_range;
        return;
    }
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

#define CREATE_INT_PROPERTY(INT_TYPE)\
    template <>\
    void xstate_accessor_t::do_create_int_property<properties::xproperty_type_t::INT_TYPE>(std::string const & property_name, std::error_code & ec) {\
        assert(!ec);\
        assert(property_name_min_length <= property_name.length() && property_name.length() < property_name_max_length);\
        auto const int_property = bstate_->new_##INT_TYPE##_var(property_name, canvas_.get());\
        if (int_property == nullptr) {\
            ec = error::xerrc_t::create_property_failed;\
            return;\
        }\
    }

CREATE_INT_PROPERTY(int64)
CREATE_INT_PROPERTY(uint64)

#undef CREATE_INT_PROPERTY



}
}
