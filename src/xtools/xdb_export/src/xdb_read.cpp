#include "../xdb_read.h"


NS_BEG2(top, db_read)

xdb_read_tools_t::xdb_read_tools_t(std::string const & db_path) {
    int dst_db_kind = top::db::xdb_kind_kvdb | top::db::xdb_kind_readonly;
    std::shared_ptr<db::xdb_face_t> db = top::db::xdb_factory_t::create(dst_db_kind, db_path);  
    m_store = top::make_object_ptr<store::xstore>(db);
    m_xvdb_ptr = dynamic_cast<base::xvdbstore_t*>(m_store.get());
    m_xvblockdb_ptr = new store::xvblockdb_t(m_xvdb_ptr);
}

xdb_read_tools_t::~xdb_read_tools_t() {
    if (m_xvdb_ptr != nullptr) {
        m_xvdb_ptr->close();
    }

    if (m_xvblockdb_ptr != nullptr) {
        m_xvblockdb_ptr->release_ref();
    }
}

void xdb_read_tools_t::db_read_block(std::string const & address, const uint64_t height) {
    std::cout << "db_read_block start: " << std::endl;
    if (m_xvblockdb_ptr != nullptr) {
        std::vector<base::xvbindex_t*> vector_index = m_xvblockdb_ptr->load_index_from_db(address, height);
        for (auto it = vector_index.begin(); it != vector_index.end(); ++it) {
            base::xvbindex_t* index = (base::xvbindex_t*)*it;
            std::cout << index->dump().c_str() << ", block_hash=" << base::xstring_utl::to_hex(index->get_block_hash()) << \
            ", last_block_hash=" << base::xstring_utl::to_hex(index->get_last_block_hash()) <<  ", block_clock=" << index->get_this_block()->get_clock() << \
            ", block_class=" <<  index->get_block_class() << std::endl;
            (*it)->release_ref();   //release ptr that reference added by read_index_from_db
        }
    }

}

void xdb_read_tools_t::db_read_meta(std::string const & address) {
    std::cout << "db_read_meta start: " << std::endl;
    if (m_xvdb_ptr != nullptr) {
        base::xvaccount_t _vaddr{address};
        std::string new_meta_key = base::xvdbkey_t::create_account_meta_key(_vaddr);
        std::string value = m_xvdb_ptr->get_value(new_meta_key);
        base::xvactmeta_t* _meta = new base::xvactmeta_t(_vaddr);  // create empty meta default
        if (!value.empty()) {
            if (_meta->serialize_from_string(value) <= 0) {
                std::cerr << "meta serialize_from_string fail !!!" << std::endl;
            } else {
                std::cout << "meta serialize_from_string succ,meta=" << _meta->clone_block_meta().ddump() << std::endl;
            }
        } else {
            std::cerr << "meta value empty !!!" << std::endl;
        }    
    }
}

NS_END2