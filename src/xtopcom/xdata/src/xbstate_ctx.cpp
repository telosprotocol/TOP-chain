// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xbstate_ctx.h"

#include "xbase/xobject.h"
#include "xbasic/xbyte_buffer.h"
#include "xbasic/xhex.h"
#include "xdata/xdata_error.h"
#include "xdata/xproperty.h"
#include "xevm_common/fixed_hash.h"
#include "xevm_common/rlp.h"
#include "xvledger/xvproperty.h"
#include "xvledger/xvpropertyrules.h"

#include <cinttypes>
#include <string>

NS_BEG2(top, data)

#define CHECK_PROPERTY_NULL_RETURN(propobj, funcname, propname)                                                                                                                    \
    do {                                                                                                                                                                           \
        if (nullptr == propobj) {                                                                                                                                                  \
            std::string str_funcname = std::string(funcname);                                                                                                                      \
            xwarn("%s,fail-property not exist.address=%s,propname=%s", str_funcname.c_str(), get_address().c_str(), propname.c_str());                                             \
            return xaccount_property_not_create;                                                                                                                                   \
        }                                                                                                                                                                          \
    } while (0)

#define CHECK_FIND_PROPERTY(bstate, propname)                                                                                                                                      \
    do {                                                                                                                                                                           \
        if (false == bstate->find_property(propname)) {                                                                                                                            \
            xwarn("xbstate_ctx_t,fail-find property.address=%s,propname=%s", get_address().c_str(), propname.c_str());                                                             \
            return xaccount_property_not_create;                                                                                                                                   \
        }                                                                                                                                                                          \
    } while (0)

xbstate_ctx_t::xbstate_ctx_t(base::xvbstate_t * bstate, bool readonly) {
    bstate->add_ref();
    m_bstate.attach(bstate);

    if (!readonly) {
        m_canvas = make_object_ptr<base::xvcanvas_t>();
        m_snapshot_canvas_height = 0;
        base::xvbstate_t * _new_bstate = static_cast<base::xvbstate_t *>(bstate->clone());
        xassert(_new_bstate != nullptr);
        m_snapshot_origin_bstate.attach(_new_bstate);  // clone another bstate
    }
}

xbstate_ctx_t::~xbstate_ctx_t() {
    if (m_bstate != nullptr) {
        m_bstate->close();  // must do close firstly
        m_bstate = nullptr;
    }
    if (m_snapshot_origin_bstate != nullptr) {
        m_snapshot_origin_bstate->close();  // must do close firstly
        m_snapshot_origin_bstate = nullptr;
    }
}

size_t xbstate_ctx_t::do_snapshot() {
    size_t change = m_canvas->get_op_records_size() - m_snapshot_canvas_height;
    m_snapshot_canvas_height = m_canvas->get_op_records_size();
    return change;
}

bool xbstate_ctx_t::do_rollback() {
    if (m_snapshot_canvas_height < m_canvas->get_op_records_size()) {
        m_canvas->rollback(m_snapshot_canvas_height);
        std::deque<base::xvmethod_t> records = m_canvas->clone();

        base::xauto_ptr<base::xvbstate_t> _new_bstate = dynamic_cast<base::xvbstate_t *>(m_snapshot_origin_bstate->clone());
        bool ret = _new_bstate->apply_changes_of_binlog(std::move(records));
        if (!ret) {
            xerror("xbstate_ctx_t::do_rollback fail-apply_changes_of_binlog");
            return ret;
        }
        m_bstate = _new_bstate;
        xdbg("xbstate_ctx_t::do_rollback rollback addr %s", get_address().c_str());
    }
    return true;
}

bool xbstate_ctx_t::is_state_readonly() const {
    return m_canvas == nullptr;
}

bool xbstate_ctx_t::is_state_dirty() const {
    return m_snapshot_canvas_height != m_canvas->get_op_records_size();
}

bool xbstate_ctx_t::is_state_changed() const {
    return m_snapshot_canvas_height != 0;
}

