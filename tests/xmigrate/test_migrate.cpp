#include <stdio.h>
#include "gtest/gtest.h"
#include "xvledger/xvdbkey.h"
#include "xvledger/xvtxindex.h"
#include "xvledger/xvbindex.h"
#include "xmigrate/xvmigrate.h"
#include "xdb/xdb_factory.h"
#include "xvledger/xvdbkey.h"
#include "tests/mock/xdatamock_address.hpp"

using namespace top;
using namespace top::base;

class test_migrate : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

void delete_folder(const std::string & path)
{
    rmdir(path.c_str());
}

void set_common_config(top::base::xvconfig_t* sys_config_ptr, const std::string & src_db_path, const std::string & dst_db_path)
{
    //configure bootstrap
    sys_config_ptr->set_config("system.version", "0.0.0.1");
    sys_config_ptr->set_config("system.boot.size", "1");
    //configure db migrate as bootstrap
    sys_config_ptr->set_config("system.boot.0.object_key", "/init/migrate/db" );
    sys_config_ptr->set_config("system.boot.0.object_version","0.0.0.1");
    //configu db filter options
    
    sys_config_ptr->set_config("/init/migrate/db/src_path", src_db_path );
    sys_config_ptr->set_config("/init/migrate/db/dst_path", dst_db_path );
    sys_config_ptr->set_config("/init/migrate/db/dst_version", "3.0.0.0" );
    sys_config_ptr->set_config("/init/migrate/db/size", "3" );
    // sys_config_ptr->set_config("/init/migrate/db/0/object_key","/init/migrate/db/kvfilter");
    // sys_config_ptr->set_config("/init/migrate/db/1/object_key","/init/migrate/db/blkfilter");
    // sys_config_ptr->set_config("/init/migrate/db/2/object_key","/init/migrate/db/txsfilter");    

    sys_config_ptr->set_config("/init/migrate/db/0/object_key","/init/migrate/db/blkfilter");
    sys_config_ptr->set_config("/init/migrate/db/1/object_key","/init/migrate/db/txsfilter");    
    sys_config_ptr->set_config("/init/migrate/db/2/object_key","/init/migrate/db/kvfilter"); 
}

TEST_F(test_migrate, version_config) {
    {
        std::string str = "3.0.0.1";
        uint32_t version = xsysobject_t::string_to_version(str);
        ASSERT_EQ(version, 0x03000001);
        std::string str2 = xsysobject_t::version_to_string(0x03000001);
        ASSERT_EQ(str, str2);
    }
    {
        std::string str = "0.0.0.1";
        uint32_t version = xsysobject_t::string_to_version(str);
        ASSERT_EQ(version, 0x00000001);
        std::string str2 = xsysobject_t::version_to_string(0x00000001);
        ASSERT_EQ(str, str2);
    }
}

TEST_F(test_migrate, db_version_control_no_src_dst_db_path) {
    std::string src_db_path = "./db_v2_0";
    std::string dst_db_path = "./db_v3_0";
    delete_folder(src_db_path);
    delete_folder(dst_db_path);

    top::base::xvconfig_t* sys_config_ptr = new top::base::xvconfig_t();
    set_common_config(sys_config_ptr, src_db_path, dst_db_path);
    top::base::init_migrate();

    top::base::xsysobject_t * init_module = top::base::xvsyslibrary::instance(). create_object(top::base::xvsysinit_t::get_register_key());
    xassert(init_module->init(*sys_config_ptr) == enum_xcode_successful);
    init_module->start();
}

#if 0
TEST_F(test_migrate, key_migrate_1_mormal_kvs) {
    std::string src_db_path = "./db_v2_1";
    std::string dst_db_path = "./db_v3_1";
    uint32_t count = 10;
    {
        std::shared_ptr<db::xdb_face_t> old_db = db::xdb_factory_t::create(db::xdb_kind_kvdb,src_db_path);
        for (uint32_t i=0; i<count;i++) {
            std::string key= "key"+base::xstring_utl::tostring(i);
            std::string value = "value"+base::xstring_utl::tostring(i);
            old_db->write(key, value);
        }
    }

    top::base::xvconfig_t* sys_config_ptr = new top::base::xvconfig_t();
    set_common_config(sys_config_ptr, src_db_path, dst_db_path);
    top::base::init_migrate();
    top::base::xsysobject_t * init_module = top::base::xvsyslibrary::instance(). create_object(top::base::xvsysinit_t::get_register_key());
    xassert(init_module->init(*sys_config_ptr) == enum_xcode_successful);
    init_module->start();

    {
        std::shared_ptr<db::xdb_face_t> new_db = db::xdb_factory_t::create(db::xdb_kind_kvdb,dst_db_path);
        std::string value;
        for (uint32_t i=0; i<count;i++) {
            std::string key= "key"+base::xstring_utl::tostring(i);
            std::string expect_value = "value"+base::xstring_utl::tostring(i);
            new_db->read(key, value);
            ASSERT_EQ(value, expect_value);
        }
    }    
}

