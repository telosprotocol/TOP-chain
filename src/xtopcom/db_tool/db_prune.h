#pragma once
//#include <string>
//#include <iostream>
//#include <sstream> 
//#include <fstream>

#include "xbasic/xtimer_driver.h"

#include "nlohmann/fifo_map.hpp"
#include "nlohmann/json.hpp"
#include "xbase/xobject_ptr.h"
#include "xvledger/xvstate.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>

#include "xgrpcservice/xgrpc_service.h"
#include "xmbus/xmessage_bus.h"
#include "xstore/xstore_face.h"
#include "xtxstore/xtxstore_face.h"
#include "xvledger/xvcnode.h"
#include "xbasic/xmemory.hpp"

NS_BEG2(top, db_prune)
class DbPrune {
private:
    std::unique_ptr<xbase_timer_driver_t> m_timer_driver;
    xobject_ptr_t<mbus::xmessage_bus_face_t> m_bus;
    xobject_ptr_t<store::xstore_face_t> m_store;
    xobject_ptr_t<base::xvblockstore_t> m_blockstore;
    xobject_ptr_t<base::xvtxstore_t> m_txstore;
    xobject_ptr_t<base::xvnodesrv_t> m_nodesvr_ptr;
    std::shared_ptr<rpc::xrpc_handle_face_t> m_getblock;


    int db_init(const std::string datadir);
    std::vector<std::string> get_db_unit_accounts();
    std::vector<std::string> get_table_accounts();
public:
    static DbPrune & instance();
    int db_prune(const std::string datadir, std::ostringstream & out_str);
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