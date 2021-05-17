// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xproperties/xproperty_access_control.h"

#include "xbasic/xerror/xthrow_error.h"

#include <cassert>

NS_BEG3(top, contract_common, properties)


void xtop_property_utl::property_assert(bool condition, error::xerrc_t error_enum, std::string const& exception_msg) {
    if (!condition) {
        std::error_code ec{ error_enum };
        top::error::throw_error(ec, exception_msg);
    }
}


xtop_property_access_control::xtop_property_access_control(top::observer_ptr<top::base::xvbstate_t> bstate,
                                                           xproperty_access_control_data_t ac_data)
    : bstate_(bstate), canvas_{ make_object_ptr<base::xvcanvas_t>() }, ac_data_(ac_data) {
}

/**
 *
 * @brief  map apis
 *
 */
template<>
void xtop_property_access_control::map_prop_create<std::string, std::string>(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id) {
    auto prop_name = prop_id.full_name();
    if (write_permitted(user, prop_id)) {
        property_assert(!property_exist(user, prop_id), "[xtop_property_access_control::map_prop_create]property already exist，prop_name: " + prop_name, error::xerrc_t::property_already_exist);
        auto prop = bstate_->new_string_map_var(prop_name, canvas_.get());
        property_assert(prop, "[xtop_property_access_control::map_prop_create]property create error，prop_name: " + prop_name);
    } else {
        std::error_code ec{ error::xerrc_t::property_permission_not_allowed };
        top::error::throw_error(ec, "[xtop_property_access_control::map_prop_create]permission denied");
    }
}

template<>
void xtop_property_access_control::map_prop_add<std::string, std::string>(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id, std::string const& prop_key, std::string const& prop_value) {
    auto prop_name = prop_id.full_name();
    if (write_permitted(user, prop_id)) {
        auto prop = bstate_->load_string_map_var(prop_name);
        property_assert(prop, "[xtop_property_access_control::map_prop_add]property not exist, prop_name: " + prop_name + " , prop_key: " + prop_key + ", prop_value: " + prop_value);
        property_assert(prop->insert(prop_key, prop_value, canvas_.get()), "[xtop_property_access_control::map_prop_add]property insert error, prop_name: " + prop_name + " , prop_key: " + prop_key + ", prop_value: " + prop_value);
    } else {
        std::error_code ec{ error::xerrc_t::property_permission_not_allowed };
        top::error::throw_error(ec, "[xtop_property_access_control::map_prop_add]permission denied");
    }
}

template<>
void xtop_property_access_control::map_prop_update<std::string, std::string>(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id, std::string const& prop_key,  std::string const& prop_value) {
    auto prop_name = prop_id.full_name();
    if (write_permitted(user, prop_id)) {
        auto prop = bstate_->load_string_map_var(prop_name);
        property_assert(prop, "[xtop_property_access_control::map_prop_update]property not exist, prop_name: " + prop_name + " , prop_key: " + prop_key + ", prop_value: " + prop_value);
        property_assert(prop->find(prop_key), "[xtop_property_access_control::map_prop_update]property update key not exist,  prop_name: " + prop_name + " , prop_key: " + prop_key + ", prop_value: " + prop_value);

        property_assert(prop->insert(prop_key, prop_value, canvas_.get()), "[xtop_property_access_control::map_prop_update]property update key error, prop_name: " + prop_name + " , prop_key: " + prop_key + ", prop_value: " + prop_value);
    } else {
        std::error_code ec{ error::xerrc_t::property_permission_not_allowed };
        top::error::throw_error(ec, "[xtop_property_access_control::map_prop_update]permission denied");
    }

}


template<>
void xtop_property_access_control::map_prop_update<std::string, std::string>(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id, std::map<std::string, std::string> const& prop_value) {
    auto prop_name = prop_id.full_name();
    if (write_permitted(user, prop_id)) {
        auto prop = bstate_->load_string_map_var(prop_name);
        property_assert(prop, "[xtop_property_access_control::map_prop_update]property not exist, prop_name: " + prop_name);
        property_assert(prop->reset(prop_value, canvas_.get()), "[xtop_property_access_control::map_prop_update]property update error, prop_name: " + prop_name);
    } else {
        std::error_code ec{ error::xerrc_t::property_permission_not_allowed };
        top::error::throw_error(ec, "[xtop_property_access_control::map_prop_update]permission denied");
    }

}

