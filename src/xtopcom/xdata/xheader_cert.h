// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>
#include "xvledger/xdataobj_base.hpp"
#include "xvledger/xvblock.h"
#include "xvledger/xvaccount.h"
#include "xutility/xmerkle.hpp"

NS_BEG2(top, data)

class xblock_para_t {
 public:
    base::enum_xchain_id        chainid;
    base::enum_xvblock_level    block_level;
    base::enum_xvblock_class    block_class;
    base::enum_xvblock_type     block_type;
    std::string                 account;
    uint64_t                    height{0};
    std::string                 last_block_hash;
    std::string                 justify_block_hash;
    std::string                 last_full_block_hash;
    uint64_t                    last_full_block_height{0};
    std::string                 extra_data;
};

class xblockheader_t : public base::xvheader_t {
    friend class xblock_t;
 public:
    static base::xauto_ptr<base::xvheader_t> create_next_blockheader(base::xvblock_t* prev_block,
                                                    base::enum_xvblock_class block_class);
    static base::xauto_ptr<base::xvheader_t> create_blockheader(const xblock_para_t & para);
    static base::xauto_ptr<base::xvheader_t> create_lightunit_header(const std::string & account,
                                                    uint64_t height,
                                                    const std::string & last_block_hash,
                                                    const std::string & justify_block_hash,
                                                    const std::string & last_full_block_hash,
                                                    uint64_t last_full_block_height);
    static base::xauto_ptr<base::xvheader_t> create_fullunit_header(const std::string & account,
                                                    uint64_t height,
                                                    const std::string & last_block_hash,
                                                    const std::string & last_full_block_hash,
                                                    const std::string & justify_block_hash,
                                                    uint64_t last_full_block_height);
 protected:
    xblockheader_t();
    virtual ~xblockheader_t();
 private:
    xblockheader_t(const xblockheader_t &);
    xblockheader_t & operator = (const xblockheader_t &);
};

class xblockcert_t : public base::xvqcert_t {
    friend class xblock_t;
 protected:
    enum { object_type_value = enum_xdata_type::enum_xdata_type_max - xdata_type_block_cert };
 public:
    static base::xauto_ptr<xblockcert_t> create_blockcert(const std::string & account,
                                        uint64_t height,
                                        base::enum_xconsensus_flag flag,
                                        uint64_t viewid,
                                        uint64_t clock);
    static base::enum_xvchain_key_curve    get_key_curve_type_from_account(const std::string & account);
 public:
    xblockcert_t();
    xblockcert_t(const std::string & account, uint64_t height);
 protected:
    virtual ~xblockcert_t();
 private:
    xblockcert_t(const xblockcert_t &);
    xblockcert_t & operator = (const xblockcert_t &);
 public:
    static int32_t get_object_type() {return object_type_value;}
    static xobject_t *create_object(int type);
    void *query_interface(const int32_t _enum_xobject_type_) override;

 public:  // set block cert member
    void                set_consensus_flag(base::enum_xconsensus_flag flag) {base::xvqcert_t::set_consensus_flag(flag);}

 public:
    void                set_parent_cert_and_path(base::xvqcert_t* parent_cert, const xmerkle_path_256_t & path);
    bool                check_parent_cert_and_path();
    base::xvqcert_t*    get_parent_cert();
    static bool         extend_data_serialize_from(const std::string & extend_data, xmerkle_path_256_t & path);

 private:
    base::xvqcert_t*    m_parent_cert{nullptr};
    xmerkle_path_256_t  m_cert_path;
};

NS_END2
