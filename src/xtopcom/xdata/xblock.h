// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>
#include <map>
#include "json/json.h"
#include "xbasic/xns_macro.h"
#include "xbase/xvblock.h"
#include "xdata/xdata_common.h"
#include "xbasic/xobject_ptr.h"
#include "xdata/xnative_property.h"
#include "xdata/xheader_cert.h"
#include "xdata/xpropertylog.h"
#include "xdata/xaccount_mstate.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xlightunit_info.h"

NS_BEG2(top, data)

class xblock_consensus_para_t {
 public:
    xblock_consensus_para_t() = default;
    xblock_consensus_para_t(uint64_t clock,
                            const xvip2_t & validator,
                            const xvip2_t & auditor,
                            uint32_t viewtoken,
                            uint64_t viewid,
                            uint64_t drand_height)
    : m_clock(clock), m_validator(validator), m_auditor(auditor),
      m_viewtoken(viewtoken), m_viewid(viewid), m_drand_height(drand_height) {
    }
    xblock_consensus_para_t(const xvip2_t & validator, base::xvblock_t* prev_block);

    void set_common_consensus_para(uint64_t clock,
                                   const xvip2_t & validator,
                                   const xvip2_t & auditor,
                                   uint64_t viewid,
                                   uint32_t m_viewtoken);

    void set_tableblock_consensus_para(uint64_t timestamp,
                                       uint64_t drand_height,
                                       const std::string & random_seed,
                                       uint64_t total_lock_tgas_token,
                                       const std::string & extra_data);

    const std::string & get_extra_data() const {return m_extra_data;}
    const std::string & get_random_seed() const {return m_random_seed;}
    uint64_t get_clock() const {return m_clock;}
    uint64_t get_viewid() const {return m_viewid;}
    uint32_t get_viewtoken() const {return m_viewtoken;}
    uint64_t get_timestamp() const {return m_timestamp;}
    uint64_t get_drand_height() const {return m_drand_height;}
    const xvip2_t & get_validator() const {return m_validator;}
    const xvip2_t & get_auditor() const {return m_auditor;}
    uint64_t get_total_lock_tgas_token() const {return m_total_lock_tgas_token;}

 private:
    uint64_t    m_clock{0};
    xvip2_t     m_validator;
    xvip2_t     m_auditor;
    uint32_t    m_viewtoken{0};
    uint64_t    m_viewid{0};
    uint64_t    m_timestamp{0};
    uint64_t    m_drand_height{0};
    std::string m_random_seed;
    uint64_t    m_total_lock_tgas_token{0};
    std::string m_extra_data;
};

class xblockheader_extra_data_t : public xserializable_based_on<void> {
 protected:
    enum xblockheader_extra_data_type : uint16_t {
        enum_extra_data_type_tgas_total_lock_amount_property_height = 0,
    };

 public:
    int32_t do_write(base::xstream_t & stream) const override;
    int32_t do_read(base::xstream_t & stream) override;

    int32_t deserialize_from_string(const std::string & extra_data);
    int32_t serialize_to_string(std::string & extra_data);

 public:
    uint64_t get_tgas_total_lock_amount_property_height() const;
    void     set_tgas_total_lock_amount_property_height(uint64_t height);

 private:
    std::map<uint16_t, std::string>  m_paras;
};

class xblock_t : public base::xvblock_t {
 public:
    static bool check_merkle_path(const std::string &leaf, const xmerkle_path_256_t &hash_path, const std::string & root);
    static std::string get_block_base_path(base::xvblock_t* block) {return block->get_account() + ':' + std::to_string(block->get_height());}
 public:
    xblock_t(enum_xdata_type type);
    xblock_t(base::xvheader_t & header, xblockcert_t & cert, enum_xdata_type type);
    // xblock_t(base::xvheader_t & header, xblockcert_t & cert, const std::string & input, const std::string & output, enum_xdata_type type);
    xblock_t(base::xvheader_t & header, xblockcert_t & cert, const xinput_ptr_t & input, const xoutput_ptr_t & output, enum_xdata_type type);


#ifdef XENABLE_PSTACK  // tracking memory
    virtual int32_t add_ref() override;
    virtual int32_t release_ref() override;
#endif

