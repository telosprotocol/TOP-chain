// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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

#include "xvledger/xvstate.h"
#include "xbase/xvmethod.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xbasic/xenable_to_string.h"
#include "xbasic/xserializable_based_on.h"
#include "xbasic/xerror/xthrow_error.h"
#include "xcommon/xaddress.h"
#include "xcontract_common/xerror/xerror.h"
#include "xcontract_common/xproperties/xaccess_control_data.h"
#include "xcontract_common/xproperties/xproperty_identifier.h"
#include "xvledger/xvcanvas.h"
#include "xvledger/xvstate.h"

#include <map>
#include <memory>
#include <string>
#include <type_traits>

NS_BEG3(top, contract_common, properties)


class xtop_property_utl {
public:
    static void property_assert(bool condition, error::xerrc_t error_enum, std::string const& exception_msg);
};
using xproperty_utl_t = xtop_property_utl;

class xtop_property_access_control {
private:
    top::observer_ptr<top::base::xvbstate_t> bstate_;
    top::xobject_ptr_t<top::base::xvcanvas_t> canvas_;
    xproperty_access_control_data_t ac_data_;

public:
    xtop_property_access_control(xtop_property_access_control const&) = delete;
    xtop_property_access_control& operator=(xtop_property_access_control const&) = delete;
    xtop_property_access_control(xtop_property_access_control&&) = default;
    xtop_property_access_control& operator=(xtop_property_access_control&&) = default;
    ~xtop_property_access_control() =  default;

    explicit xtop_property_access_control(top::observer_ptr<top::base::xvbstate_t> bstate, xproperty_access_control_data_t ac_data);

    /***********************************************************************/
    /*****************        attribute related apis       *****************/
    /***********************************************************************/
    // map apis
    /**
     * @brief create a map property
     *
     * @param user the user addr
     * @param prop_id the property identifier
     */
    template<typename KEYT, typename VALUET, typename = typename std::enable_if<std::is_same<KEYT, std::string>::value &&
                                                                            (
                                                                                std::is_same<VALUET, std::string>::value ||
                                                                                std::is_same<VALUET, std::int8_t>::value ||
                                                                                std::is_same<VALUET, std::int16_t>::value ||
                                                                                std::is_same<VALUET, std::int32_t>::value ||
                                                                                std::is_same<VALUET, std::int64_t>::value ||
                                                                                std::is_same<VALUET, std::uint64_t>::value
                                                                            )>::type>
    void map_prop_create(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id);

    /**
     * @brief add a key of the map property
     *
     * @param user the user addr
     * @param prop_id  the property identifier
     * @param prop_key the key of the map property
     * @param prop_value the value of the specific key
     */
    template<typename KEYT, typename VALUET, typename = typename std::enable_if<std::is_same<KEYT, std::string>::value &&
                                                                            (
                                                                                std::is_same<VALUET, std::string>::value ||
                                                                                std::is_same<VALUET, std::int8_t>::value ||
                                                                                std::is_same<VALUET, std::int16_t>::value ||
                                                                                std::is_same<VALUET, std::int32_t>::value ||
                                                                                std::is_same<VALUET, std::int64_t>::value ||
                                                                                std::is_same<VALUET, std::uint64_t>::value
                                                                            )>::type>
    void map_prop_add(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id, KEYT const& prop_key, VALUET const& prop_value);
    /**
     * @brief update a specific key of the map property
     *
     * @param user the user addr
     * @param prop_id the property identifier
     * @param prop_key the key of the map property
     * @param prop_value the value of the specific key
     */
    template<typename KEYT, typename VALUET, typename = typename std::enable_if<std::is_same<KEYT, std::string>::value &&
                                                                            (
                                                                                std::is_same<VALUET, std::string>::value ||
                                                                                std::is_same<VALUET, std::int8_t>::value ||
                                                                                std::is_same<VALUET, std::int16_t>::value ||
                                                                                std::is_same<VALUET, std::int32_t>::value ||
                                                                                std::is_same<VALUET, std::int64_t>::value ||
                                                                                std::is_same<VALUET, std::uint64_t>::value
                                                                            )>::type>
    void map_prop_update(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id, KEYT const& prop_key, VALUET const& prop_value);
    /**
     * @brief update the entire map property
     *
     * @param user the user addr
     * @param prop_id the property  identifier
     * @param prop_value the value of the map property
     */
    template<typename KEYT, typename VALUET, typename = typename std::enable_if<std::is_same<KEYT, std::string>::value &&
                                                                            (
                                                                                std::is_same<VALUET, std::string>::value ||
                                                                                std::is_same<VALUET, std::int8_t>::value ||
                                                                                std::is_same<VALUET, std::int16_t>::value ||
                                                                                std::is_same<VALUET, std::int32_t>::value ||
                                                                                std::is_same<VALUET, std::int64_t>::value ||
                                                                                std::is_same<VALUET, std::uint64_t>::value
                                                                            )>::type>
    void map_prop_update(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id, std::map<KEYT, VALUET> const& prop_value);

