// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xheader_cert.h"
#include "xdata/xdata_common.h"
#include "xdata/xrootblock.h"
#include "xdata/xnative_contract_address.h"

#include "xbase/xhash.h"
#include "xbase/xobject.h"
#include "xbase/xutl.h"
#include "xbase/xvledger.h"

#include <cinttypes>
#include <string>
NS_BEG2(top, data)

REG_CLS(xblockcert_t);

static bool check_merkle_path(const std::string & leaf, const xmerkle_path_256_t & hash_path, const std::string & root) {
    xmerkle_t<utl::xsha2_256_t, uint256_t> merkle;
    return merkle.validate_path(leaf, root, hash_path.m_levels);
}

base::xauto_ptr<base::xvheader_t> xblockheader_t::create_next_blockheader(base::xvblock_t* prev_block,
                                                         base::enum_xvblock_class block_class) {
    // const uint16_t   ledger_id = base::xvaccount_t::get_ledgerid_from_account(prev_block->get_account());
    // auto             chainid = base::xvaccount_t::get_chainid_from_ledgerid(ledger_id);
    xblockheader_t * _header = new xblockheader_t();
    _header->set_chainid(prev_block->get_chainid());
    _header->set_account(prev_block->get_account());
    _header->set_height(prev_block->get_height() + 1);
    _header->set_block_level(prev_block->get_header()->get_block_level());
    _header->set_weight(1);
    _header->set_last_block_hash(prev_block->get_block_hash());
    if (prev_block->is_genesis_block() || prev_block->get_block_class() == base::enum_xvblock_class_full) {
        _header->set_last_full_block(prev_block->get_block_hash(), prev_block->get_height());
    } else {
        _header->set_last_full_block(prev_block->get_last_full_block_hash(), prev_block->get_last_full_block_height());
    }
    _header->set_block_class(block_class);
    _header->set_block_type(prev_block->get_block_type());
    return _header;
}

base::xauto_ptr<base::xvheader_t> xblockheader_t::create_blockheader(const xblock_para_t & para) {
    // const uint16_t   ledger_id = base::xvaccount_t::get_ledgerid_from_account(para.account);
    // auto             chainid = base::xvaccount_t::get_chainid_from_ledgerid(ledger_id);
    // get chainid from rootblock instead of account.
    xblockheader_t * _header = new xblockheader_t();
    _header->set_chainid(para.chainid);
    _header->set_account(para.account);
    _header->set_height(para.height);
    _header->set_block_level(para.block_level);
    _header->set_block_class(para.block_class);
    _header->set_weight(1);
    if (!para.extra_data.empty()) {
        _header->set_extra_data(para.extra_data);
    }

    // _header->set_block_version(0);
    // _header->set_block_features(0);
    if (0 == para.height) {  // init for genesis block
        _header->set_block_type(base::enum_xvblock_type_genesis);  // must be enum_xvblock_type_genesis
        _header->set_last_block_hash(para.last_block_hash);
#ifdef DEBUG
    if (para.account != genesis_root_addr_main_chain) {
        xassert(para.last_block_hash == xrootblock_t::get_rootblock_hash());
    } else {
        xassert(para.last_block_hash == std::string());
    }
#endif
        _header->set_last_full_block(std::string(), 0);           // must be nil
    } else {
        _header->set_block_type(para.block_type);
        _header->set_last_block_hash(para.last_block_hash);
        xassert(para.last_block_hash != std::string());
        _header->set_last_full_block(para.last_full_block_hash, para.last_full_block_height);
    }
    return _header;
}

