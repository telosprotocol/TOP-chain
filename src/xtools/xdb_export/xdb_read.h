#pragma once


#include "xdb_util.h"
#include "xstore/xstore_face.h"
#include "xtxstore/xtxstore_face.h"
#include "xdb/xdb.h"
#include "xdb/xdb_factory.h"
#include "xdb/xdb_face.h"
#include "xstore/xstore.h"


NS_BEG2(top, db_read)

class xdb_read_tools_t {

public:
    xdb_read_tools_t(std::string const & db_path);
    ~xdb_read_tools_t();
    void query_block_info(std::string const & address, const uint64_t height);

private:


private:
     xobject_ptr_t<store::xstore_face_t> m_store {nullptr};
    base::xvdbstore_t*  m_xvdb_ptr { nullptr};

};

NS_END2
