#include "../xdb_read.h"
#include "xblockstore/xblockstore_face.h"

NS_BEG2(top, db_read)


xdb_read_tools_t::xdb_read_tools_t(std::string const & db_path) {
    
    int dst_db_kind = top::db::xdb_kind_kvdb | top::db::xdb_kind_readonly;
    std::shared_ptr<db::xdb_face_t> db = top::db::xdb_factory_t::create(dst_db_kind, db_path);  
    m_store = top::make_object_ptr<store::xstore>(db);
    m_xvdb_ptr =    dynamic_cast<base::xvdbstore_t*>(m_store.get());
}

xdb_read_tools_t::~xdb_read_tools_t() {
    if (m_xvdb_ptr != nullptr) {
        m_xvdb_ptr->close();
    }
}

void xdb_read_tools_t::query_block_info(std::string const & address, const uint64_t height) {
    std::cout << "start  query_block_info " << std::endl;
  
    std::vector<base::xvbindex_t*> vector_index =  store::read_block_index_db_by_path(m_xvdb_ptr,  address, height);
    for(auto it = vector_index.begin(); it != vector_index.end(); ++it) {
        base::xvbindex_t* index =  (base::xvbindex_t*)*it;

        std::cout << "cout: " << index->dump().c_str() << "block hash: " << base::xstring_utl::to_hex(index->get_block_hash()) << \
        " last_block_hash: " << base::xstring_utl::to_hex(index->get_last_block_hash()) << \
        " block_class: " <<  index->get_block_class() << std::endl;
         //" block_clock: " << index->get_this_block()->get_clock() << 
        (*it)->release_ref();   //release ptr that reference added by read_index_from_db
    }
}

NS_END2