 protected:
    virtual ~xblock_t();
 private:
    xblock_t(const xblock_t &);
    xblock_t & operator = (const xblock_t &);

 protected:
    xblockcert_t*       get_blockcert() const {return (xblockcert_t*)get_cert();}

 public:
    virtual int32_t     full_block_serialize_to(base::xstream_t & stream);  // for block sync
    static  base::xvblock_t*    full_block_read_from(base::xstream_t & stream);  // for block sync
    void        set_parent_cert_and_path(base::xvqcert_t* parent_cert, const xmerkle_path_256_t & path);
    bool        calc_input_merkle_path(const std::string & leaf, xmerkle_path_256_t& hash_path) const;
    bool        calc_output_merkle_path(const std::string & leaf, xmerkle_path_256_t& hash_path) const;
    bool        check_block_hash();

 public:
    void            set_consensus_para(const xblock_consensus_para_t & para);

 public:
    inline base::enum_xvblock_level get_block_level() const {return get_header()->get_block_level();}
    inline base::enum_xvblock_class get_block_class() const {return get_header()->get_block_class();}
    inline bool     is_unitblock() const {return get_block_level() == base::enum_xvblock_level_unit;}
    inline bool     is_tableblock() const {return get_block_level() == base::enum_xvblock_level_table;}
    inline bool     is_lightunit() const {return get_block_level() == base::enum_xvblock_level_unit && get_block_class() == base::enum_xvblock_class_light;}
    inline bool     is_fullunit() const {return get_block_level() == base::enum_xvblock_level_unit && get_block_class() == base::enum_xvblock_class_full;}
    inline bool     is_emptyblock() const {return get_block_class() == base::enum_xvblock_class_nil;}
    std::string     get_block_hash_hex_str() const;
    std::string     dump_header() const;
    std::string     dump_cert() const;
    virtual std::string     dump_body() const;
    std::string     dump_cert(base::xvqcert_t* qcert) const;

 public:
    virtual const std::map<std::string, std::string> & get_property_hash_map() const {return m_empty_map;}
    virtual std::string                 get_property_hash(const std::string & prop_name) const {return m_empty_string;}
    virtual const xnative_property_t &  get_native_property() const {return m_empty_native;}
    virtual const std::vector<xlightunit_tx_info_ptr_t> & get_txs() const { return m_empty_txs;}
    virtual xlightunit_tx_info_ptr_t    get_tx_info(const std::string & txhash) const;
    virtual xaccount_binlog_t*          get_property_log() const {return nullptr;}
    virtual int64_t                     get_pledge_balance_change_tgas() const {return 0;}
    virtual uint32_t                    get_txs_count() const {return 0;}
    virtual int64_t                     get_balance_change() const {return 0;}
    virtual int64_t                     get_burn_balance_change() const {return 0;}
    virtual const xaccount_mstate2*     get_fullunit_mstate() const {return nullptr;}
    virtual const std::map<std::string, std::string> * get_fullunit_propertys() const {return nullptr;}
    virtual const std::vector<xobject_ptr_t<xblock_t>> & get_tableblock_units(bool need_parent_cert) const {return m_empty_blocks;}
    virtual void                        dump_block_data(xJson::Value & json) const {return;}

 public:
    uint64_t    get_timerblock_height() const {return get_clock();}
    std::string get_block_owner()const {return get_account();}
    uint64_t    get_timestamp() {return get_cert()->get_gmtime();}

 private:
    static std::map<std::string, std::string>      m_empty_map;
    static xnative_property_t                      m_empty_native;
    static std::vector<xlightunit_tx_info_ptr_t>   m_empty_txs;
    static uint256_t                               m_empty_uint256;
    static std::string                             m_empty_string;
    static std::vector<xobject_ptr_t<xblock_t>>    m_empty_blocks;
};

using xblock_ptr_t = xobject_ptr_t<xblock_t>;
using xvblock_ptr_t = xobject_ptr_t<base::xvblock_t>;
using xvheader_ptr_t = xobject_ptr_t<base::xvheader_t>;

NS_END2
