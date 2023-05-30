#include "../xdb_write.h"
#include "../xdbtool_util.h"
#include "xdbstore/xstore.h"
#include <json/json.h>
#include "xdata/xblockextract.h"
#include "xdata/xblocktool.h"
#include "xbasic/xhex.h"
#include "xbase/xutl.h"
#include "xblockstore/src/xunitstore.h"


NS_BEG2(top, db_export)

xdb_write_tools_t::xdb_write_tools_t(std::string const & db_path) {
    int dst_db_kind = top::db::xdb_kind_kvdb;
    std::vector<db::xdb_path_t> db_data_paths {};
    base::xvchain_t::instance().get_db_config_custom(db_data_paths, dst_db_kind);
    std::cout << "--------db_path:" << db_path << std::endl;
    std::shared_ptr<db::xdb_face_t> db = top::db::xdb_factory_t::create(dst_db_kind, db_path, db_data_paths);

    m_store = store::xstore_factory::create_store_with_static_kvdb(db);
    m_xvdb_ptr = m_store.get();
    m_xvblockdb_ptr = new store::xvblockdb_t(m_xvdb_ptr);
}

xdb_write_tools_t::~xdb_write_tools_t() {
    if (m_xvdb_ptr != nullptr) {
        m_xvdb_ptr->close();
    }

    if (m_xvblockdb_ptr != nullptr) {
        m_xvblockdb_ptr->release_ref();
    }
}

bool xdb_write_tools_t::is_match_function_name(std::string const & func_name) {
    static std::vector<std::string> names = {
        "correct_all_txindex",
        "correct_one_txindex",
        "correct_table_block_units",
    };

    for (auto & v : names) {
        if (v == func_name) {
            return true;
        }
    }
    std::cout << "xdb_write_tools_t unmatch" << std::endl; // TODO(jimmy)
    return false;
}

bool xdb_write_tools_t::process_function(std::string const & func_name, int argc, char ** argv) {
    if (func_name == "correct_all_txindex") {
        if (argc != 3) return false;
        correct_all_txindex();
    } else if (func_name == "correct_one_txindex") {
        if (argc != 4) return false;
        correct_one_txindex(argv[3]);
    } else if (func_name == "correct_table_block_units") {
        if (argc != 5) return false;
        correct_table_block_units(argv[3], std::stoi(argv[4]));        
    } else {
        xassert(false);
        std::cout << "ERROR not find function name" << std::endl;
        return false;
    }
    return true;    
}

