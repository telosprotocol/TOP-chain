// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include <vector>
#include "xbase/xobject_ptr.h"
#include "xbasic/xversion.h"
#include "xvledger/xdataobj_base.hpp"
#include "xvledger/xvaccount.h"
#include "xvledger/xvaction.h"
#include "xvledger/xventity.h"
#include "xdata/xtransaction.h"
#include "xdata/xblock_paras.h"
namespace top { namespace data {

// lightunit action is wrap for vaction, should not cache any members
class xlightunit_action_t : public base::xvaction_t {
 protected:

 public:
    xlightunit_action_t(const base::xvaction_t & _action);
    xlightunit_action_t(const std::string & tx_hash, base::enum_transaction_subtype _subtype, const std::string & caller_addr,const std::string & target_uri,const std::string & method_name);
    ~xlightunit_action_t() = default;

    base::xvtxkey_t             get_tx_key() const {return base::xvtxkey_t(get_tx_hash(), get_tx_subtype());}
    const std::string &         get_tx_hash() const {return get_org_tx_hash();}
    bool                        is_self_tx() const {return get_tx_subtype() == base::enum_transaction_subtype_self;}
    bool                        is_send_tx() const {return get_tx_subtype() == base::enum_transaction_subtype_send;}
    bool                        is_recv_tx() const {return get_tx_subtype() == base::enum_transaction_subtype_recv;}
    bool                        is_confirm_tx() const {return get_tx_subtype() == base::enum_transaction_subtype_confirm;}
    base::enum_transaction_subtype    get_tx_subtype() const {return (base::enum_transaction_subtype)get_org_tx_action_id();}
    std::string                 get_tx_dump_key() const {return get_tx_key().get_tx_dump_key();}
    uint256_t                   get_tx_hash_256() const {return get_tx_key().get_tx_hash_256();}
    std::string                 get_tx_hex_hash() const {return get_tx_key().get_tx_hex_hash();}
    std::string                 get_tx_subtype_str() const {return get_tx_key().get_tx_subtype_str();}

    uint32_t                    get_used_disk()const;
    uint32_t                    get_used_tgas()const;
    uint32_t                    get_used_deposit()const;
    uint32_t                    get_send_tx_lock_tgas()const;
    uint32_t                    get_recv_tx_use_send_tx_tgas()const;
    enum_xunit_tx_exec_status   get_tx_exec_status() const;
    uint64_t                    get_receipt_id() const;
    base::xtable_shortid_t      get_receipt_id_self_tableid()const;
    base::xtable_shortid_t      get_receipt_id_peer_tableid()const;

 private:
    std::string                 get_action_result_property(const std::string & key) const;
};

using xlightunit_action_ptr_t = std::shared_ptr<xlightunit_action_t>;

}  // namespace data
}  // namespace top
