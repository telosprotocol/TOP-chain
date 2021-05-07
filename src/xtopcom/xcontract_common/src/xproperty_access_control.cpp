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

std::string xtop_property_utl::property_category_str(xproperty_category_t const& category) {
    switch (category) {
    case xproperty_category_t::sys_kernel:
        return "@";
    case xproperty_category_t::sys_business:
        return "#";
    case xproperty_category_t::user:
        return "$";

    default:
        assert(0); //todo current cannot go here
        return "";
    }
}


xtop_property_access_control::xtop_property_access_control(top::observer_ptr<top::base::xvbstate_t> bstate,  xproperty_access_control_data_t ac_data): bstate_(bstate), ac_data_(ac_data){}

/**
 *
 * @brief  map apis
 *
 */
template<>
void xtop_property_access_control::map_prop_create<std::string, std::string>(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id) {
    auto prop_name = prop_id.full_name();
    if (write_permitted(user, prop_id)) {
        property_assert(!prop_exist(prop_name), "[xtop_property_access_control::map_prop_create]property already exist，prop_name: " + prop_name, error::xerrc_t::property_already_exist);
        auto prop = bstate_->new_string_map_var(prop_name);
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
        property_assert(prop->insert(prop_key, prop_value), "[xtop_property_access_control::map_prop_add]property insert error, prop_name: " + prop_name + " , prop_key: " + prop_key + ", prop_value: " + prop_value);
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

        property_assert(prop->insert(prop_key, prop_value), "[xtop_property_access_control::map_prop_update]property update key error, prop_name: " + prop_name + " , prop_key: " + prop_key + ", prop_value: " + prop_value);
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
        property_assert(prop->reset(prop_value), "[xtop_property_access_control::map_prop_update]property update error, prop_name: " + prop_name);
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

        property_assert(prop->erase(prop_key), "[ xtop_property_access_control::map_prop_erase]property erase key error, prop_name: " + prop_name + ", prop_key: " + prop_key);
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
 * @brief queue apis
 *
 */
template<>
void xtop_property_access_control::QUEUE_PROP_CREATE<std::string>(std::string const& prop_name) {
    property_assert(!QUEUE_PROP_EXIST(prop_name), "[QUEUE_PROP_CREATE]queue property already exist, prop_name: " + prop_name);

    auto prop = bstate_->new_string_deque_var(prop_name);
    property_assert(prop, "[QUEUE_PROP_CREATE]queue property create error, prop_name: " + prop_name);
}

bool xtop_property_access_control::QUEUE_PROP_EXIST(std::string const& prop_name) {
    return bstate_->find_property(prop_name);
}

template<>
void xtop_property_access_control::QUEUE_PROP_PUSHBACK<std::string>(std::string const& prop_name, std::string const& prop_value) {
    auto prop = bstate_->load_string_deque_var(prop_name);
    property_assert(prop, "[QUEUE_PROP_PUSHBACK]queue property not exist, prop_name: " + prop_name + ", prop_value: " + prop_value);

    property_assert(prop->push_back(prop_value), "[QUEUE_PROP_PUSHBACK]queue property pushback error, prop_name: " + prop_name + ", prop_value: " + prop_value);
}

template<>
void xtop_property_access_control::QUEUE_PROP_UPDATE<std::string>(std::string const& prop_name, std::uint32_t pos, std::string const& prop_value) {
    auto prop = bstate_->load_string_deque_var(prop_name);
    property_assert(prop, "[QUEUE_PROP_UPDATE]queue property not exist, prop_name: " + prop_name + ", pos: " + std::to_string(pos) + ", prop_value: " + prop_value);

    property_assert(prop->update(pos, prop_value), "[QUEUE_PROP_UPDATE]queue property update pos error, prop_name: " + prop_name + ", pos: " + std::to_string(pos) + ", prop_value: " + prop_value);
}

template<>
void xtop_property_access_control::QUEUE_PROP_UPDATE<std::string>(std::string const& prop_name, std::deque<std::string> const& prop_value) {
    auto prop = bstate_->load_string_deque_var(prop_name);
    property_assert(prop, "[QUEUE_PROP_UPDATE]queue property not exist, prop_name: " + prop_name);

    property_assert(prop->reset(prop_value), "[QUEUE_PROP_UPDATE]queue property update error, prop_name: " + prop_name);
}

// void xtop_property_access_control::QUEUE_PROP_ERASE(std::string const& prop_name, std::uint32_t pos) {
//     auto prop = bstate_->load_string_deque_var(prop_name);
//     property_assert(prop, "queue property not exist");
//     property_assert(pos < prop->query().size(), "queue property erase pos invalid");

//     property_assert(prop->erase(pos), "queue property erase pos error");
// }

template<>
void xtop_property_access_control::QUEUE_PROP_CLEAR<std::string>(std::string const& prop_name) {
    auto prop = bstate_->load_string_deque_var(prop_name);
    property_assert(prop, "[QUEUE_PROP_CLEAR]queue property not exist, prop_name: " + prop_name);

    xassert(false); // TODO(jimmy)
//    property_assert(prop->reset(), "[QUEUE_PROP_CLEAR]queue property reset error, prop_name: " + prop_name);
}

template<>
std::string xtop_property_access_control::QUEUE_PROP_QUERY<std::string>(std::string const& prop_name, std::uint32_t pos) {
    auto prop = bstate_->load_string_deque_var(prop_name);

    property_assert(prop, "[QUEUE_PROP_QUERY]queue property not exist, prop_name: " + prop_name + ", pos: " + std::to_string(pos));
    property_assert(pos < prop->query().size(), "[QUEUE_PROP_QUERY]queue property query pos invalid, prop_name: " + prop_name + ", pos: " + std::to_string(pos));

    return prop->query(pos);
}

template<>
std::deque<std::string> xtop_property_access_control::QUEUE_PROP_QUERY<std::string>(std::string const& prop_name) {
    auto prop = bstate_->load_string_deque_var(prop_name);
    property_assert(prop, "[QUEUE_PROP_QUERY]queue property not exist, prop_name: " + prop_name);

    return prop->query();
}


/**
 *
 * @brief str apis
 *
 */
void xtop_property_access_control::STR_PROP_CREATE(std::string const& prop_name) {
    property_assert(!STR_PROP_EXIST(prop_name), "[STR_PROP_CREATE]str property already exist, prop_name: " + prop_name);

    auto prop = bstate_->new_string_var(prop_name);

    property_assert(prop, "[STR_PROP_CREATE]str property create error, prop_name: " + prop_name);
}

bool xtop_property_access_control::STR_PROP_EXIST(std::string const& prop_name) {
    return bstate_->find_property(prop_name);
}


void xtop_property_access_control::STR_PROP_UPDATE(std::string const& prop_name, std::string const& prop_value) {
    auto prop = bstate_->load_string_var(prop_name);
    property_assert(prop, "[STR_PROP_UPDATE]str property not exist, prop_name: " + prop_name + ", prop_value: " + prop_value);

    property_assert(prop->reset(prop_value), "[STR_PROP_UPDATE]str property update error, prop_name: " + prop_name + ", prop_value: " + prop_value);
}


void xtop_property_access_control::STR_PROP_CLEAR(std::string const& prop_name) {
    auto prop = bstate_->load_string_var(prop_name);
    property_assert(prop, "[STR_PROP_CLEAR]str property not exist, prop_name: " + prop_name);

    xassert(false); // TODO(jimmy)
    //property_assert(prop->reset(), "[STR_PROP_CLEAR]str property clear error, prop_name: " + prop_name);
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
        property_assert(!prop_exist(prop_name), "[xtop_property_access_control::token_prop_create]property already exist, prop_name: " + prop_name, error::xerrc_t::property_already_exist);
        auto prop = bstate_->new_token_var(prop_name);

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
        return prop->withdraw((base::vtoken_t)amount);
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
        return prop->deposit((base::vtoken_t)amount);
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
        property_assert(!prop_exist(prop_name), "[xtop_property_access_control::code_prop_create]property already exist, prop_name: " + prop_name, error::xerrc_t::property_already_exist);
        auto prop = bstate_->new_code_var(prop_name);
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
        return prop->deploy_code(code);

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

    auto src_prop = bstate_->new_code_var(prop_id.full_name());
    if (!src_prop->deploy_code(src_code)) {
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

    auto src_prop = bstate_->new_code_var(prop_id.full_name());
    if (!src_prop->deploy_code({ std::begin(bin_code), std::end(bin_code) })) {
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
    return common::xaccount_address_t{bstate_->get_account_addr()};
}

uint64_t xtop_property_access_control::blockchain_height() {
    return bstate_->get_block_height();
}


bool xtop_property_access_control::prop_exist(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id) {
    auto prop_name = prop_id.full_name();
    if (read_permitted(user, prop_id)) {
        return bstate_->find_property(prop_name);
    } else {
        std::error_code ec{ error::xerrc_t::property_permission_not_allowed };
        top::error::throw_error(ec, "[xtop_property_api::map_prop_update]permission denied");
        return {};
    }
}

/**
 *
 * @brief util apis
 *
 */
void xtop_property_access_control::property_assert(bool condition, std::string const& exception_msg, error::xerrc_t error_enum) const {
    xtop_property_utl::property_assert(condition, error_enum, exception_msg);
}

bool xtop_property_access_control::prop_exist(std::string const& prop_name) {
    return bstate_->find_property(prop_name);
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