    /**
     * @brief erase the specific key of the map property
     *
     * @param user the user addr
     * @param prop_id the property identifier
     * @param prop_key the specific key
     */
        template<typename KEYT, typename VALUET, typename = typename std::enable_if<std::is_same<KEYT, std::string>::value &&
                                                                            (
                                                                                std::is_same<VALUET, std::string>::value ||
                                                                                std::is_same<VALUET, std::int8_t>::value ||
                                                                                std::is_same<VALUET, std::int16_t>::value ||
                                                                                std::is_same<VALUET, std::int32_t>::value ||
                                                                                std::is_same<VALUET, std::int64_t>::value ||
                                                                                std::is_same<VALUET, std::uint64_t>::value
                                                                            )>::type>
    void map_prop_erase(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id, KEYT const& prop_key);
    // /**
    //  * @brief remove the entire map property
    //  *
    //  * @param prop_name the property name
    //  */
    // virtual void MAP_PROP_REMOVE(std::string const& prop_name);
    /**
     * @brief clear/reset the entire map property
     *
     * @param user the user addr
     * @param prop_id the property identifier
     */
    template<typename KEYT, typename VALUET, typename = typename std::enable_if<std::is_same<KEYT, std::string>::value &&
                                                                            (
                                                                                std::is_same<VALUET, std::string>::value ||
                                                                                std::is_same<VALUET, std::int8_t>::value ||
                                                                                std::is_same<VALUET, std::int16_t>::value ||
                                                                                std::is_same<VALUET, std::int32_t>::value ||
                                                                                std::is_same<VALUET, std::int64_t>::value ||
                                                                                std::is_same<VALUET, std::uint64_t>::value
                                                                            )>::type>
    void map_prop_clear(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id);

    /**
     * @brief query a specific key of the map property
     *
     * @param user the user addr
     * @param prop_id the property identifier
     * @param prop_key the specific key
     * @return std::string  the value of the specific key(if not exist, return empty string)
     */
    template<typename KEYT, typename VALUET, typename = typename std::enable_if<std::is_same<KEYT, std::string>::value &&
                                                                            (
                                                                                std::is_same<VALUET, std::string>::value ||
                                                                                std::is_same<VALUET, std::int8_t>::value ||
                                                                                std::is_same<VALUET, std::int16_t>::value ||
                                                                                std::is_same<VALUET, std::int32_t>::value ||
                                                                                std::is_same<VALUET, std::int64_t>::value ||
                                                                                std::is_same<VALUET, std::uint64_t>::value
                                                                            )>::type>
    std::string map_prop_query(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id, KEYT const& prop_key);
    /**
     * @brief query the entire map property
     *
     * @param user the user addr
     * @param prop_name the property identifier
     * @return std::map<std::string, std::string> the entire map property
     */
    template<typename KEYT, typename VALUET, typename = typename std::enable_if<std::is_same<KEYT, std::string>::value &&
                                                                            (
                                                                                std::is_same<VALUET, std::string>::value ||
                                                                                std::is_same<VALUET, std::int8_t>::value ||
                                                                                std::is_same<VALUET, std::int16_t>::value ||
                                                                                std::is_same<VALUET, std::int32_t>::value ||
                                                                                std::is_same<VALUET, std::int64_t>::value ||
                                                                                std::is_same<VALUET, std::uint64_t>::value
                                                                            )>::type>
    std::map<KEYT, VALUET> map_prop_query(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id);