void xdb_write_tools_t::correct_one_table_txindex(std::string const& table_addr, uint64_t & finish_height) {
    base::xvaccount_t _vaddr(table_addr);
    uint32_t changed_txindex = 0;
    uint32_t total_txindex = 0;
    int64_t begin = top::base::xtime_utl::gmttime_ms();

    for (uint64_t height = 1; ;height++) {
        xobject_ptr_t<base::xvbindex_t> bindex = m_xvblockdb_ptr->load_committed_index_from_db(_vaddr, height);
        if (nullptr == bindex) {
            finish_height = height;
            int64_t finish = top::base::xtime_utl::gmttime_ms();
            std::cout << "table=" << table_addr << " height=" << height << " finish load" << 
            " changed=" << changed_txindex << " total=" << total_txindex << " time_ms=" << finish-begin << std::endl;
            return;
        }

        if (bindex->get_block_class() == base::enum_xvblock_class_light) {
            if (false == m_xvblockdb_ptr->load_block_object(bindex.get())) {
                std::cout << "table=" << table_addr << " height=" << height << " ERROR load block object" << std::endl;
                return;
            }
            if (false == m_xvblockdb_ptr->load_block_input(bindex.get())) {
                std::cout << "table=" << table_addr << " height=" << height << " ERROR load block input" << std::endl;
                return;
            }

            std::vector<xobject_ptr_t<base::xvtxindex_t>> sub_txs;
            if (false == bindex->get_this_block()->extract_sub_txs(sub_txs)) {
                std::cout << "table=" << table_addr << " height=" << height << " ERROR extract_sub_txs" << std::endl;
                return;                
            }

            for (auto & v : sub_txs) {
                total_txindex++;
                base::enum_txindex_type txindex_type = base::xvtxkey_t::transaction_subtype_to_txindex_type(v->get_tx_phase_type());
                const std::string tx_key = base::xvdbkey_t::create_tx_index_key(v->get_tx_hash(), txindex_type);
                std::string tx_idx_bin = m_xvdb_ptr->get_value(tx_key);
                if (tx_idx_bin.empty()) {
                    std::cout << "table=" << table_addr << " height=" << height << " ERROR load txindex " << base::xvtxkey_t::transaction_hash_subtype_to_string(v->get_tx_hash(), v->get_tx_phase_type()) << std::endl;
                    return;
                }

                base::xauto_ptr<base::xvtxindex_t> txindex(new base::xvtxindex_t());
                if (txindex->serialize_from_string(tx_idx_bin) <= 0) {
                    std::cout << "table=" << table_addr << " height=" << height << " ERROR deserialize txindex " << base::xvtxkey_t::transaction_hash_subtype_to_string(v->get_tx_hash(), v->get_tx_phase_type()) << std::endl;
                    return;
                }
                // txindex->set_tx_hash(txhash);

                if (bindex->get_block_hash() != txindex->get_block_hash()) {
                    txindex->set_block_hash(bindex->get_block_hash());
                    std::string new_tx_indx_bin;
                    txindex->serialize_to_string(new_tx_indx_bin);
                    if (false == m_xvdb_ptr->set_value(tx_key, new_tx_indx_bin)) {
                        std::cout << "table=" << table_addr << " height=" << height << " ERROR set db txindex " << base::xvtxkey_t::transaction_hash_subtype_to_string(v->get_tx_hash(), v->get_tx_phase_type()) << std::endl;
                        return;
                    }
                    changed_txindex++;                 
                }
            }
        }
    }
}

void xdb_write_tools_t::correct_all_txindex() {
    int64_t begin = top::base::xtime_utl::gmttime_ms();
    uint64_t total_block_count = 0;
    uint64_t finish_height = 0;
    auto all_tables = data::xblocktool_t::make_all_table_addresses();
    for (auto & addr : all_tables) {
        correct_one_table_txindex(addr, finish_height);
        total_block_count += finish_height;
    }
    int64_t finish = top::base::xtime_utl::gmttime_ms();
    std::cout << "total_blocks=" << total_block_count << " time_ms=" << finish-begin << std::endl;
}

void xdb_write_tools_t::correct_one_phase_txindex(std::string const & hex_txhash, base::enum_txindex_type txindex_type) {
    std::string txhash = top::to_string(top::from_hex(hex_txhash));
    const std::string tx_key = base::xvdbkey_t::create_tx_index_key(txhash, txindex_type);
    std::string tx_idx_bin = m_xvdb_ptr->get_value(tx_key);
    if (tx_idx_bin.empty()) {
        std::cout << "fail load txindex=" << hex_txhash << " txindex_type=" << (uint32_t)txindex_type << std::endl;
        return;
    }
    base::xauto_ptr<base::xvtxindex_t> txindex(new base::xvtxindex_t());
    if (txindex->serialize_from_string(tx_idx_bin) <= 0) {
        std::cout << "fail unserialize from txindex " << hex_txhash << " txindex_type=" << (uint32_t)txindex_type << std::endl;
        return;
    }
    txindex->set_tx_hash(txhash);
    std::cout << "load txindex " << txindex->dump() << std::endl;

    std::vector<base::xvbindex_t*> vector_index = m_xvblockdb_ptr->load_index_from_db(txindex->get_block_addr(), txindex->get_block_height());
    for (auto & bindex : vector_index) {
        if (bindex->check_block_flag(base::enum_xvblock_flag_committed)) {
            if (bindex->get_block_hash() != txindex->get_block_hash()) {
                txindex->set_block_hash(bindex->get_block_hash());
                std::string new_tx_indx_bin;
                if (txindex->serialize_to_string(new_tx_indx_bin) <= 0) {
                    std::cout << "ERROR serialize txindex " << txindex->dump() << std::endl;
                    continue;
                }
                if (false == m_xvdb_ptr->set_value(tx_key, new_tx_indx_bin)) {
                    std::cout << "ERROR set db txindex " << txindex->dump() << std::endl;
                    continue;
                }
                std::cout << "changed txindex " << txindex->dump() << std::endl;                 
            }
        }

        bindex->release_ref();   //release ptr that reference added by read_index_from_db
    }
}


