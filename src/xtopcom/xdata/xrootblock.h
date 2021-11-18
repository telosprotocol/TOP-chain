// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <map>
#include <string>

#include "xvledger/xvblock.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xdata/xdata_common.h"
#include "xdata/xdatautil.h"
#include "xdata/xblock.h"

NS_BEG2(top, data)

class xrootblock_input_t : public base::xdataunit_t {
 public:
    xrootblock_input_t();

 protected:
    virtual ~xrootblock_input_t();
    int32_t do_write(base::xstream_t &stream);
    int32_t do_read(base::xstream_t &stream);

 public:
    bool    set_account_balances(std::map<std::string, uint64_t> const& balances);
    bool    set_genesis_funds_accounts(std::vector<std::string> const & accounts);
    bool    set_genesis_tcc_accounts(std::vector<std::string> const& accounts);
    bool    set_genesis_nodes(const std::vector<node_info_t> & nodes);

 public:
    const uint64_t get_account_balance(const std::string& account_addr) const;
    const std::map<std::string, uint64_t>& get_account_balances() const {return m_account_balances;}
    const std::vector<std::string>& get_funds_accounts() const {return m_geneis_funds_accounts;}
    const std::vector<std::string>& get_tcc_accounts() const {return m_tcc_accounts;};
    const std::vector<node_info_t> & get_seed_nodes() const {return m_genesis_nodes;}

 private:
    std::map<std::string, uint64_t>           m_account_balances;
    std::vector<std::string>                  m_geneis_funds_accounts;
    std::vector<std::string>                  m_tcc_accounts;
    std::vector<node_info_t>                  m_genesis_nodes;
};

struct xrootblock_para_t {
    std::map<std::string, uint64_t>           m_account_balances;
    std::vector<std::string>                  m_geneis_funds_accounts;
    std::vector<std::string>                  m_tcc_accounts;
    std::vector<node_info_t>                  m_genesis_nodes;
    uint64_t                                  m_genesis_time_stamp{0};
};
class xrootblock_t : public xblock_t {
 public:
    static XINLINE_CONSTEXPR char const * root_resource_name     = "0";
    static XINLINE_CONSTEXPR char const * ROOT_BLOCK_PROPERTY_NAME  = "$R";
 protected:
    enum { object_type_value = enum_xdata_type::enum_xdata_type_max - xdata_type_rootblock };
 public:
    static bool init(const xrootblock_para_t & para);
 public:
    xrootblock_t();
    xrootblock_t(base::xvheader_t & header, base::xvqcert_t & cert, base::xvinput_t* input, base::xvoutput_t* output);
 protected:
    virtual ~xrootblock_t();
 private:
    xrootblock_t(const xrootblock_t &) = delete;
    xrootblock_t & operator = (const xrootblock_t &) = delete;

    static std::once_flag m_flag;
    static xrootblock_t* m_instance;

 public:
    static int32_t get_object_type() {return object_type_value;}
    static xobject_t *create_object(int type);
    void *query_interface(const int32_t _enum_xobject_type_) override;
    void dump_block_data(xJson::Value & json) const override;

 protected:
    xrootblock_input_t*    get_rootblock_input() const;

 public:
    static base::xvblock_t* get_rootblock();
    static std::string get_rootblock_address();
    static uint64_t get_initial_balance(const std::string& account_addr);
    static const std::map<std::string, uint64_t>& get_account_balances();
    static const std::vector<std::string>& get_tcc_initial_committee_addr();
    static const std::vector<node_info_t> & get_seed_nodes();
    static std::map<std::string, uint64_t> get_all_genesis_accounts();
    static bool is_seed_node(const std::string & account);
    static const std::string get_rootblock_hash();
    static base::enum_xchain_id get_rootblock_chainid();
    static void get_rootblock_data(xJson::Value & json);
};

NS_END2