    /// @brief Get the mapped string type value specified by the string key.
    /// @param reader
    /// @param property_full_name Property full name
    template <typename KeyT, typename ValueT, typename std::enable_if<std::is_same<KeyT, std::string>::value && std::is_same<ValueT, std::string>::value>::type * = nullptr>
    ValueT map_at(common::xaccount_address_t const & reader, std::string const & property_full_name, KeyT const & key, std::error_code & ec) const {
        if (!read_permitted(reader, property_full_name)) {
            ec = error::xerrc_t::property_permission_not_allowed;
            return {};
        }

        if (!bstate_->find_property(property_full_name)) {
            ec = error::xerrc_t::property_not_exist;
            return {};
        }

        auto map = bstate_->load_string_map_var(property_full_name);
        if (!map->find(key)) {
            ec = error::xerrc_t::property_map_key_not_exist;
            return {};
        }

        return map->query(key);
    }

    /// @brief Get the mapped bytes type value specified by the string key.
    /// @param reader
    /// @param property_full_name Property full name
    template <typename KeyT, typename ValueT, typename std::enable_if<std::is_same<KeyT, std::string>::value && std::is_same<ValueT, xbyte_buffer_t>::value>::type * = nullptr>
    ValueT map_at(common::xaccount_address_t const & reader, std::string const & property_full_name, KeyT const & key, std::error_code & ec) const {
        auto str = map_at<KeyT, std::string>(reader, property_full_name, key, ec);
        return {std::begin(str), std::end(str)};
    }

    /// @brief Get the mapped complex type value specified by the string key. The mapped value must be inheriented from xserializable_based_on.
    /// @param reader
    /// @param property_full_name Property full name
    template <typename KeyT,
              typename ValueT,
              typename std::enable_if<std::is_same<KeyT, std::string>::value && std::is_base_of<xserializable_based_on<void>, ValueT>::value>::type * = nullptr>
    ValueT map_at(common::xaccount_address_t const & reader, std::string const & property_full_name, KeyT const & key, std::error_code & ec) const {
        auto bytes = map_at<KeyT, xbyte_buffer_t>(reader, property_full_name, key, ec);

        ValueT ret;
        base::xstream_t stream{base::xcontext_t::instance(), bytes.data(), static_cast<uint32_t>(bytes.size())};
        ret.serialize_from(stream, ec);
        return ret;
    }

    /// @brief Get the mapped string value specified by the key.
    /// @param reader
    /// @param property_full_name Property full name
    template <typename KeyT,
              typename ValueT,
              typename std::enable_if<std::is_base_of<xenable_to_string_t<KeyT>, KeyT>::value && std::is_same<std::string, ValueT>::value>::type * = nullptr>
    ValueT map_at(common::xaccount_address_t const & reader, std::string const & property_full_name, KeyT const & key, std::error_code & ec) const {
        auto const str_key = key.to_string();
        return map_at<std::string, std::string>(reader, property_full_name, str_key, ec);
    }

    /// @brief Get the mapped bytes value specified by the key.
    /// @param reader
    /// @param property_full_name Property full name
    template <typename KeyT, typename ValueT, typename std::enable_if<std::is_base_of<xenable_to_string_t<KeyT>, KeyT>::value && std::is_same<ValueT, xbyte_buffer_t>::value>::type * = nullptr>
    ValueT map_at(common::xaccount_address_t const & reader, std::string const & property_full_name, KeyT const & key, std::error_code & ec) const {
        auto const str_key = key.to_string();
        return map_at<std::string, xbyte_buffer_t>(reader, property_full_name, str_key, ec);
    }

    /// @brief Get the mapped string value specified by the key.
    /// @param reader
    /// @param property_full_name Property full name
    template <typename KeyT,
              typename ValueT,
              typename std::enable_if<std::is_base_of<xenable_to_string_t<KeyT>, KeyT>::value && std::is_base_of<xserializable_based_on<void>, ValueT>::value>::type * = nullptr>
    xbyte_buffer_t map_at(common::xaccount_address_t const & reader, std::string const & property_full_name, KeyT const & key, std::error_code & ec) const {
        auto const str_key = key.to_string();
        auto bytes = map_at<std::string, xbyte_buffer_t>(reader, property_full_name, str_key, ec);

        ValueT ret;
        base::xstream_t stream{base::xcontext_t::instance(), bytes.data(), static_cast<uint32_t>(bytes.size())};
        ret.serialize_from(stream, ec);
        return ret;
    }

