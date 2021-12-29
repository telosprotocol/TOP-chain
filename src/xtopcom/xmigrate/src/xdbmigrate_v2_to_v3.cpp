// Copyright (c) 2018-Present Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iostream>
#include <atomic>
#include <thread>
#include "xvmigrate.h"
#include "xdbmigrate.h"
#include "xblockdb_v2.h"
#include "xvledger/xvledger.h"
#include "xdata/xblocktool.h"
#include "xdata/xnative_contract_address.h"

namespace top
{
    namespace base
    {
        #define     DB_VERSION_V3   "0.3.0.0"

        bool db_migrate_v2_to_v3(const std::string & src_db_path, const std::string & dst_db_path)
        {    
            // if src db path not exist, no need do migrate
            if (!base::xfile_utl::folder_exist(src_db_path)) {
                xwarn("db_migrate_v2_to_v3, src db path folder not exist.for src db path(%s)",src_db_path.c_str());
                return true;
            }

            xkinfo("db_migrate_v2_to_v3 begin.src_db_path=%s,dst_db_path=%s",src_db_path.c_str(),dst_db_path.c_str());
            std::cout << "db_migrate_v2_to_v3 begin" << std::endl;
            const std::string dst_db_version = DB_VERSION_V3;
            base::xauto_ptr<top::base::xvconfig_t> sys_config_ptr = new top::base::xvconfig_t();
            //configure bootstrap
            sys_config_ptr->set_config("system.version", "0.0.0.1");
            sys_config_ptr->set_config("system.boot.size", "1");
            //configure db migrate as bootstrap
            sys_config_ptr->set_config("system.boot.0.object_key", "/init/migrate/db" );
            sys_config_ptr->set_config("system.boot.0.object_version","0.0.0.1");
            //configu db filter options
            sys_config_ptr->set_config("/init/migrate/db/src_path", src_db_path );
            sys_config_ptr->set_config("/init/migrate/db/dst_path", dst_db_path );
            sys_config_ptr->set_config("/init/migrate/db/dst_version", dst_db_version );
            // sys_config_ptr->set_config("/init/migrate/db/size", "3" );
            // sys_config_ptr->set_config("/init/migrate/db/0/object_key","/init/migrate/db/blkfilter");
            // sys_config_ptr->set_config("/init/migrate/db/1/object_key","/init/migrate/db/txsfilter");
            // sys_config_ptr->set_config("/init/migrate/db/2/object_key","/init/migrate/db/kvfilter");
            sys_config_ptr->set_config("/init/migrate/db/size", "1" );
            sys_config_ptr->set_config("/init/migrate/db/0/object_key","/init/migrate/db/kvfilter");

            top::base::init_migrate();
            top::base::xsysobject_t * init_module = top::base::xvsyslibrary::instance(). create_object(top::base::xvsysinit_t::get_register_key());
            if (enum_xcode_successful != init_module->init(*sys_config_ptr.get()))
            {
                xerror("db_migrate_v2_to_v3 fail-module init");
                return false;
            }
            if (false == init_module->start())
            {
                xerror("db_migrate_v2_to_v3 fail-module start");
                return false;
            }
            xkinfo("db_migrate_v2_to_v3 finish.");
            return true;
        }


        uint64_t get_v2_db_genesis_height(const base::xvaccount_t & address, base::xvdbstore_t* _dst_dbstore) {
            uint64_t height = xblockdb_v2_t::get_genesis_height(_dst_dbstore, address);
            return height;
        }

        uint64_t get_v3_db_genesis_height(const base::xvaccount_t & address, base::xvdbstore_t* _dst_dbstore, base::xvblockstore_t* _dst_blockstore) {
            uint64_t height;
            if (address.get_address() == sys_drand_addr) {
                height = _dst_blockstore->get_latest_connected_block_height(address);
            } else {
                height = xblockdb_v2_t::get_v3_genesis_height(_dst_dbstore, address);
            }
            return height;
        }

        bool  db_delta_migrate_v2_to_v3_block(const base::xvaccount_t & address, xblockdb_v2_t* _src_dbstore, base::xvblockstore_t* _dst_blockstore, uint64_t & total_count)
        {
            base::xvdbstore_t* _dst_dbstore = base::xvchain_t::instance().get_xdbstore();
            uint64_t src_db_min_height = get_v2_db_genesis_height(address, _src_dbstore->get_xdbstore());
            uint64_t dst_db_min_height = get_v3_db_genesis_height(address, _dst_dbstore, _dst_blockstore);

            base::xauto_ptr<base::xvactmeta_t> src_meta = xblockdb_v2_t::get_meta(_src_dbstore->get_xdbstore(), address);
            base::xauto_ptr<base::xvactmeta_t> dst_meta = xblockdb_v2_t::get_v3_meta(_dst_dbstore, address);
            uint64_t src_db_max_height = src_meta->clone_block_meta()._highest_cert_block_height;
            xinfo("db_delta_migrate_v2_to_v3_block address=%s,srcmeta=%s,dst_meta=%s,src_height=%ld,dst_height=%ld", address.get_account().c_str(),src_meta->clone_block_meta().ddump().c_str(),dst_meta->clone_block_meta().ddump().c_str(),src_db_min_height,dst_db_min_height);
            uint64_t begin_height = dst_db_min_height + 1; // always migrate block from dst_db_min_height + 1
            uint64_t end_height = src_db_max_height;
            if (begin_height > end_height) {
                return true;
            }

            uint64_t count = 0;
            for (uint64_t h = begin_height; h <= end_height; h++) {
                std::vector<xobject_ptr_t<base::xvblock_t>> blocks;
                _src_dbstore->load_blocks(address, h, blocks);
                // store table block to dst db
                for (auto & block : blocks) {
                    // reset block local flags
                    block->reset_block_flags();
                    block->set_block_flag(enum_xvblock_flag_authenticated);
                    _dst_blockstore->store_block(address, block.get());
                    count++;
                }
            }
            total_count += count;
            xassert(count > 0);
            base::xauto_ptr<base::xvactmeta_t> dst_meta_new = xblockdb_v2_t::get_v3_meta(_dst_dbstore, address);
            uint64_t dst_db_min_height_new = get_v3_db_genesis_height(address, _dst_dbstore, _dst_blockstore);
            xinfo("db_delta_migrate_v2_to_v3_block migrated_done.address=%s,srcmeta=%s,dst_meta=%s,dst_height=%ld,count=%ld", address.get_account().c_str(),src_meta->clone_block_meta().ddump().c_str(),dst_meta_new->clone_block_meta().ddump().c_str(),dst_db_min_height_new,count);
            return true;
        }


