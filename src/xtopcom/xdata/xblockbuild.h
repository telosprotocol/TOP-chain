// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xvledger/xvblockbuild.h"
#include "xdata/xlightunit.h"
#include "xdata/xfullunit.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xtableblock.h"
#include "xdata/xrootblock.h"
#include "xdata/xblock.h"

NS_BEG2(top, data)

// XTODO keep old structure for compatibility
class xtableheader_extra_t : public xserializable_based_on<void> {
 protected:
    enum xblockheader_extra_data_type : uint16_t {
        enum_extra_data_type_tgas_total_lock_amount_property_height = 0,
        enum_extra_data_type_tgas_second_level_gmtime               = 1,
        enum_extra_data_type_eth_header                             = 2,
        enum_extra_data_type_relay_block_data                       = 3,
        enum_extra_data_type_relay_wrap_info                        = 4,
    };
 public:
    static std::string build_extra_string(base::xvheader_t* _tableheader, uint64_t tgas_height, uint64_t gmtime, const std::string & eth_header);
 protected:
    int32_t do_write(base::xstream_t & stream) const override;
    int32_t do_read(base::xstream_t & stream) override;
 public:
    int32_t deserialize_from_string(const std::string & extra_data);
    int32_t serialize_to_string(std::string & extra_data);

 public:
    uint64_t get_tgas_total_lock_amount_property_height() const;
    void     set_tgas_total_lock_amount_property_height(uint64_t height);
    uint64_t get_second_level_gmtime() const;
    void     set_second_level_gmtime(uint64_t gmtime);
    std::string get_ethheader() const;
    void     set_ethheader(const std::string & value);
    std::string get_relay_block_data() const;
    void     set_relay_block_data(const std::string & relay_block_data);
    std::string get_relay_wrap_info() const;
    void     set_relay_wrap_info(const std::string & relay_wrap_info);

 private:
    std::map<uint16_t, std::string>  m_paras;
};

class xextra_map_base_t {
 public:
    int32_t serialize_to_string(std::string & str) const;
    int32_t do_write(base::xstream_t & stream) const;
    int32_t serialize_from_string(const std::string & _data);
    int32_t do_read(base::xstream_t & stream);
    void insert(const std::string & key, const std::string & val);
    std::string get_val(const std::string & key) const;
    std::map<std::string, std::string> const&   get_all_val() const {return m_map;}

 private:
    std::map<std::string, std::string> m_map;
};
class xunitheader_extra_t : public xextra_map_base_t {
 private:
    static XINLINE_CONSTEXPR char const * KEY_HEADER_KEY_TXS              = "t";
 public:
    static std::string build_extra_string(base::xvheader_t* _tableheader, const xlightunit_block_para_t & bodypara);
    static std::string build_extra_string(base::xvheader_t* _tableheader, const base::xvtxkey_vec_t & txkeys);

 public:
    std::vector<base::xvtxkey_t>    get_txkeys() const;
    void    set_txkeys(const base::xvtxkey_vec_t & txkeys);
};

class xtable_primary_inentity_extend_t : public xextra_map_base_t {
 private:
    static XINLINE_CONSTEXPR char const * KEY_ETH_RECEIPT_ACTIONS              = "t";
 public:

 public:
    base::xvactions_t  get_txactions() const;
    void    set_txactions(base::xvactions_t const& txactions);
};

class xblockaction_build_t {
 public:
    static base::xvaction_t make_tx_action(const xcons_transaction_ptr_t & tx);
    static base::xvaction_t make_block_build_action(const std::string & target_uri, const std::map<std::string, std::string> & action_result = {});
    static base::xvaction_t make_table_block_action_with_table_prop_prove(const std::string & target_uri, uint32_t block_version, const std::map<std::string, std::string> & property_hashs, base::xtable_shortid_t tableid, uint64_t height);
};

xlightunit_tx_info_ptr_t build_tx_info(const xcons_transaction_ptr_t & tx);

class xlightunit_build_t : public base::xvblockmaker_t {
 public:
    static bool should_build_unit_opt(const uint64_t clock, const uint64_t height);
    xlightunit_build_t(const std::string & account, const xlightunit_block_para_t & bodypara);
    xlightunit_build_t(base::xvblock_t* prev_block, const xlightunit_block_para_t & bodypara, const xblock_consensus_para_t & para);
    xlightunit_build_t(base::xvblock_t* prev_block, const xunit_block_para_t & bodypara, const xblock_consensus_para_t & para);
    xlightunit_build_t(base::xvheader_t* header, base::xvinput_t* input, base::xvoutput_t* output);

    base::xauto_ptr<base::xvblock_t> create_new_block() override;
private:
    bool build_block_body(const xlightunit_block_para_t & para);
    bool build_block_body_v2(const xunit_block_para_t & para);
};

