// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>

#include "xbase/xcontext.h"
#include "xbase/xmem.h"
#include "xchain_timer/xchain_timer_face.h"
#include "xcommon/xaddress.h"
#include "xcommon/xlogic_time.h"
#include "xvm/xcontract_helper.h"
#include "xvm/xvm_context.h"

NS_BEG3(top, xvm, xcontract)

// using namespace top::store;

/**
 * @brief the enum definition for contract property type
 *
 */
enum class enum_type_t {
    map,
    list,
    string
};

#define BLOCK_DEFUALT_KEY   "_default"

/**
 * @brief  the contract base class
 *
 */
class xcontract_base {
public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xcontract_base);
    XDECLARE_DEFAULTED_VIRTULA_DESTRUCTOR(xcontract_base);

protected:

    explicit
    xcontract_base(common::xnetwork_id_t const & network_id);

public:
    /**
     * @brief  clone the contract base object
     *
     * @return xcontract_base*  the contract base object
     */
    virtual xcontract_base* clone() = 0;

    /**
     * @brief  setup the contract on chain
     *
     */
    virtual void setup() {}     ///< per chain

    /**
     * @brief shutdown the contract on chain
     *
     */
    virtual void shutdown() {}  ///< once

    /**
     * @brief process event to the contract
     *
     */
    virtual void on_event() {}

    /**
     * @brief execute the contract
     *
     * @param vm_ctx  the virtual machine context
     */
    virtual void exec(xvm_context* vm_ctx) = 0;
    // virtual void init() = 0;

    /**
     * @brief Set the contract helper object
     *
     * @param contract_helper  the contract helper object
     */
    virtual void set_contract_helper(std::shared_ptr<xcontract_helper> contract_helper) final;

    /**
     * @brief Get the exec account object
     *
     * @return std::string  the account addr string
     */
    virtual std::string GET_EXEC_ACCOUNT() const final;

    /**
     * @brief Get the balance object
     *
     * @return uint64_t  the balance of the contract account
     */
    virtual uint64_t GET_BALANCE() const final;

    /**
     * @brief Get the transaction object
     *
     * @return data::xtransaction_ptr_t  the according transaction object in contract
     */
    virtual data::xtransaction_ptr_t GET_TRANSACTION() const;

    /**
     * @brief get current logic time
     *
     * @return common::xlogic_time_t  the login time
     */
    virtual common::xlogic_time_t TIME() const;

    /**
     * @brief create property for contract
     *
     * @param type  the property type
     * @param prop_key  the property key
     */
    virtual void CREATE(enum_type_t type, const std::string& prop_key);

    /**
     * @brief query the contract property
     *
     * @param type  the property type
     * @param prop_key  the property key
     * @param key the property real type's content key
     * @param addr  the according addr the property belong to
     * @return std::string  the content
     */
    virtual std::string QUERY(enum_type_t type, const std::string& prop_key, const std::string& key="", const std::string& addr="") const;

    /**
     * @brief read the contract property
     *
     * @param type  the property type
     * @param prop_key  the property key
     * @param key  the property real type's content key
     * @return std::string  the content
     */
    virtual std::string READ(enum_type_t type, const std::string& prop_key, const std::string& key="");

    /**
     * @brief write the contract property
     *
     * @param type  the property type
     * @param prop_key  the property key
     * @param value  the content to write to
     * @param key  the property real type's content key
     */
    virtual void WRITE(enum_type_t type, const std::string& prop_key, const std::string& value, const std::string& key="");

    /**
     * @brief remove specific content of the contract property
     *
     * @param type  the property type
     * @param prop_key  the property key
     * @param key  the property real type's content key
     */
    virtual void REMOVE(enum_type_t type, const std::string& prop_key, const std::string& key="");

    /**
     * @brief do call function in contract to generate according transaction
     *
     * @param target_addr  the target addr for the transaction
     * @param action  the action of the transaction
     * @param params  the action params
     */
    virtual void CALL(common::xaccount_address_t const & target_addr, const std::string& action, const std::string& params);

     /**
     * @brief do async call function in contract to generate according transaction
     *
     * @param target_addr  the target addr for the transaction
     * @param action  the action of the transaction
     * @param params  the action params
     */
    virtual void ASYNC_CALL(const std::string& target_addr, const std::string& action, const std::string& params);

    /**
     * @brief transfer balance to target addr
     *
     * @param target_addr  the target addr to transfer
     * @param amount  the amount to transfer
     */
    virtual void TRANSFER(const std::string& target_addr, uint64_t amount);

    /**
     * @brief check the addr whether exist
     *
     * @param addr  the addr to check
     * @return true  the addr exist
     * @return false  the addr not exist
     */
    virtual bool EXISTS(const std::string& addr);

    /**
     * @brief the size of the property
     *
     * @param type  the property type
     * @param prop_key  the property key
     * @param addr  the addr the property belong to
     * @return int32_t  the size of the property content
     */
    virtual int32_t SIZE(enum_type_t type, const std::string& prop_key, const std::string& addr="");

    /**
     * @brief clear the whole property
     *
     * @param type  the property type
     * @param prop_key  the property key
     */
    virtual void CLEAR(enum_type_t type, const std::string& prop_key);

    /**
     * @brief calculate the addr of the contract
     *
     * @param contract_name  the contract name(addr)
     * @param table_id  the table id the contract belong to
     * @return std::string  the calculated addr
     */
    virtual std::string CALC_CONTRACT_ADDRESS(const std::string& contract_name, const uint32_t& table_id);

    /**
     * @brief extract the table id from the addr
     *
     * @param addr  the addr to extract the table id
     * @param table_id  the extracted table id to store to
     * @return true  the table id extracts successfully
     * @return false  the table id extracts failed
     */
    virtual bool EXTRACT_TABLE_ID(common::xaccount_address_t const & addr, uint32_t& table_id) const;

    /**
     * @brief the source address to call the contract
     *
     * @return std::string  the source address
     */
    virtual std::string SOURCE_ADDRESS();

    /**
     * @brief the self addr of the contract
     *
     * @return common::xaccount_address_t const&  the self addr returned
     */
    virtual common::xaccount_address_t const & SELF_ADDRESS() const noexcept;

    /**
     * @brief add token
     *
     * @return void
     */
    void TOP_TOKEN_INCREASE(const uint64_t amount);

    /**
     * @brief sub token
     *
     * @return void
     */
    void TOP_TOKEN_DECREASE(const uint64_t amount);

    /**
     * @brief create the string type property
     *
     * @param key  the property key
     */
    virtual void STRING_CREATE(const std::string& key) final;

    /**
     * @brief set the string type property
     *
     * @param key  the property key
     * @param value  the property value
     */
    virtual void STRING_SET(const std::string& key, const std::string& value) final;

    /**
     * @brief get the string type property
     *
     * @param key  the property key
     * @return std::string  the returned string property
     */
    virtual std::string STRING_GET(const std::string& key) const final;

    /**
     * @brief get the string type property(not throw exception )
     *
     * @param key  the property key
     * @param addr  the addr the property belong to
     * @return std::string  the returned string property
     */
    virtual std::string STRING_GET2(const std::string& key, const std::string& addr = "") const final;

    /**
     * @brief whether the string property exist
     *
     * @param key  the property key
     * @return true  the property exist
     * @return false  the property not exist
     */
    virtual bool STRING_EXIST(const std::string& key) final;

    /**
     * @brief create list type property
     *
     * @param key  the list property key
     */
    virtual void LIST_CREATE(const std::string& key) final;

    /**
     * @brief push content back to the list property
     *
     * @param key  the key of the list property
     * @param value  the value to push back
     */
    virtual void LIST_PUSH_BACK(const std::string& key, const std::string& value) final;

    /**
     * @brief push content front to the list property
     *
     * @param key  the key of the list property
     * @param value  the value to push front
     */
    virtual void LIST_PUSH_FRONT(const std::string& key, const std::string& value) final;

    /**
     * @brief pop content back from the list property
     *
     * @param key  the key of the list property
     * @param value  the value to pop back
     */
    virtual void LIST_POP_BACK(const std::string& key, std::string& value) final;

    /**
     * @brief pop content front from the list property
     *
     * @param key  the key of the list property
     * @param value  the value to pop front
     */
    virtual void LIST_POP_FRONT(const std::string& key, std::string& value) final;

    /**
     * @brief get the size of the list property
     *
     * @param key  the key of the list property
     * @return int32_t  the size of the list property
     */
    virtual int32_t LIST_SIZE(const std::string& key) final;

    /**
     * @brief get the whole list of the list property
     *
     * @param key  the key of the list property
     * @param addr  the addr the list property belong to
     * @return std::vector<std::string>  the returned whole list
     */
    virtual std::vector<std::string> LIST_GET_ALL(const std::string& key, const string& addr = "") final;

    /**
     * @brief judge whether the list exist
     *
     * @param key  the list property key
     * @return true  the list property exists
     * @return false  the list property not exist
     */
    virtual bool LIST_EXIST(const std::string& key) final;

    /**
     * @brief clear the list property
     *
     * @param key  the list property key
     */
    virtual void LIST_CLEAR(const std::string& key) final;

    /**
     * @brief create map type property
     *
     * @param key  the key of the map type property
     */
    virtual void MAP_CREATE(const std::string& key) final;

    /**
     * @brief get map type property
     *
     * @param key  the key of the map type property
     * @param field  the specific map content key
     * @return std::string  the returned map property content
     */
    virtual std::string MAP_GET(const std::string& key, const std::string& field) const final;

    /**
     * @brief get map type property(not throw exception)
     *
     * @param key  the key of the map type property
     * @param field  the specific map content key
     * @param value  the returned map property
     * @param addr  the addr the map property belong to
     * @return int32_t
     */
    virtual int32_t MAP_GET2(const string& key, const string& field, string& value, const std::string& addr="") const final;

    /**
     * @brief set map type property
     *
     * @param key  the map type property key
     * @param field  the specific map content key
     * @param value  the value to set to
     */
    virtual void MAP_SET(const std::string& key, const std::string& field, const std::string & value) final;

    /**
     * @brief remove specific map property content
     *
     * @param key  the map property key
     * @param field  the specific map content key
     */
    virtual void MAP_REMOVE(const std::string& key, const std::string& field) final;

    /**
     * @brief clear the whole map property
     *
     * @param key  the map property key
     */
    virtual void MAP_CLEAR(const std::string& key);

    /**
     * @brief get the whole map property
     *
     * @param key  the map property key
     * @param map  the whole map property to store to
     * @param addr  the addr the map property belong to
     */
    virtual void MAP_COPY_GET(const std::string& key, std::map<std::string, std::string> & map, const std::string& addr = "") const;

    /**
     * @brief the size of the map property
     *
     * @param key  the map property key
     * @return int32_t  the returned map property size
     */
    virtual int32_t MAP_SIZE(const std::string& key) final;

    /**
     * @brief check whether a specific content field exists
     *
     * @param key  the map key to check
     * @param field  the specific map content key
     * @return true  the specific content exist
     * @return false  the specific content not exist
     */
    virtual bool MAP_FIELD_EXIST(const std::string& key, const std::string& field) const final;

    /**
     * @brief Get the map property object by height
     *
     * @param key  the map property key
     * @param value  the map property to stored to
     * @param height  the specific height
     * @param addr  the addr the map property belong to
     */
    virtual void GET_MAP_PROPERTY(const std::string& key, std::map<std::string, std::string>& value, uint64_t height, const std::string& addr) const final;

    virtual bool MAP_PROPERTY_EXIST(const std::string& key) const final;

    virtual void GET_STRING_PROPERTY(const std::string& key, std::string& value, uint64_t height, const std::string& addr) const final;

    /**
     * @brief  generate a transaction
     *
     * @param target_addr  the target addr of the transaction
     * @param func_name  the contract function name to call
     * @param func_param  the contract fruntion param
     */
    virtual void GENERATE_TX(common::xaccount_address_t const & target_addr, const std::string& func_name, const std::string& func_param) final;

      /**
     * @brief Generate a log for the transaction
     *
     * @param func_sign  The log content description can be customized
     * @param data  log information
     */
    virtual void EVENT(const std::string& func_sign , const std::string& data);

      /**
     * @brief Generate a log for the transaction
     *
     * @param indexed_form transfer source  address
     * @param indexed_to  transfer target  information
     * @param data  log information
     */
    virtual void EVENT_TRANSFER(const std::string& indexed_form ,const std::string& indexed_to , const uint64_t& data);
    
    /**
     * @brief Get the block by height object
     *
     * @param owner the block owner
     * @param height  the block height
     * @return base::xauto_ptr<xblock_t>
     */
    base::xauto_ptr<data::xblock_t>
    get_block_by_height(const std::string & owner, uint64_t height) const;

    /**
     * @brief Get the blockchain block lastest height
     *
     * @param owner  the block owner
     * @return std::uint64_t the returned block height
     */
    std::uint64_t
    get_blockchain_height(const std::string & owner) const;

    /**
     * @brief get next fullblock
     *
     * @param owner the block owner
     * @param cur_full_height current fullblock height
     * @return the full block
     */
    base::xauto_ptr<data::xblock_t>
    get_next_fullblock(std::string const & owner, uint64_t const cur_full_height) const;


    /**
     * @brief get network id
     *
     * @return common::xnetwork_id_t const&
     */
    common::xnetwork_id_t const &
    network_id() const noexcept;

    /**
     * @brief Get the account from xip object
     *
     * @param target_node  the target xip
     * @param target_addr  the target addr from the xip
     * @return int32_t
     */
    int32_t
    get_account_from_xip(const xvip2_t & target_node, std::string &target_addr);

protected:
    shared_ptr<xcontract_helper> m_contract_helper;  /// the contract helper
    common::xnetwork_id_t m_network_id;
};

NS_END3