std::string xbstate_ctx_t::dump() const {
    char local_param_buf[256];
    xprintf(local_param_buf,
            sizeof(local_param_buf),
            "{bstatectx:address=%s,height=%ld,snapshot_h=%" PRIu64 ",records_h=0x%" PRIx64 " :}",
            get_address().c_str(),
            get_block_height(),
            m_snapshot_canvas_height,
            m_canvas->get_op_records_size());
    return std::string(local_param_buf);
}

std::string xbstate_ctx_t::take_snapshot() {
    std::string fullstate_bin;
    m_bstate->take_snapshot(fullstate_bin);
    return fullstate_bin;
}
std::string xbstate_ctx_t::take_binlog() {
    std::string property_binlog;
    m_canvas->encode(property_binlog);
    return property_binlog;
}

size_t xbstate_ctx_t::get_canvas_records_size() const {
    return m_canvas->get_op_records_size();
}

int32_t xbstate_ctx_t::check_create_property(const std::string & key) {
    if (false == base::xvpropertyrules_t::is_valid_sys_contract_property(key)) {
        xerror("xbstate_ctx_t::check_create_property,property name not valid.key=%s", key.c_str());
        return xaccount_property_create_fail;
    }
    if (get_bstate()->find_property(key)) {
        xerror("xbstate_ctx_t::check_create_property fail-already exist.propname=%s", key.c_str());
        return xaccount_property_create_fail;
    }
    return xsuccess;
}