void xdb_write_tools_t::correct_one_txindex(std::string const & hex_txhash) {
    correct_one_phase_txindex(hex_txhash, base::enum_txindex_type_send);
    correct_one_phase_txindex(hex_txhash, base::enum_txindex_type_receive);
    correct_one_phase_txindex(hex_txhash, base::enum_txindex_type_confirm);
}

xobject_ptr_t<base::xvblock_t> xdb_write_tools_t::load_commit_table_block(std::string const& table_addr, uint64_t height) {
    base::xvaccount_t _vaddr(table_addr);
    xobject_ptr_t<base::xvbindex_t> bindex = m_xvblockdb_ptr->load_committed_index_from_db(_vaddr, height);
    if (nullptr == bindex) {
        std::cout << "xdb_write_tools_t::load_commit_table_block FAIL load index. addr=" << table_addr << " height=" << height << std::endl;
        return nullptr;
    }
    if (false == m_xvblockdb_ptr->load_block_object(bindex.get())) {
        std::cout << "xdb_write_tools_t::load_commit_table_block FAIL load object. table=" << table_addr << " height=" << height << std::endl;
        return nullptr;
    }
    if (false == m_xvblockdb_ptr->load_block_input(bindex.get())) {
        std::cout << "xdb_write_tools_t::load_commit_table_block FAIL load input. table=" << table_addr << " height=" << height << std::endl;
        return nullptr;
    }
    if (false == m_xvblockdb_ptr->load_block_output(bindex.get())) {
        std::cout << "xdb_write_tools_t::load_commit_table_block FAIL load output. table=" << table_addr << " height=" << height << std::endl;
        return nullptr;
    }
    if (false == m_xvblockdb_ptr->load_block_output_offdata(bindex.get(), bindex->get_this_block())) {
        std::cout << "xdb_write_tools_t::load_commit_table_block FAIL load offdata. table=" << table_addr << " height=" << height << std::endl;
        return nullptr;
    }
    xobject_ptr_t<base::xvblock_t> block;
    bindex->get_this_block()->add_ref();
    block.attach(bindex->get_this_block());
    return block;
}

void xdb_write_tools_t::correct_table_block_units(std::string const& table_addr, uint64_t height) {
    xobject_ptr_t<base::xvblock_t> tableblock = load_commit_table_block(table_addr, height);
    if (tableblock == nullptr) {
        std::cout << "xdb_write_tools_t::correct_table_block_units FAIL load block. table=" << table_addr << " height=" << height << std::endl;
        return;
    }
    store::xunitstore_t unitstore(m_xvblockdb_ptr);
    bool ret = unitstore.store_units(tableblock.get());
    if (!ret) {
        std::cout << "xdb_write_tools_t::correct_table_block_units FAIL store units. table=" << table_addr << " height=" << height << std::endl;
        return;
    }
    std::cout << "xdb_write_tools_t::correct_table_block_units SUCC. table=" << table_addr << " height=" << height << std::endl;
}


NS_END2