template<>
void xtop_property_access_control::map_prop_erase<std::string, std::string>(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id, std::string const& prop_key) {
    auto prop_name = prop_id.full_name();
    if (write_permitted(user, prop_id)) {
        auto prop = bstate_->load_string_map_var(prop_name);
        property_assert(prop, "[ xtop_property_access_control::map_prop_erase]property not exist, prop_name: " + prop_name + ", prop_key: " + prop_key);
        property_assert(prop->find(prop_key), "[ xtop_property_access_control::map_prop_erase]property erase key invalid, prop_name: " + prop_name + ", prop_key: " + prop_key);

        property_assert(prop->erase(prop_key, canvas_.get()), "[ xtop_property_access_control::map_prop_erase]property erase key error, prop_name: " + prop_name + ", prop_key: " + prop_key);
    } else {
        std::error_code ec{ error::xerrc_t::property_permission_not_allowed };
        top::error::throw_error(ec, "[xtop_property_access_control::map_prop_update]permission denied");
    }

}

template<>
void xtop_property_access_control::map_prop_clear<std::string, std::string>(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id) {
    auto prop_name = prop_id.full_name();
    if (write_permitted(user, prop_id)) {
        auto prop = bstate_->load_string_map_var(prop_name);
        property_assert(prop, "[xtop_property_access_control::map_prop_clear]property not exist, prop_name: " + prop_name);

        xassert(false); // TODO(jimmy)
        // property_assert(prop->reset(), "[xtop_property_access_control::map_prop_clear]property reset error,  prop_name: " + prop_name);
    } else {
        std::error_code ec{ error::xerrc_t::property_permission_not_allowed };
        top::error::throw_error(ec, "[xtop_property_access_control::map_prop_clear]permission denied");
    }

}

template<>
std::string xtop_property_access_control::map_prop_query<std::string, std::string>(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id, std::string const& prop_key) {
    auto prop_name = prop_id.full_name();
    if (read_permitted(user, prop_id)) {
        auto prop = bstate_->load_string_map_var(prop_name);
        property_assert(prop, "[xtop_property_access_control::map_prop_query]property not exist, prop_name: " + prop_name + ", prop_key: " + prop_key);
        return prop->query(prop_key);
    } else {
        std::error_code ec{ error::xerrc_t::property_permission_not_allowed };
        top::error::throw_error(ec, "[xtop_property_access_control::map_prop_query]permission denied");
        return {};
    }
}


template<>
std::map<std::string, std::string> xtop_property_access_control::map_prop_query<std::string, std::string>(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id) {
    auto prop_name = prop_id.full_name();
    if (write_permitted(user, prop_id)) {
        auto prop = bstate_->load_string_map_var(prop_name);
        property_assert(prop, "[xtop_property_access_control::map_prop_query]property not exist, prop_name: " + prop_name);
        return prop->query();
    } else {
        std::error_code ec{ error::xerrc_t::property_permission_not_allowed };
        top::error::throw_error(ec, "[xtop_property_access_control::map_prop_query]permission denied");
        return {};
    }

}


/**
 *
 * @brief str apis
 *
 */
void xtop_property_access_control::STR_PROP_CREATE(std::string const& prop_name) {
    auto prop = bstate_->new_string_var(prop_name, canvas_.get());

    property_assert(prop, "[STR_PROP_CREATE]str property create error, prop_name: " + prop_name);
}


void xtop_property_access_control::STR_PROP_UPDATE(std::string const& prop_name, std::string const& prop_value) {
    auto prop = bstate_->load_string_var(prop_name);
    property_assert(prop, "[STR_PROP_UPDATE]str property not exist, prop_name: " + prop_name + ", prop_value: " + prop_value);

    property_assert(prop->reset(prop_value, canvas_.get()), "[STR_PROP_UPDATE]str property update error, prop_name: " + prop_name + ", prop_value: " + prop_value);
}

std::string xtop_property_access_control::STR_PROP_QUERY(std::string const& prop_name) {
    auto prop = bstate_->load_string_var(prop_name);
    property_assert(prop, "[STR_PROP_QUERY]str property not exist, prop_name: " + prop_name);

    return prop->query();
}


/**
 *
 * @brief token apis
 *
 */
void xtop_property_access_control::token_prop_create(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id) {
    auto prop_name = prop_id.full_name();
    if (write_permitted(user, prop_id)) {
        property_assert(!property_exist(user, prop_id), "[xtop_property_access_control::token_prop_create]property already exist, prop_name: " + prop_name, error::xerrc_t::property_already_exist);
        auto prop = bstate_->new_token_var(prop_name, canvas_.get());

        property_assert(prop, "[xtop_property_access_control::token_prop_create]property create error, prop_name: " + prop_name);
    } else {
        std::error_code ec{ error::xerrc_t::property_permission_not_allowed };
        top::error::throw_error(ec, "[xtop_property_access_control::token_prop_create]permission denied");
    }

}

