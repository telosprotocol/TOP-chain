#pragma once
#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>

#include "xbase/xobject_ptr.h"
#include "xvledger/xvstate.h"
#include "xgrpcservice/xgrpc_service.h"
#include "xdbstore/xstore_face.h"
#include "xtxstore/xtxstore_face.h"
#include "xvledger/xvcnode.h"
#include "xvledger/xvblockstore.h"
#include "xbasic/xmemory.hpp"
#include "xutility/xhash.h"

NS_BEG2(top, db_prune)
class DbPrune {
private:
    xobject_ptr_t<store::xstore_face_t> m_store;
    xobject_ptr_t<base::xvblockstore_t> m_blockstore;
    int update_meta(base::xvaccount_t& _vaddr, const uint64_t& height);

    int db_init(const std::string datadir);
    int db_close();
    std::vector<std::string> get_db_unit_accounts();
    std::vector<std::string> get_table_accounts();
    int do_db_prune(const std::string& node_addr, const std::string& datadir, std::ostringstream & out_str);
    int db_check(const std::string& node_addr, const std::string& datadir, std::ostringstream & out_str);
public:
    static DbPrune & instance();
    int db_prune(const std::string& node_addr, const std::string& datadir, std::ostringstream & out_str);
    int db_convert(const std::string& miner_type, const std::string& datadir, std::ostringstream & out_str);
    void compact_db(const std::string datadir, std::ostringstream& out_str);
};

class xtop_hash_t : public top::base::xhashplugin_t {
public:
    xtop_hash_t()
      : base::xhashplugin_t(-1)  //-1 = support every hash types
    {
    }

private:
    xtop_hash_t(const xtop_hash_t &) = delete;
    xtop_hash_t & operator=(const xtop_hash_t &) = delete;

public:
    virtual ~xtop_hash_t(){};
    virtual const std::string hash(const std::string & input, enum_xhash_type type) override {
        xassert(type == enum_xhash_type_sha2_256);
        auto hash = utl::xsha2_256_t::digest(input);
        return std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    }
};
NS_END2