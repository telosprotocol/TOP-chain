// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xcommon/xaccount_address.h"
#include "xcommon/xtoken_metadata.h"
#include "xevm_common/common.h"
#include "xvledger/xvstate.h"
#include "xvledger/xvstate.h"

NS_BEG2(top, data)

// bstate with canvas
class xbstate_ctx_t {
public:
    xbstate_ctx_t(base::xvbstate_t* bstate, bool readonly);
    virtual ~xbstate_ctx_t();

public: // common APIs for state basic info
    const std::string & get_account()const {return m_bstate->get_account();}
    const std::string & get_address()const {return m_bstate->get_account();}
    uint64_t            get_block_height()const {return m_bstate->get_block_height();}
    uint64_t            get_chain_height() const {return m_bstate->get_block_height();}
    uint64_t            get_block_viewid() const {return m_bstate->get_block_viewid();}
    inline uint64_t     get_last_full_unit_height() const {return m_bstate->get_last_fullblock_height();}
    inline const std::string & get_last_full_unit_hash() const {return m_bstate->get_last_fullblock_hash();}

    const xobject_ptr_t<base::xvbstate_t> & get_bstate() const { return m_bstate;}
    const xobject_ptr_t<base::xvbstate_t> & get_origin_bstate() const { return m_snapshot_origin_bstate;}
    const xobject_ptr_t<base::xvcanvas_t> & get_canvas() const {return m_canvas;}
    std::string         dump() const;

    common::xaccount_address_t account_address() const;

public:
    bool                do_rollback();
    size_t              do_snapshot();
    bool                is_state_readonly() const;
    bool                is_state_dirty() const; // dirty from snapshot
    bool                is_state_changed() const;  // change from origin
    std::string         take_snapshot();
    std::string         take_binlog();
    size_t              get_canvas_records_size() const;

public: // APIs for property operation with error code return
    int32_t string_create(const std::string& key);
    int32_t string_set(const std::string& key, const std::string& value);
    int32_t string_get(const std::string& key, std::string& value) const;

    int32_t list_create(const std::string& key);
    int32_t list_push_back(const std::string& key, const std::string& value);
    int32_t list_push_front(const std::string& key, const std::string& value);
    int32_t list_pop_back(const std::string& key, std::string& value);
    int32_t list_pop_front(const std::string& key, std::string& value);
    int32_t list_clear(const std::string &key);
    int32_t list_get(const std::string& key, const uint32_t index, std::string & value);
    int32_t list_size(const std::string& key, int32_t& size);
    int32_t list_get_all(const std::string &key, std::vector<std::string> &values);
    int32_t list_copy_get(const std::string &prop_name, std::deque<std::string> & deque);

    int32_t map_create(const std::string& key);
    int32_t map_get(const std::string & key, const std::string & field, std::string & value);
    int32_t map_set(const std::string & key, const std::string & field, const std::string & value);
    int32_t map_remove(const std::string & key, const std::string & field);
    int32_t map_clear(const std::string & key);
    int32_t map_size(const std::string & key, int32_t& size) const;
    int32_t map_copy_get(const std::string & key, std::map<std::string, std::string> & map) const;

    uint64_t    token_balance(const std::string& key);
    int32_t     token_deposit(const std::string& key, base::vtoken_t add_token);
    int32_t     token_withdraw(const std::string& key, base::vtoken_t sub_token);

    int32_t     uint64_add(const std::string& key, uint64_t change);
    int32_t     uint64_sub(const std::string& key, uint64_t change);
    int32_t     uint64_set(const std::string& key, uint64_t value);

public:// APIs for property operation with property value return
    uint64_t            token_get(const std::string& prop) const;
    uint64_t            uint64_property_get(const std::string& prop) const;
    std::string         map_get(const std::string & prop, const std::string & field) const;
    std::map<std::string, std::string>  map_get(const std::string & prop) const;
    std::string         string_get(const std::string & prop) const;

    base::xauto_ptr<base::xmapvar_t<std::string>> load_tep_token_for_write();
    evm_common::u256 tep_token_balance(common::xtoken_id_t const token_id) const;
    top::xbytes_t tep_token_balance_bytes(common::xtoken_id_t const token_id) const;
    int32_t tep_token_deposit(common::xtoken_id_t const token_id, evm_common::u256 add_token);
    int32_t tep_token_withdraw(common::xtoken_id_t const token_id, evm_common::u256 sub_token);
    int32_t set_tep_balance(common::xtoken_id_t const token_id, evm_common::u256 new_balance);
    int32_t set_tep_balance_bytes(common::xtoken_id_t const token_id, const top::xbytes_t & new_balance);

private:
    evm_common::u256 tep_token_balance(const std::string & token_name) const;
    top::xbytes_t tep_token_balance_bytes(const std::string & token_name) const;
    int32_t tep_token_deposit(const std::string & token_name, evm_common::u256 add_token);
    int32_t tep_token_withdraw(const std::string & token_name, evm_common::u256 sub_token);
    int32_t set_tep_balance(const std::string & token_name, evm_common::u256 new_balance);
    int32_t set_tep_balance_bytes(const std::string & token_name, const top::xbytes_t & new_balance);

    int32_t                                 check_create_property(const std::string& key);
    base::xauto_ptr<base::xstringvar_t>             load_string_for_write(const std::string & key);
    base::xauto_ptr<base::xdequevar_t<std::string>> load_deque_for_write(const std::string & key);
    base::xauto_ptr<base::xmapvar_t<std::string>>   load_map_for_write(const std::string & key);
    base::xauto_ptr<base::xvintvar_t<uint64_t>>     load_uin64_for_write(const std::string & key);
    base::xauto_ptr<base::xtokenvar_t>              load_token_for_write(const std::string & key);

protected:
    xobject_ptr_t<base::xvbstate_t>     m_bstate{nullptr};
    xobject_ptr_t<base::xvcanvas_t> m_canvas{nullptr};

private:
    xobject_ptr_t<base::xvbstate_t>     m_snapshot_origin_bstate{nullptr};
    size_t                              m_snapshot_canvas_height{0};
};


NS_END2
