#pragma once

#include "xdb_util.h"
#include "xstore/xstore_face.h"
#include "xtxstore/xtxstore_face.h"
#include "xdb/xdb.h"
#include "xdb/xdb_factory.h"
#include "xdb/xdb_face.h"
#include "xstore/xstore.h"
#include "xblockstore/src/xvblockdb.h"

NS_BEG2(top, db_read)

class xdb_read_tools_t {

public:
    xdb_read_tools_t(std::string const & db_path);
    ~xdb_read_tools_t();
    void db_read_block(std::string const & address, const uint64_t height);
    void db_read_meta(std::string const & address);
    void db_compact_db();
private:
    xobject_ptr_t<store::xstore_face_t> m_store { nullptr };
    base::xvdbstore_t*  m_xvdb_ptr { nullptr };
    store::xvblockdb_t* m_xvblockdb_ptr { nullptr };
};

NS_END2