    // STRING apis
    /**
     * @brief create a string property
     *
     * @param prop_name the property name
     *
     */
    virtual void STR_PROP_CREATE(std::string const& prop_name);

    /**
     * @brief update the string property
     *
     * @param prop_name the property name
     * @param prop_value the property value
     */
    virtual void STR_PROP_UPDATE(std::string const& prop_name, std::string const& prop_value);

    /**
     * @brief query the string property
     *
     * @param prop_name the property name
     * @return std::string  the property value
     */
    virtual std::string STR_PROP_QUERY(std::string const& prop_name);


    // TOKEN apis
    /**
     * @brief create main token property
     *
     * @param user the user addr
     * @param prop_id the property identifier
     */
    void token_prop_create(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id);

    /**
     * @brief withdraw balance from current env account
     *
     * @param user the user addr
     * @param prop_id the property identifier
     * @param amount the amount of the token
     * @return uint64_t  the amount to withdraw
     */
    uint64_t withdraw(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id, uint64_t amount);

    /**
     * @brief deposit balance to current env account
     *
     * @param amount the amount of the token
     * @param token_prop the type of the token
     * @return uint64_t the amount to deposti
     */
    uint64_t deposit(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id, uint64_t amount);

    /**
     * @brief balance of the current execute environment account address
     *
     * @param user the user addr
     * @param prop_id  the property identifier
     * @return uint64_t
     */
    uint64_t balance(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id);

    // CODE apis
    /**
     * @brief create code property
     *
     * @param prop_name the name of code property
     */
    void code_prop_create(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_ide);

    /**
     * @brief query code property
     * @param prop_name the name of code property
     * @return std::string
     */
    std::string code_prop_query(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id);

    /**
     * @brief set code property
     * @param prop_name the name of code property
     * @return std::string
     */
    bool code_prop_update(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id, std::string const& code);


    std::string src_code(xproperty_identifier_t const & prop_id, std::error_code & ec) const;
    std::string src_code(xproperty_identifier_t const & prop_id) const;

    void deploy_src_code(xproperty_identifier_t const & prop_id, std::string src_code, std::error_code & ec);
    void deploy_src_code(xproperty_identifier_t const & prop_id, std::string src_code);

    xbyte_buffer_t bin_code(xproperty_identifier_t const & prop_id, std::error_code & ec) const;
    xbyte_buffer_t bin_code(xproperty_identifier_t const & prop_id) const;

    void deploy_bin_code(xproperty_identifier_t const & prop_id, xbyte_buffer_t bin_code, std::error_code & ec);
    void deploy_bin_code(xproperty_identifier_t const & prop_id, xbyte_buffer_t bin_code);

    bool property_exist(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id, std::error_code & ec) const;
    bool property_exist(common::xaccount_address_t const & user, xproperty_identifier_t const & prop_id) const;

    bool system_property(xproperty_identifier_t const & property_id) const;

public:
    /***********************************************************************/
    /*****************               utl apis              *****************/
    /***********************************************************************/
    void property_assert(bool condition,  std::string const& exception_msg, error::xerrc_t error_enum = error::xerrc_t::property_internal_error) const;

public:
    /***********************************************************************/
    /*****************        context related apis         *****************/
    /***********************************************************************/
    /**
     * @brief  get current environment account addr
     *
     * @return common::xaccount_address_t
     */
    common::xaccount_address_t address() const;

    uint64_t blockchain_height() const;


    virtual void load_access_control_data(std::string const & json);
    virtual void load_access_control_data(xproperty_access_control_data_t const& data);

    virtual bool read_permitted(common::xaccount_address_t const & reader, xproperty_identifier_t const & property_id) const noexcept;
    virtual bool write_permitted(common::xaccount_address_t const & writer, xproperty_identifier_t const & property_id) const noexcept;

    bool read_permitted(common::xaccount_address_t const & reader, std::string const & property_full_name) const noexcept;
};
using xproperty_access_control_t = xtop_property_access_control;

NS_END3