        void  db_delta_migrate_v2_to_v3_addresses(const std::vector<std::string> & addresses, xblockdb_v2_t* _src_dbstore, base::xvblockstore_t* _dst_blockstore, uint64_t & total_count)
        {
            for (auto & addr : addresses) {
                base::xvaccount_t _vaddr(addr);
                db_delta_migrate_v2_to_v3_block(_vaddr, _src_dbstore, _dst_blockstore, total_count);
            }
        }

        bool db_delta_migrate_v2_to_v3(const std::string & src_db_path, base::xvblockstore_t* dst_blockstore)
        {
            if (src_db_path.empty() || nullptr == dst_blockstore) {
                xassert(false);
                return false;
            }

            // set latest db version
            std::string org_db_version = base::xvchain_t::instance().get_xdbstore()->get_value(xvdbkey_t::get_xdb_version_key());
            base::xvchain_t::instance().get_xdbstore()->set_value(xvdbkey_t::get_xdb_version_key(), DB_VERSION_V3);

            xkinfo("db_delta_migrate_v2_to_v3 begin.org_db_version=%s,src_db_path=%s", org_db_version.c_str(), src_db_path.c_str());
            std::cout << "db_delta_migrate_v2_to_v3 begin" << std::endl;
            int64_t begin_s = base::xtime_utl::gettimeofday();
            // if src db path not exist, no need do migrate
            if (!base::xfile_utl::folder_exist(src_db_path)) {
                xinfo("db_delta_migrate_v2_to_v3, src db path folder not exist.for src db path(%s)",src_db_path.c_str());
                return true;
            }

            int src_db_kind = db::xdb_kind_kvdb | db::xdb_kind_no_multi_cf;  // dbv2 has no multi cf. blockstore may write, so shoul not use readonly option
            xobject_ptr_t<xmigratedb_t> src_dbstore = make_object_ptr<xmigratedb_t>(src_db_kind, src_db_path);
            if(src_dbstore->open_db() == false) {
                xerror("db_delta_migrate_v2_to_v3,failed to open src DB at path(%s)",src_db_path.c_str());
                return false;
            }

            xobject_ptr_t<xblockdb_v2_t> _vblockdb = make_object_ptr<xblockdb_v2_t>(src_dbstore.get());
            uint64_t total_count{0};
            // migrate delta table-blocks
            std::vector<std::string> all_table_addrs = data::xblocktool_t::make_all_table_addresses();
            all_table_addrs.push_back(sys_drand_addr);

            const size_t THREAD_NUM = 8;
            uint64_t thread_total_count[THREAD_NUM];
            memset(thread_total_count, 0, sizeof(thread_total_count));
            std::vector<std::string> thread_address[THREAD_NUM];
            size_t thread_index = 0;
            for (size_t i = 0; i < all_table_addrs.size(); i++) {
                auto & addr = all_table_addrs[i];
                thread_address[thread_index].push_back(addr);                
                // std::cout << "db_delta_migrate_v2_to_v3 thread_index=" << thread_index << " addr=" << addr << std::endl;
                thread_index++;
                thread_index = thread_index % THREAD_NUM;
            }

            std::vector<std::thread> all_thread;
            for (size_t i = 0; i < THREAD_NUM; i++) {
                std::vector<std::string> addresses = thread_address[i];
                std::thread th(db_delta_migrate_v2_to_v3_addresses, addresses, _vblockdb.get(), dst_blockstore, std::ref(thread_total_count[i]));
                all_thread.emplace_back(std::move(th));
            }
            for (size_t i = 0; i < THREAD_NUM; i++) {
                all_thread[i].join();
                total_count += thread_total_count[i];
            }

            if (total_count > 50000) {
                // do compact if migrate blocks too much
                std::string begin_key;
                std::string end_key;
                base::xvchain_t::instance().get_xdbstore()->compact_range(begin_key, end_key);
            }

            int64_t end_s = base::xtime_utl::gettimeofday();
            xkinfo("db_delta_migrate_v2_to_v3 finish.total_time_s = %ld,total_count=%ld", end_s - begin_s, total_count);
            std::cout << "db_delta_migrate_v2_to_v3 finish.thread = " << THREAD_NUM << " total_time_s = " << end_s - begin_s << " total_count=" << total_count << std::endl;
            return true;
        }
    
    }//end of namespace of base
}//end of namespace top