base::xauto_ptr<base::xvheader_t> xblockheader_t::create_lightunit_header(const std::string & account,
                                                         uint64_t            height,
                                                         const std::string & last_block_hash,
                                                         const std::string & justify_block_hash,
                                                         const std::string & last_full_block_hash,
                                                         uint64_t last_full_block_height) {
    xblock_para_t block_para;
    block_para.chainid     = xrootblock_t::get_rootblock_chainid();
    block_para.block_level = base::enum_xvblock_level_unit;
    block_para.block_class = base::enum_xvblock_class_light;
    block_para.block_type = base::enum_xvblock_type_general;
    block_para.account = account;
    block_para.height = height;
    block_para.last_block_hash = last_block_hash;
    block_para.justify_block_hash = justify_block_hash;
    block_para.last_full_block_hash = last_full_block_hash;
    block_para.last_full_block_height = last_full_block_height;
    return xblockheader_t::create_blockheader(block_para);
}

base::xauto_ptr<base::xvheader_t> xblockheader_t::create_fullunit_header(const std::string & account,
                                                        uint64_t            height,
                                                        const std::string & last_block_hash,
                                                        const std::string & last_full_block_hash,
                                                        const std::string & justify_block_hash,
                                                        uint64_t last_full_block_height) {
    xblock_para_t block_para;
    block_para.chainid     = xrootblock_t::get_rootblock_chainid();
    block_para.block_level = base::enum_xvblock_level_unit;
    block_para.block_class = base::enum_xvblock_class_full;
    block_para.block_type = base::enum_xvblock_type_general;
    block_para.account = account;
    block_para.height = height;
    block_para.last_block_hash = last_block_hash;
    block_para.justify_block_hash = justify_block_hash;
    block_para.last_full_block_hash = last_full_block_hash;
    block_para.last_full_block_height = last_full_block_height;
    return xblockheader_t::create_blockheader(block_para);
}

xblockheader_t::xblockheader_t() {}

xblockheader_t::~xblockheader_t() {}

base::xauto_ptr<xblockcert_t> xblockcert_t::create_blockcert(const std::string &        account,
                                              uint64_t                   height,
                                              base::enum_xconsensus_flag flag,
                                              uint64_t                   viewid,
                                              uint64_t                   clock) {
    xblockcert_t * _cert = new xblockcert_t(account, height);
    if (height != 0) {
        _cert->set_viewid(viewid);
        _cert->set_clock(clock);
        if (flag != (base::enum_xconsensus_flag)0) {
            _cert->set_consensus_flag(flag);
        }
    }
    return _cert;
}

base::enum_xvchain_key_curve xblockcert_t::get_key_curve_type_from_account(const std::string & account) {
    base::enum_xvchain_key_curve  key_curve_type;
    base::enum_vaccount_addr_type addr_type = base::xvaccount_t::get_addrtype_from_account(account);
    switch (addr_type) {
    case base::enum_vaccount_addr_type_secp256k1_user_account:
    case base::enum_vaccount_addr_type_secp256k1_user_sub_account:
    case base::enum_vaccount_addr_type_native_contract:
    case base::enum_vaccount_addr_type_custom_contract:
    case base::enum_vaccount_addr_type_black_hole:

    case base::enum_vaccount_addr_type_block_contract: {
        key_curve_type = base::enum_xvchain_key_curve_secp256k1;
    } break;

    case base::enum_vaccount_addr_type_ed25519_user_account:
    case base::enum_vaccount_addr_type_ed25519_user_sub_account: {
        key_curve_type = base::enum_xvchain_key_curve_ed25519;
    } break;

    default: {
        if (((int)addr_type >= base::enum_vaccount_addr_type_ed25519_reserved_start) && ((int)addr_type <= base::enum_vaccount_addr_type_ed25519_reserved_end)) {
            key_curve_type = base::enum_xvchain_key_curve_ed25519;
        } else {
            key_curve_type = base::enum_xvchain_key_curve_secp256k1;
        }
    } break;
    }
    return key_curve_type;
}

xblockcert_t::xblockcert_t() : base::xvqcert_t(std::string(), (enum_xdata_type)object_type_value) {
    XMETRICS_XBASE_DATA_CATEGORY_NEW(object_type_value);
}