class xemptyblock_build_t : public base::xvblockmaker_t {
 public:
    xemptyblock_build_t(const std::string & account);
    xemptyblock_build_t(base::xvblock_t* prev_block, const xblock_consensus_para_t & para);
    xemptyblock_build_t(base::xvblock_t* prev_block);  // TODO(jimmy) not valid block
    xemptyblock_build_t(base::xvheader_t* header);
    xemptyblock_build_t(const std::string & tc_account, uint64_t _tc_height);  // for tc block


    base::xauto_ptr<base::xvblock_t> create_new_block() override;
};

class xrelay_block_build_t : public base::xvblockmaker_t {
 public:
     xrelay_block_build_t(base::xvblock_t * prev_block,
                          const xblock_consensus_para_t & para,
                          const std::string & relay_extra_data,
                          bool need_relay_prove);
     base::xauto_ptr<base::xvblock_t> create_new_block() override;
};
class xrelayblock_build_t : public base::xvblockmaker_t {
 public:
    xrelayblock_build_t(base::xvblock_t* curr_block, const std::string & relay_block_data, const std::string & relay_wrap_data,
        const std::string& sign_data, const uint64_t& block_height);
    xrelayblock_build_t(const std::string & relay_block_data);
    base::xauto_ptr<base::xvblock_t> create_new_block() override;
};

class xfullunit_build_t : public base::xvblockmaker_t {
 public:
    xfullunit_build_t(base::xvblock_t* prev_block, const xfullunit_block_para_t & bodypara, const xblock_consensus_para_t & para);
    xfullunit_build_t(base::xvblock_t* prev_block, const xunit_block_para_t & bodypara, const xblock_consensus_para_t & para);
    xfullunit_build_t(base::xvheader_t* header, base::xvinput_t* input, base::xvoutput_t* output);

    base::xauto_ptr<base::xvblock_t> create_new_block() override;

 private:
    bool build_block_body(const xunit_block_para_t & para);
};

class xlighttable_build_t : public base::xvtableblock_maker_t {
 public:
    static std::vector<xobject_ptr_t<base::xvblock_t>> unpack_units_from_table(const base::xvblock_t* _tableblock);
    static xobject_ptr_t<base::xvblock_t> unpack_one_unit_from_table(const base::xvblock_t* _tableblock, uint32_t entity_id, const std::string & extend_cert, const std::string & extend_data);
 public:
    xlighttable_build_t(base::xvblock_t* prev_block, const xtable_block_para_t & bodypara, const xblock_consensus_para_t & para);

    base::xauto_ptr<base::xvblock_t> create_new_block() override;

 private:
    bool                build_block_body(const xtable_block_para_t & para, const base::xvaccount_t & account, uint64_t height);
    base::xvaction_t    make_action(const std::vector<xobject_ptr_t<base::xvblock_t>> & batch_units);
    static base::xauto_ptr<base::xvinput_t>     make_unit_input_from_table(const base::xvblock_t* _tableblock, const base::xtable_inentity_extend_t & extend, base::xvinentity_t* _table_unit_inentity);
    static base::xauto_ptr<base::xvoutput_t>    make_unit_output_from_table(const base::xvblock_t* _tableblock, const base::xtable_inentity_extend_t & extend, base::xvoutentity_t* _table_unit_outentity);
};

class xfulltable_build_t : public base::xvblockmaker_t {
 public:
    xfulltable_build_t(base::xvblock_t* prev_block, const xfulltable_block_para_t & bodypara, const xblock_consensus_para_t & para);

    base::xauto_ptr<base::xvblock_t> create_new_block() override;

 private:
    bool build_block_body(const xfulltable_block_para_t & para, const base::xvaccount_t & account, uint64_t height);
};

class xrootblock_build_t : public base::xvblockmaker_t {
 public:
    // root block is a root genesis block
    xrootblock_build_t(base::enum_xchain_id chainid, const std::string & account, const xrootblock_para_t & bodypara);

    base::xauto_ptr<base::xvblock_t> create_new_block() override;

 private:
    bool build_block_body(const xrootblock_para_t & para);
};

class xrelay_wrap_info_t : public xextra_map_base_t {
protected:
    static XINLINE_CONSTEXPR char const * KEY_WRAP_PHASE = "a";
    static XINLINE_CONSTEXPR char const * KEY_EVM_HEIGHT = "b";
    static XINLINE_CONSTEXPR char const * KEY_ELECT_HEIGHT = "c";

public:
    static std::string build_relay_wrap_info_string(uint8_t wrap_phase, uint64_t evm_height, uint64_t elect_height);

public:
    uint8_t get_wrap_phase() const;
    uint64_t get_evm_height() const;
    uint64_t get_elect_height() const;

private:
    void set_wrap_phase(uint8_t phase);
    void set_evm_height(uint64_t height);
    void set_elect_height(uint64_t height);
};

NS_END2