uint64_t xtop_property_access_control::withdraw(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id, uint64_t amount) {
    auto prop_name = prop_id.full_name();
    if (write_permitted(user, prop_id)) {
        auto prop = bstate_->load_token_var(prop_name);
        property_assert(prop, "[xtop_property_access_control::withdraw]property not exist, token_prop: " + prop_name + ", amount: " + std::to_string(amount));
        return prop->withdraw((base::vtoken_t)amount, canvas_.get());
    } else {
        std::error_code ec{ error::xerrc_t::property_permission_not_allowed };
        top::error::throw_error(ec, "[xtop_property_access_control::withdraw]permission denied");
        return {};
    }
}

uint64_t xtop_property_access_control::deposit(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id, uint64_t amount) {
    auto prop_name = prop_id.full_name();
    if (write_permitted(user, prop_id)) {
        auto prop = bstate_->load_token_var(prop_name);
        property_assert(prop, "[xtop_property_access_control::deposit]token property not exist, token_prop: " + prop_name + ", amount: " + std::to_string(amount));
        return prop->deposit((base::vtoken_t)amount, canvas_.get());
    } else {
        std::error_code ec{ error::xerrc_t::property_permission_not_allowed };
        top::error::throw_error(ec, "[xtop_property_access_control::deposit]permission denied");
        return {};
    }

}

uint64_t xtop_property_access_control::balance(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id) {
    auto prop_name = prop_id.full_name();
    if (write_permitted(user, prop_id)) {
        auto prop = bstate_->load_token_var(prop_name);
        property_assert(prop, "[xtop_property_access_control::balance]property not exist, token_prop: " + prop_name);
        return (uint64_t)prop->get_balance();
    } else {
        std::error_code ec{ error::xerrc_t::property_permission_not_allowed };
        top::error::throw_error(ec, "[xtop_property_access_control::balance]permission denied");
        return {};
    }

}



/**
 * @brief code api
 *
 */
void xtop_property_access_control::code_prop_create(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id) {
    auto prop_name = prop_id.full_name();
    if (write_permitted(user, prop_id)) {
        property_assert(!property_exist(user, prop_id), "[xtop_property_access_control::code_prop_create]property already exist, prop_name: " + prop_name, error::xerrc_t::property_already_exist);
        auto prop = bstate_->new_code_var(prop_name, canvas_.get());
        property_assert(prop, "[xtop_property_access_control::code_prop_create]property create error, prop_name: " + prop_name);
    } else {
        std::error_code ec{ error::xerrc_t::property_permission_not_allowed };
        top::error::throw_error(ec, "[xtop_property_access_control::code_prop_create]permission denied");
    }
}



std::string xtop_property_access_control::code_prop_query(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id) {
    auto prop_name = prop_id.full_name();
    if (read_permitted(user, prop_id)) {
        auto prop = bstate_->load_code_var(prop_name);
        property_assert(prop, "[xtop_property_access_control::code_prop_query]property not exist, prop_name: " + prop_name);

        return prop->query();
    } else {
        std::error_code ec{ error::xerrc_t::property_permission_not_allowed };
        top::error::throw_error(ec, "[xtop_property_access_control::code_prop_query]permission denied");
        return {};
    }
}

bool xtop_property_access_control::code_prop_update(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id, std::string const& code) {
    auto prop_name = prop_id.full_name();
    if (write_permitted(user, prop_id)) {
        auto prop = bstate_->load_code_var(prop_name);
        property_assert(prop, "[xtop_property_access_control::code_prop_query]property not exist, prop_name: " + prop_name);
        return prop->deploy_code(code, canvas_.get());

    } else {
        std::error_code ec{ error::xerrc_t::property_permission_not_allowed };
        top::error::throw_error(ec, "[xtop_property_access_control::code_prop_query]permission denied");
        return {};
    }
}


std::string xtop_property_access_control::src_code(xproperty_identifier_t const & prop_id, std::error_code & ec) const {
    assert(!ec);
    auto prop_name = prop_id.full_name();
    auto prop = bstate_->load_code_var(prop_name);
    property_assert(prop, "[xtop_property_access_control::src_code]property not exist, prop_name: " + prop_name);

    return prop->query();
}