xblockcert_t::xblockcert_t(const std::string & account, uint64_t height)
    : base::xvqcert_t(std::string(), (enum_xdata_type)object_type_value) {
    XMETRICS_XBASE_DATA_CATEGORY_NEW(object_type_value);
    if (height == 0) {
        set_consensus_type(base::enum_xconsensus_type_genesis);                 // must be enum_xconsensus_type_genesis
        set_consensus_threshold(base::enum_xconsensus_threshold_anyone);        // must be anyone
        set_crypto_sign_type(base::enum_xvchain_threshold_sign_scheme_none);    // genesis dose not need signature
        set_clock(0);                                   // clock  must be 0
        set_viewid(0);                                  // viewid must be 0
        set_viewtoken(-1);                              // must be -1
        set_validator({(uint64_t)-1, (uint64_t)-1});    // genesis block not need verify
    } else {
        set_consensus_type(base::enum_xconsensus_type_xhbft);
        set_consensus_threshold(base::enum_xconsensus_threshold_2_of_3);         // >= 2/3 vote
        set_crypto_sign_type(base::enum_xvchain_threshold_sign_scheme_schnorr);  // default schnorr scheme
    }

    set_drand(0);  // must init 0
    auto key_curve_type = xblockcert_t::get_key_curve_type_from_account(account);
    set_crypto_key_type(key_curve_type);
    set_crypto_hash_type(enum_xhash_type_sha2_256);  // default sha2_256
}

xblockcert_t::~xblockcert_t() {
    XMETRICS_XBASE_DATA_CATEGORY_DELETE(get_obj_type());
    if (m_parent_cert != nullptr) {
        m_parent_cert->release_ref();
    }
}
base::xobject_t * xblockcert_t::create_object(int type) {
    (void)type;
    return new xblockcert_t;
}

void * xblockcert_t::query_interface(const int32_t _enum_xobject_type_) {
    if (object_type_value == _enum_xobject_type_)
        return this;
    return xvqcert_t::query_interface(_enum_xobject_type_);
}

void xblockcert_t::set_parent_cert_and_path(base::xvqcert_t * parent_cert, const xmerkle_path_256_t & path) {
    m_parent_cert = parent_cert;
    m_parent_cert->add_ref();
    m_cert_path = path;
}

base::xvqcert_t * xblockcert_t::get_parent_cert() {
    if (m_parent_cert != nullptr) {
        return m_parent_cert;
    }
    check_parent_cert_and_path();
    return m_parent_cert;
}

bool xblockcert_t::extend_data_serialize_from(const std::string & extend_data, xmerkle_path_256_t & path) {
    xassert(!extend_data.empty());
    base::xstream_t _stream2(base::xcontext_t::instance(), (uint8_t *)extend_data.data(), (uint32_t)extend_data.size());
    int32_t ret = path.serialize_from(_stream2);
    if (ret <= 0) {
        xerror("xblockcert_t::extend_data_serialize_from deserialize fail. ret=%d", ret);
        return false;
    }
    return true;
}

bool xblockcert_t::check_parent_cert_and_path() {
    if (!(get_consensus_flags() & base::enum_xconsensus_flag_extend_cert)) {
        return true;
    }

    if (get_extend_cert().empty() || get_extend_data().empty()) {
        xerror("xblock_t::check_block_hash has none extend cert or data");
        return false;
    }

    if (m_parent_cert == nullptr && !get_extend_cert().empty()) {
        base::xstream_t _stream(base::xcontext_t::instance(), (uint8_t *)get_extend_cert().data(), (uint32_t)get_extend_cert().size());
        m_parent_cert = (base::xvqcert_t *)xdataunit_t::read_from(_stream);
        xassert(m_parent_cert != nullptr);

        if (!get_extend_data().empty()) {
            base::xstream_t _stream2(base::xcontext_t::instance(), (uint8_t *)get_extend_data().data(), (uint32_t)get_extend_data().size());
            m_cert_path.serialize_from(_stream2);
        }
    }

    if (m_parent_cert == nullptr) {
        xassert(0);
        return false;
    }

    const std::string leaf = get_hash_to_sign();
    return check_merkle_path(leaf, m_cert_path, m_parent_cert->get_output_root_hash());
}

NS_END2
