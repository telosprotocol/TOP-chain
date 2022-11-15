#pragma once

#include "xdb_util.h"

#include "xtxstore/xtxstore_face.h"
#include "xdb/xdb.h"
#include "xdb/xdb_factory.h"
#include "xdb/xdb_face.h"
#include "xdbstore/xstore_face.h"
#include "xblockstore/src/xvblockdb.h"

NS_BEG2(top, db_export)

class xdb_write_tools_t {
public:
    xdb_write_tools_t(std::string const & db_path);
    ~xdb_write_tools_t();

    static bool is_match_function_name(std::string const & func_name);
    bool process_function(std::string const & func_name, int argc, char ** argv);

protected:
    void correct_one_txindex(std::string const & hex_txhash);
    void correct_one_phase_txindex(std::string const & hex_txhash, base::enum_txindex_type txindex_type);
    void correct_all_txindex();
    void correct_one_table_txindex(std::string const& table_addr, uint64_t & finish_height);
private:
    xobject_ptr_t<store::xstore_face_t> m_store { nullptr };
    base::xvdbstore_t*  m_xvdb_ptr { nullptr };
    store::xvblockdb_t* m_xvblockdb_ptr { nullptr };
};

NS_END2