base::xauto_ptr<base::xstringvar_t> xbstate_ctx_t::load_string_for_write(const std::string & key) {
    if (false == get_bstate()->find_property(key)) {
        if (base::xvpropertyrules_t::is_valid_native_property(key)) {
            return get_bstate()->new_string_var(key, m_canvas.get());
        }
    }
    auto propobj = get_bstate()->load_string_var(key);
    if (nullptr != propobj) {
        return propobj;
    }
    return nullptr;
}
base::xauto_ptr<base::xdequevar_t<std::string>> xbstate_ctx_t::load_deque_for_write(const std::string & key) {
    if (false == get_bstate()->find_property(key)) {
        if (base::xvpropertyrules_t::is_valid_native_property(key)) {
            return get_bstate()->new_string_deque_var(key, m_canvas.get());
        }
    }
    auto propobj = get_bstate()->load_string_deque_var(key);
    if (nullptr != propobj) {
        return propobj;
    }
    return nullptr;
}
base::xauto_ptr<base::xmapvar_t<std::string>> xbstate_ctx_t::load_map_for_write(const std::string & key) {
    if (false == get_bstate()->find_property(key)) {
        if (base::xvpropertyrules_t::is_valid_native_property(key)) {
            return get_bstate()->new_string_map_var(key, m_canvas.get());
        }
    }
    auto propobj = get_bstate()->load_string_map_var(key);
    if (nullptr != propobj) {
        return propobj;
    }
    return nullptr;
}
base::xauto_ptr<base::xtokenvar_t> xbstate_ctx_t::load_token_for_write(const std::string & key) {
    if (false == get_bstate()->find_property(key)) {
        if (base::xvpropertyrules_t::is_valid_native_property(key)) {
            return get_bstate()->new_token_var(key, m_canvas.get());
        }
    }
    auto propobj = get_bstate()->load_token_var(key);
    if (nullptr != propobj) {
        return propobj;
    }
    xassert(false);
    return nullptr;
}
base::xauto_ptr<base::xvintvar_t<uint64_t>> xbstate_ctx_t::load_uin64_for_write(const std::string & key) {
    if (false == get_bstate()->find_property(key)) {
        if (base::xvpropertyrules_t::is_valid_native_property(key)) {
            return get_bstate()->new_uint64_var(key, m_canvas.get());
        }
    }
    auto propobj = get_bstate()->load_uint64_var(key);
    if (nullptr != propobj) {
        return propobj;
    }
    xassert(false);
    return nullptr;
}
int32_t xbstate_ctx_t::string_create(const std::string & key) {
    xdbg("xbstate_ctx_t::string_create,property_modify_enter.address=%s,height=%ld,propname=%s", get_address().c_str(), get_chain_height(), key.c_str());
    auto ret = check_create_property(key);
    if (ret) {
        return ret;
    }
    auto propobj = get_bstate()->new_string_var(key, m_canvas.get());
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::string_create", key);

    return xsuccess;
}
int32_t xbstate_ctx_t::string_set(const std::string & key, const std::string & value) {
    xdbg("xbstate_ctx_t::string_set,property_modify_enter.address=%s,height=%ld,propname=%s", get_address().c_str(), get_chain_height(), key.c_str());
    auto propobj = load_string_for_write(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::string_set", key);
    return propobj->reset(value, m_canvas.get()) == true ? xsuccess : xaccount_property_operate_fail;
}

int32_t xbstate_ctx_t::string_get(const std::string & key, std::string & value) const {
    CHECK_FIND_PROPERTY(get_bstate(), key);

    auto propobj = get_bstate()->load_string_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::string_get", key);

    value = propobj->query();
    return xsuccess;
}

int32_t xbstate_ctx_t::list_create(const std::string & key) {
    xdbg("xbstate_ctx_t::list_create,property_modify_enter.address=%s,height=%ld,propname=%s", get_address().c_str(), get_chain_height(), key.c_str());
    auto ret = check_create_property(key);
    if (ret) {
        return ret;
    }
    auto & bstate = get_bstate();
    auto propobj = bstate->new_string_deque_var(key, m_canvas.get());
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::list_create", key);
    return xsuccess;
}
int32_t xbstate_ctx_t::list_push_back(const std::string & key, const std::string & value) {
    xdbg("xbstate_ctx_t::list_push_back,property_modify_enter.address=%s,height=%ld,propname=%s", get_address().c_str(), get_chain_height(), key.c_str());
    auto propobj = load_deque_for_write(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::list_push_back", key);
    return propobj->push_back(value, m_canvas.get()) == true ? xsuccess : xaccount_property_operate_fail;
}
int32_t xbstate_ctx_t::list_push_front(const std::string & key, const std::string & value) {
    xdbg("xbstate_ctx_t::list_push_front,property_modify_enter.address=%s,height=%ld,propname=%s", get_address().c_str(), get_chain_height(), key.c_str());
    auto propobj = load_deque_for_write(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::list_push_front", key);
    return propobj->push_front(value, m_canvas.get()) == true ? xsuccess : xaccount_property_operate_fail;
}
int32_t xbstate_ctx_t::list_pop_back(const std::string & key, std::string & value) {
    xdbg("xbstate_ctx_t::list_pop_back,property_modify_enter.address=%s,height=%ld,propname=%s", get_address().c_str(), get_chain_height(), key.c_str());
    auto & bstate = get_bstate();
    auto propobj = bstate->load_string_deque_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::list_pop_back", key);

    if (propobj->query().size() == 0) {
        xwarn("xbstate_ctx_t::list_pop_back fail-property is empty.addr=%s,propname=%s", get_address().c_str(), key.c_str());
        return xaccount_property_operate_fail;
    }
    value = propobj->query_back();
    return propobj->pop_back(m_canvas.get()) == true ? xsuccess : xaccount_property_operate_fail;
}
int32_t xbstate_ctx_t::list_pop_front(const std::string & key, std::string & value) {
    xdbg("xbstate_ctx_t::list_pop_front,property_modify_enter.address=%s,height=%ld,propname=%s", get_address().c_str(), get_chain_height(), key.c_str());
    auto & bstate = get_bstate();
    auto propobj = bstate->load_string_deque_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::list_pop_front", key);
    if (propobj->query().size() == 0) {
        xwarn("xbstate_ctx_t::list_pop_front fail-property is empty.addr=%s,propname=%s", get_address().c_str(), key.c_str());
        return xaccount_property_operate_fail;
    }
    value = propobj->query_front();
    return propobj->pop_front(m_canvas.get()) == true ? xsuccess : xaccount_property_operate_fail;
}
int32_t xbstate_ctx_t::list_clear(const std::string & key) {
    xdbg("xbstate_ctx_t::list_clear,property_modify_enter.address=%s,height=%ld,propname=%s", get_address().c_str(), get_chain_height(), key.c_str());
    auto & bstate = get_bstate();
    auto propobj = bstate->load_string_deque_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::list_clear", key);
    propobj->clear(m_canvas.get());
    return xsuccess;
}

int32_t xbstate_ctx_t::list_get(const std::string & key, const uint32_t pos, std::string & value) {
    auto & bstate = get_bstate();
    CHECK_FIND_PROPERTY(bstate, key);
    auto propobj = bstate->load_string_deque_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::list_get", key);

    if (pos >= propobj->query().size()) {
        xwarn("xbstate_ctx_t::list_get fail-query pos invalid.addr=%s,propname=%s", get_address().c_str(), key.c_str());
        return xaccount_property_operate_fail;
    }
    value = propobj->query(pos);
    return xsuccess;
}

int32_t xbstate_ctx_t::list_size(const std::string & key, int32_t & size) {
    auto & bstate = get_bstate();
    CHECK_FIND_PROPERTY(bstate, key);

    auto propobj = bstate->load_string_deque_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::list_size", key);

    size = (int32_t)(propobj->query().size());
    return xsuccess;
}

int32_t xbstate_ctx_t::list_copy_get(const std::string & key, std::deque<std::string> & deque) {
    auto & bstate = get_bstate();
    CHECK_FIND_PROPERTY(bstate, key);
    auto propobj = bstate->load_string_deque_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::list_copy_get", key);
    deque = propobj->query();
    return xsuccess;
}

int32_t xbstate_ctx_t::map_create(const std::string & key) {
    xdbg("xbstate_ctx_t::map_create,property_modify_enter.address=%s,height=%ld,propname=%s", get_address().c_str(), get_chain_height(), key.c_str());
    auto ret = check_create_property(key);
    if (ret) {
        return ret;
    }
    auto & bstate = get_bstate();
    auto propobj = bstate->new_string_map_var(key, m_canvas.get());
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::map_create", key);
    return xsuccess;
}

int32_t xbstate_ctx_t::map_get(const std::string & key, const std::string & field, std::string & value) {
    auto & bstate = get_bstate();
    CHECK_FIND_PROPERTY(bstate, key);
    auto propobj = bstate->load_string_map_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::map_get", key);

    if (false == propobj->find(field)) {
        xwarn("xbstate_ctx_t::map_get fail-field not find.addr=%s,propname=%s,field=%s", get_address().c_str(), key.c_str(), field.c_str());
        return xaccount_property_map_field_not_create;
    }
    value = propobj->query(field);
    return xsuccess;
}

int32_t xbstate_ctx_t::map_set(const std::string & key, const std::string & field, const std::string & value) {
    xdbg("xbstate_ctx_t::map_set,property_modify_enter.address=%s,height=%ld,propname=%s,field=%s", get_address().c_str(), get_chain_height(), key.c_str(), field.c_str());
    auto propobj = load_map_for_write(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::map_set", key);
    if (true == propobj->find(field)) {
        auto old_value = propobj->query(field);
        if (old_value == value) {
            // TODO(jimmy) some system contract will set same value
            xwarn("xbstate_ctx_t::map_set-warn set same value.address=%s,height=%ld,propname=%s,field=%s", get_address().c_str(), get_chain_height(), key.c_str(), field.c_str());
            return xsuccess;
        }
    }
    return propobj->insert(field, value, m_canvas.get()) == true ? xsuccess : xaccount_property_operate_fail;
}
int32_t xbstate_ctx_t::map_remove(const std::string & key, const std::string & field) {
    xdbg("xbstate_ctx_t::map_remove,property_modify_enter.address=%s,height=%ld,propname=%s,field=%s", get_address().c_str(), get_chain_height(), key.c_str(), field.c_str());
    auto & bstate = get_bstate();
    auto propobj = bstate->load_string_map_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::map_remove", key);
    if (false == propobj->find(field)) {
        xwarn("xbstate_ctx_t::map_remove fail-field not find.addr=%s,propname=%s", get_address().c_str(), key.c_str());
        return xaccount_property_map_field_not_create;
    }
    return propobj->erase(field, m_canvas.get()) == true ? xsuccess : xaccount_property_operate_fail;
}
int32_t xbstate_ctx_t::map_clear(const std::string & key) {
    xdbg("xbstate_ctx_t::map_clear,property_modify_enter.address=%s,height=%ld,propname=%s", get_address().c_str(), get_chain_height(), key.c_str());
    auto & bstate = get_bstate();
    auto propobj = bstate->load_string_map_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::map_clear", key);
    return propobj->clear(m_canvas.get()) == true ? xsuccess : xaccount_property_operate_fail;
}
int32_t xbstate_ctx_t::map_size(const std::string & key, int32_t & size) const {
    auto & bstate = get_bstate();
    CHECK_FIND_PROPERTY(bstate, key);
    auto propobj = bstate->load_string_map_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::map_size", key);

    size = (int32_t)(propobj->query().size());
    return xsuccess;
}
int32_t xbstate_ctx_t::map_copy_get(const std::string & key, std::map<std::string, std::string> & map) const {
    auto & bstate = get_bstate();
    CHECK_FIND_PROPERTY(bstate, key);
    auto propobj = bstate->load_string_map_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::map_copy_get", key);

    map = propobj->query();
    return xsuccess;
}

uint64_t xbstate_ctx_t::token_balance(const std::string & key) {
    auto & bstate = get_bstate();
    if (!bstate->find_property(key)) {
        return 0;
    }
    auto propobj = bstate->load_token_var(key);
    base::vtoken_t balance = propobj->get_balance();
    if (balance < 0) {
        xerror("xbstate_ctx_t::token_balance fail-should not appear. balance=%ld", balance);
        return 0;
    }
    return (uint64_t)balance;
}

int32_t xbstate_ctx_t::token_withdraw(const std::string & key, base::vtoken_t sub_token) {
    xdbg("xbstate_ctx_t::token_withdraw,property_modify_enter.address=%s,height=%ld,propname=%s,token=%ld", get_address().c_str(), get_chain_height(), key.c_str(), sub_token);
    auto propobj = load_token_for_write(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::token_withdraw", key);
    auto balance = propobj->get_balance();
    if (sub_token <= 0 || sub_token > balance) {
        xwarn("xbstate_ctx_t::token_withdraw fail-can't do withdraw. propname=%ld,balance=%ld,sub_token=%ld", key.c_str(), balance, sub_token);
        return xaccount_property_operate_fail;
    }

    auto left_token = propobj->withdraw(sub_token, m_canvas.get());
    xassert(left_token < balance);
    return xsuccess;
}

int32_t xbstate_ctx_t::token_deposit(const std::string & key, base::vtoken_t add_token) {
    xdbg("xbstate_ctx_t::token_deposit,property_modify_enter.address=%s,height=%ld,propname=%s,token=%ld", get_address().c_str(), get_chain_height(), key.c_str(), add_token);
    auto propobj = load_token_for_write(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::token_withdraw", key);
    if (add_token <= 0) {
        xwarn("xbstate_ctx_t::token_withdraw fail-can't do deposit. add_token=%ld", add_token);
        return xaccount_property_operate_fail;
    }
    auto balance = propobj->get_balance();
    auto left_token = propobj->deposit(add_token, m_canvas.get());
    xassert(left_token > balance);
    return xsuccess;
}

int32_t xbstate_ctx_t::uint64_add(const std::string & key, uint64_t change) {
    if (change == 0) {
        return xsuccess;
    }
    xdbg("xbstate_ctx_t::uint64_add,property_modify_enter.address=%s,height=%ld,propname=%s,change=%ld", get_address().c_str(), get_chain_height(), key.c_str(), change);
    auto propobj = load_uin64_for_write(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::uint64_add", key);
    uint64_t oldvalue = propobj->get();
    uint64_t newvalue = oldvalue + change;  // TODO(jimmy) overflow ?
    propobj->set(newvalue, m_canvas.get());
    xdbg("xbstate_ctx_t::uint64_add property=%s,old_value=%ld,new_value=%ld,change=%ld", key.c_str(), oldvalue, newvalue, change);
    return xsuccess;
}
int32_t xbstate_ctx_t::uint64_sub(const std::string & key, uint64_t change) {
    if (change == 0) {
        return xsuccess;
    }
    xdbg("xbstate_ctx_t::uint64_sub,property_modify_enter.address=%s,height=%ld,propname=%s,change=%ld", get_address().c_str(), get_chain_height(), key.c_str(), change);
    auto propobj = load_uin64_for_write(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::uint64_sub", key);
    uint64_t oldvalue = propobj->get();
    if (oldvalue < change) {
        xwarn("xbstate_ctx_t::uint64_sub fail-invalid para.value=%ld,change=%ld", oldvalue, change);
        return xaccount_property_operate_fail;
    }
    uint64_t newvalue = oldvalue - change;
    propobj->set(newvalue, m_canvas.get());
    xdbg("xbstate_ctx_t::uint64_sub property=%s,old_value=%ld,new_value=%ld,change=%ld", key.c_str(), oldvalue, newvalue, change);
    return xsuccess;
}

int32_t xbstate_ctx_t::uint64_set(const std::string & key, uint64_t value) {
    xdbg("xbstate_ctx_t::uint64_set,property_modify_enter.address=%s,height=%ld,propname=%s,change=%ld", get_address().c_str(), get_chain_height(), key.c_str(), value);
    auto propobj = load_uin64_for_write(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::uint64_set", key);
    uint64_t oldvalue = propobj->get();
    if (oldvalue == value) {
        xwarn("xbstate_ctx_t::uint64_set fail-invalid para.value=%ld", oldvalue);
        return xaccount_property_operate_fail;
    }
    propobj->set(value, m_canvas.get());
    xdbg("xbstate_ctx_t::uint64_set property=%s,old_value=%ld,new_value=%ld", key.c_str(), oldvalue, value);
    return xsuccess;
}

//========= // APIs for property operation with property value return ========
uint64_t xbstate_ctx_t::token_get(const std::string & prop) const {
    if (false == get_bstate()->find_property(prop)) {
        return 0;
    }
    auto propobj = get_bstate()->load_token_var(prop);
    base::vtoken_t balance = propobj->get_balance();
    if (balance < 0) {
        xerror("xbstate_ctx_t::token_get fail-should not appear. balance=%ld", balance);
        return 0;
    }
    return (uint64_t)balance;
}

uint64_t xbstate_ctx_t::uint64_property_get(const std::string & prop) const {
    if (false == get_bstate()->find_property(prop)) {
        return 0;
    }
    auto propobj = get_bstate()->load_uint64_var(prop);
    uint64_t value = propobj->get();
    return value;
}

std::string xbstate_ctx_t::map_get(const std::string & prop, const std::string & field) const {
    if (false == get_bstate()->find_property(prop)) {
        return {};
    }
    auto propobj = get_bstate()->load_string_map_var(prop);
    return propobj->query(field);
}

std::map<std::string, std::string> xbstate_ctx_t::map_get(const std::string & prop) const {
    std::map<std::string, std::string> values;
    map_copy_get(prop, values);
    return values;
}

std::string xbstate_ctx_t::string_get(const std::string & prop) const {
    if (false == get_bstate()->find_property(prop)) {
        return {};
    }
    auto propobj = get_bstate()->load_string_var(prop);
    return propobj->query();
}

int32_t xbstate_ctx_t::set_tep_balance(const std::string & token_name, evm_common::u256 new_balance) {
    assert(token_name.length() == 1);

    xdbg("xbstate_ctx_t::set_tep_balance,property_modify_enter.address=%s,height=%ld,token_name=%s,new_balance=%s",
         get_address().c_str(),
         get_chain_height(),
         common::symbol(top::from_string<common::xtoken_id_t>(token_name)).c_str(),
         new_balance.str().c_str());
    top::xbytes_t result_rlp = evm_common::RLP::encode(new_balance);
    return set_tep_balance_bytes(token_name, result_rlp);
}

int32_t xbstate_ctx_t::set_tep_balance_bytes(const std::string & token_name, const top::xbytes_t & new_balance) {
    assert(token_name.length() == 1);

    auto propobj = load_tep_token_for_write();
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::set_tep_balance", token_name);

    std::error_code ec;
    std::string new_balance_str = top::from_bytes<std::string>(new_balance, ec);
    if (ec) {
        xwarn("convert bytes balance to string format fialed");
        return xaccount_property_operate_fail;
    }
    bool ret = propobj->insert(token_name, new_balance_str, m_canvas.get());
    if (!ret) {
        xwarn("failed to update TEP1 token balance property. token %s", common::symbol(top::from_string<common::xtoken_id_t>(token_name)).c_str());
        return xaccount_property_operate_fail;
    }
    return xsuccess;
}

base::xauto_ptr<base::xmapvar_t<std::string>> xbstate_ctx_t::load_tep_token_for_write() {
    if (false == m_bstate->find_property(data::XPROPERTY_TEP1_BALANCE_KEY)) {
        return m_bstate->new_string_map_var(data::XPROPERTY_TEP1_BALANCE_KEY, m_canvas.get());
    }
    auto propobj = m_bstate->load_string_map_var(data::XPROPERTY_TEP1_BALANCE_KEY);
    if (nullptr != propobj) {
        return propobj;
    }
    xassert(false);
    return nullptr;
}

evm_common::u256 xbstate_ctx_t::tep_token_balance(const std::string & token_name) const {
    assert(token_name.length() == 1);

    auto value_rlp = tep_token_balance_bytes(token_name);
    if (value_rlp.empty()) {
        return 0;
    }

    auto decoded = evm_common::RLP::decode(value_rlp);
    std::string str(decoded.decoded[0].begin(), decoded.decoded[0].end());
    evm_common::u256 balance = evm_common::fromBigEndian<top::evm_common::u256>(str);

    xdbg("xbstate_ctx_t::tep_token_balance address=%s,token=%s,balance=%s", get_address().c_str(), top::to_hex(token_name).c_str(), balance.str().c_str());
    return balance;
}

evm_common::u256 xbstate_ctx_t::tep_token_balance(common::xtoken_id_t const token_id) const {
    return tep_token_balance(top::to_string(token_id));
}

top::xbytes_t xbstate_ctx_t::tep_token_balance_bytes(const std::string & token_name) const {
    assert(token_name.length() == 1);

    auto & bstate = get_bstate();
    if (!bstate->find_property(data::XPROPERTY_TEP1_BALANCE_KEY)) {
        return {};
    }
    auto propobj = bstate->load_string_map_var(data::XPROPERTY_TEP1_BALANCE_KEY);
    if (nullptr != propobj) {
        auto balance_str = propobj->query(token_name);
        if (balance_str.empty()) {
            return {};
        }

        xbytes_t value_rlp = to_bytes(balance_str);
        return value_rlp;
    }

    return {};
}

top::xbytes_t xbstate_ctx_t::tep_token_balance_bytes(common::xtoken_id_t const token_id) const {
    return tep_token_balance_bytes(top::to_string(token_id));
}

int32_t xbstate_ctx_t::tep_token_withdraw(const std::string & token_name, evm_common::u256 sub_token) {
    assert(token_name.length() == 1);

    xdbg("xbstate_ctx_t::tep_token_withdraw,property_modify_enter.address=%s,height=%ld,tokenname=%s,token=%s",
         get_address().c_str(),
         get_chain_height(),
         top::to_hex(token_name).c_str(),
         sub_token.str().c_str());

    auto propobj = load_tep_token_for_write();
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::tep_token_withdraw", token_name);
    auto balance_str = propobj->query(token_name);

    evm_common::u256 balance = 0;
    if (!balance_str.empty()) {
        xbytes_t value_rlp = to_bytes(balance_str);
        auto decoded = evm_common::RLP::decode(value_rlp);
        std::string str(decoded.decoded[0].begin(), decoded.decoded[0].end());
        balance = evm_common::fromBigEndian<top::evm_common::u256>(str);
    }

    if (sub_token <= 0 || sub_token > balance) {
        xwarn("xbstate_ctx_t::tep_token_withdraw fail-can't do withdraw. token_name=%s,balance=%s,sub_token=%s",
              top::to_hex(token_name).c_str(),
              balance.str().c_str(),
              sub_token.str().c_str());
        return xaccount_property_operate_fail;
    }

    evm_common::u256 new_balance = balance - sub_token;
    top::xbytes_t result_rlp = evm_common::RLP::encode(new_balance);
    std::error_code ec;
    std::string new_balance_str = top::from_bytes<std::string>(result_rlp, ec);
    if (ec) {
        return xaccount_property_operate_fail;
    }
    auto ret = propobj->insert(token_name, new_balance_str, m_canvas.get());
    if (!ret) {
        return xaccount_property_operate_fail;
    }
    xdbg("xbstate_ctx_t::tep_token_withdraw address=%s,balance=%s,hex=%s", get_address().c_str(), new_balance.str().c_str(), toHex((evm_common::h256)new_balance).c_str());
    return xsuccess;
}

int32_t xbstate_ctx_t::tep_token_withdraw(common::xtoken_id_t const token_id, evm_common::u256 sub_token) {
    return tep_token_withdraw(top::to_string(token_id), sub_token);
}

int32_t xbstate_ctx_t::tep_token_deposit(const std::string & token_name, evm_common::u256 add_token) {
    assert(token_name.length() == 1);

    xdbg("xbstate_ctx_t::tep_token_deposit,property_modify_enter.address=%s,height=%ld,token_name=%s,token=%s",
         get_address().c_str(),
         get_chain_height(),
         top::to_hex(token_name).c_str(),
         add_token.str().c_str());

    auto propobj = load_tep_token_for_write();
    CHECK_PROPERTY_NULL_RETURN(propobj, "xbstate_ctx_t::tep_token_deposit", token_name);
    auto balance_str = propobj->query(token_name);

    evm_common::u256 balance = 0;
    if (!balance_str.empty()) {
        xbytes_t value_rlp = to_bytes(balance_str);
        auto decoded = evm_common::RLP::decode(value_rlp);
        std::string str(decoded.decoded[0].begin(), decoded.decoded[0].end());
        balance = evm_common::fromBigEndian<top::evm_common::u256>(str);
    }

    if (add_token <= 0) {
        xwarn("xbstate_ctx_t::tep_token_withdraw fail-can't do withdraw. token_name=%s,balance=%s,add_token=%s",
              top::to_hex(token_name).c_str(),
              balance.str().c_str(),
              add_token.str().c_str());
        return xaccount_property_operate_fail;
    }

    evm_common::u256 new_balance = balance + add_token;
    top::xbytes_t result_rlp = evm_common::RLP::encode(new_balance);
    std::error_code ec;
    std::string new_balance_str = top::from_bytes<std::string>(result_rlp, ec);
    if (ec) {
        return xaccount_property_operate_fail;
    }
    auto ret = propobj->insert(token_name, new_balance_str, m_canvas.get());
    if (!ret) {
        return xaccount_property_operate_fail;
    }
    xdbg("xbstate_ctx_t::tep_token_deposit address=%s,balance=%s,hex=%s", get_address().c_str(), new_balance.str().c_str(), toHex((evm_common::h256)new_balance).c_str());
    return xsuccess;
}

int32_t xbstate_ctx_t::tep_token_deposit(common::xtoken_id_t const token_id, evm_common::u256 add_token) {
    return tep_token_deposit(top::to_string(token_id), add_token);
}

int32_t xbstate_ctx_t::set_tep_balance(common::xtoken_id_t const token_id, evm_common::u256 new_balance) {
    return set_tep_balance(top::to_string(token_id), new_balance);
}

int32_t xbstate_ctx_t::set_tep_balance_bytes(common::xtoken_id_t const token_id, const top::xbytes_t & new_balance) {
    return set_tep_balance_bytes(top::to_string(token_id), new_balance);
}

common::xaccount_address_t xbstate_ctx_t::account_address() const {
    std::error_code ec;
    return common::xaccount_address_t::build_from(get_account(), ec);
}


NS_END2