TEST_F(test_migrate, key_migrate_2_txindex_kvs) {
    std::string src_db_path = "./db_v2_2";
    std::string dst_db_path = "./db_v3_2";
    uint32_t count = 10;

    std::string unit_addr = mock::xdatamock_address::make_user_address_random();
    base::xvaccount_t _unit_vaddr(unit_addr);
    {
        std::shared_ptr<db::xdb_face_t> old_db = db::xdb_factory_t::create(db::xdb_kind_kvdb,src_db_path);
        for (uint32_t i=0; i<count;i++) {
            std::string raw_tx_hash = "tx_hash" + base::xstring_utl::tostring(i);
            std::string key= xvdbkey_t::create_tx_index_key(raw_tx_hash, base::enum_txindex_type_confirm);

            base::xauto_ptr<base::xvtxindex_t> txindex(new base::xvtxindex_t());
            txindex->set_block_addr(unit_addr);
            txindex->set_block_height((uint64_t)i);
            std::string value;
            txindex->serialize_to_string(value);
            old_db->write(key, value);

            base::xauto_ptr<base::xvbindex_t> unitbindex = new base::xvbindex_t();
            unitbindex->reset_account_addr(_unit_vaddr);

            std::string unitbindex_key = xvdbkey_t::create_block_index_key(_unit_vaddr, txindex->get_block_height());
            std::string unitbindex_value;
            unitbindex->serialize_to(unitbindex_value);
            old_db->write(unitbindex_key, unitbindex_value);
        }
    }

    top::base::xvconfig_t* sys_config_ptr = new top::base::xvconfig_t();
    set_common_config(sys_config_ptr, src_db_path, dst_db_path);
    top::base::init_migrate();
    top::base::xsysobject_t * init_module = top::base::xvsyslibrary::instance(). create_object(top::base::xvsysinit_t::get_register_key());
    xassert(init_module->init(*sys_config_ptr) == enum_xcode_successful);
    init_module->start();

    {
        std::shared_ptr<db::xdb_face_t> new_db = db::xdb_factory_t::create(db::xdb_kind_kvdb,dst_db_path);
        std::string value;
        for (uint32_t i=0; i<count;i++) {
            std::string raw_tx_hash = "tx_hash" + base::xstring_utl::tostring(i);
            std::string key= xvdbkey_t::create_tx_index_key(raw_tx_hash, base::enum_txindex_type_confirm);
            std::string expect_value;
            new_db->read(key, value);
            ASSERT_EQ(value.empty(), false);
            // ASSERT_EQ(value, expect_value);
        }
    }    
}

TEST_F(test_migrate, key_migrate_1_all_kvs_BENCH_1000000) {
    std::string src_db_path = "./db_v2_3";
    std::string dst_db_path = "./db_v3_3";
    uint32_t count = 1000000;
    {
        std::shared_ptr<db::xdb_face_t> old_db = db::xdb_factory_t::create(db::xdb_kind_kvdb,src_db_path);
        for (uint32_t i=0; i<count;i++) {
            std::string key= "key"+base::xstring_utl::tostring(i);
            std::string value = "value"+base::xstring_utl::tostring(i);
            old_db->write(key, value);
        }
    }

    top::base::xvconfig_t* sys_config_ptr = new top::base::xvconfig_t();
    set_common_config(sys_config_ptr, src_db_path, dst_db_path);
    top::base::init_migrate();
    top::base::xsysobject_t * init_module = top::base::xvsyslibrary::instance(). create_object(top::base::xvsysinit_t::get_register_key());
    xassert(init_module->init(*sys_config_ptr) == enum_xcode_successful);
    init_module->start();

    {
        std::shared_ptr<db::xdb_face_t> new_db = db::xdb_factory_t::create(db::xdb_kind_kvdb,dst_db_path);
        std::string value;
        for (uint32_t i=0; i<count;i++) {
            std::string key= "key"+base::xstring_utl::tostring(i);
            std::string expect_value = "value"+base::xstring_utl::tostring(i);
            new_db->read(key, value);
            ASSERT_EQ(value, expect_value);
        }
    }    
}
#endif