std::string xtop_property_access_control::src_code(xproperty_identifier_t const & prop_id) const {
    std::error_code ec;
    auto r = src_code(prop_id, ec);
    top::error::throw_error(ec);
    return r;
}

void xtop_property_access_control::deploy_src_code(xproperty_identifier_t const & prop_id, std::string src_code, std::error_code & ec) {
    assert(!ec);
    if (bstate_->find_property(prop_id.full_name())) {
        ec = error::xerrc_t::property_already_exist;
        return;
    }

    auto src_prop = bstate_->new_code_var(prop_id.full_name(), canvas_.get());
    if (!src_prop->deploy_code(src_code, canvas_.get())) {
        ec = error::xerrc_t::deploy_code_failed;
    }
}

void xtop_property_access_control::deploy_src_code(xproperty_identifier_t const & prop_id, std::string src_code) {
    std::error_code ec;
    deploy_src_code(prop_id, std::move(src_code), ec);
    top::error::throw_error(ec);
}

xbyte_buffer_t xtop_property_access_control::bin_code(xproperty_identifier_t const & prop_id, std::error_code & ec) const {
    assert(!ec);
    auto prop_name = prop_id.full_name();
    auto prop = bstate_->load_code_var(prop_name);
    property_assert(prop, "[xtop_property_access_control::src_code]property not exist, prop_name: " + prop_name);

    return { std::begin(prop->query()), std::end(prop->query()) };
}

xbyte_buffer_t xtop_property_access_control::bin_code(xproperty_identifier_t const & prop_id) const {
    std::error_code ec;
    auto r = bin_code(prop_id, ec);
    top::error::throw_error(ec);
    return r;
}

void xtop_property_access_control::deploy_bin_code(xproperty_identifier_t const & prop_id, xbyte_buffer_t bin_code, std::error_code & ec) {
    assert(!ec);
    if (bstate_->find_property(prop_id.full_name())) {
        ec = error::xerrc_t::property_already_exist;
        return;
    }

    auto src_prop = bstate_->new_code_var(prop_id.full_name(), canvas_.get());
    if (!src_prop->deploy_code({ std::begin(bin_code), std::end(bin_code) }, canvas_.get())) {
        ec = error::xerrc_t::deploy_code_failed;
    }
}

void xtop_property_access_control::deploy_bin_code(xproperty_identifier_t const & prop_id, xbyte_buffer_t bin_code) {
    std::error_code ec;
    deploy_bin_code(prop_id, std::move(bin_code), ec);
    top::error::throw_error(ec);
}

/**
 *
 * @brief  context apis
 *
 */
common::xaccount_address_t xtop_property_access_control::address() const {
    return common::xaccount_address_t{bstate_->get_address()};
}

uint64_t xtop_property_access_control::blockchain_height() const {
    return bstate_->get_block_height();
}

bool xtop_property_access_control::property_exist(common::xaccount_address_t const & user, xproperty_identifier_t const & property_id, std::error_code & ec) const {
    assert(!ec);
    auto prop_name = property_id.full_name();
    if (read_permitted(user, property_id)) {
        return bstate_->find_property(prop_name);
    } else {
        ec = error::xerrc_t::property_permission_not_allowed;
        return false;
    }
}

bool xtop_property_access_control::property_exist(common::xaccount_address_t const & user, xproperty_identifier_t const & property_id) const {
    std::error_code ec;
    auto ret = property_exist(user, property_id, ec);
    top::error::throw_error(ec);
    return ret;
}

bool xtop_property_access_control::system_property(xproperty_identifier_t const & property_id) const {
    return false;
}

/**
 *
 * @brief util apis
 *
 */
void xtop_property_access_control::property_assert(bool condition, std::string const& exception_msg, error::xerrc_t error_enum) const {
    xtop_property_utl::property_assert(condition, error_enum, exception_msg);
}


void xtop_property_access_control::load_access_control_data(std::string const & json) {

}
void xtop_property_access_control::load_access_control_data(xproperty_access_control_data_t const& data) {
    ac_data_ = data;
}


bool xtop_property_access_control::read_permitted(common::xaccount_address_t const & reader, xproperty_identifier_t const & property_id) const noexcept {
    return true;
}

bool xtop_property_access_control::read_permitted(common::xaccount_address_t const & reader, std::string const & property_full_name) const noexcept {
    return true;
}

bool xtop_property_access_control::write_permitted(common::xaccount_address_t const & writer, xproperty_identifier_t const & property_id) const noexcept {
    return true;
}

NS_END3
