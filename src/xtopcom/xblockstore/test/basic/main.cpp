#include <limits.h>
#include "xbase/xutl.h"
#include "xcertauth/xcertauth_face.h"
#include "xblockstore/xsyncvstore_face.h"
#include "xtestca.hpp"
#include "xunitblock.hpp"
#include "xtestdb.hpp"

using namespace top;
using namespace top::test;

namespace top
{
    class xhashtest_t : public base::xhashplugin_t
    {
    public:
        xhashtest_t()
        :base::xhashplugin_t(-1) //-1 = support every hash types
        {
        }
    private:
        xhashtest_t(const xhashtest_t &);
        xhashtest_t & operator = (const xhashtest_t &);
        virtual ~xhashtest_t(){};
    public:
        virtual const std::string hash(const std::string & input,enum_xhash_type type) override
        {
            return base::xstring_utl::tostring(base::xhash64_t::digest(input));
        }
    };
}

//#define __portional_random__
#define __full_random__
#define __fork_test__
//#define __execute_test__
int test_sync_vstore(store::xsyncvstore_t* sync_store)
{
    xvip2_t any_xip={static_cast<xvip_t>(-1),static_cast<uint64_t>(-1)};
    const std::string test_input_output = "input&output";
    const std::string empty_input_output;
    const std::string account_publick_addr = "1234567890abcdef";
    const std::string test_account_address =  top::base::xvaccount_t::make_account_address(top::base::enum_vaccount_addr_type_secp256k1_user_account, 0, account_publick_addr);
    
    base::xvaccount_t test_account_obj(test_account_address);
    
    int total_test_blocks = 250;
    std::vector<base::xvblock_t*>  generated_blocks;
    base::xauto_ptr<base::xvblock_t> genesis_cert_block = sync_store->get_vblockstore()->get_latest_cert_block(test_account_address);
    base::xvblock_t * last_block = genesis_cert_block.get();
    last_block->add_ref();
    generated_blocks.push_back(last_block);
    uint64_t last_view_id = last_block->get_viewid();
    
    for(int i = 1; i < total_test_blocks; ++i)
    {
        xunitblock_t * new_block = NULL;
        const uint32_t random = top::base::xtime_utl::get_fast_randomu();
        if( (random % 2) == 0)
            new_block = xunitblock_t::create_unitblock(test_account_address, last_block->get_height()+1, last_block->get_clock()+1, last_view_id+1, last_block->get_block_hash(), generated_blocks[0]->get_block_hash(), 0, empty_input_output, empty_input_output);
        else
            new_block = xunitblock_t::create_unitblock(test_account_address, last_block->get_height()+1, last_block->get_clock()+1, last_view_id+1, last_block->get_block_hash(), generated_blocks[0]->get_block_hash(), 0, test_input_output, test_input_output);
        
        new_block->get_cert()->set_validator(any_xip);
        new_block->set_verify_signature(std::string("fake-signature"));
        new_block->set_block_flag(base::enum_xvblock_flag_unpacked);
        new_block->set_block_flag(base::enum_xvblock_flag_authenticated);

        
        last_view_id += 1;
        generated_blocks.push_back(new_block);
        last_block = new_block;
    }
    
    #ifdef __fork_test__
    for(int i = 1; i < total_test_blocks; ++i)
    {
        xunitblock_t * new_block = NULL;
        last_block = generated_blocks[i-1];
        const uint32_t random = top::base::xtime_utl::get_fast_randomu();
        if( (random % 2) == 1)
            new_block = xunitblock_t::create_unitblock(test_account_address, last_block->get_height()+1, last_block->get_clock()+1, last_view_id+1, last_block->get_block_hash(), generated_blocks[0]->get_block_hash(), 0, empty_input_output, empty_input_output);
        else
            new_block = xunitblock_t::create_unitblock(test_account_address, last_block->get_height()+1, last_block->get_clock()+1, last_view_id+1, last_block->get_block_hash(), generated_blocks[0]->get_block_hash(), 0, test_input_output, test_input_output);
        
        new_block->get_cert()->set_validator(any_xip);
        new_block->set_verify_signature(std::string("fake-signature"));
        new_block->set_block_flag(base::enum_xvblock_flag_unpacked);
        new_block->set_block_flag(base::enum_xvblock_flag_authenticated);
        
        last_view_id += 1;
        generated_blocks.push_back(new_block);
    }
    #endif
    
    total_test_blocks = (int)generated_blocks.size();//update to actual size
    
#ifdef __portional_random__
    for(int i = 0; i < 100000; ++i)
    {
        const int random_index = base::xtime_utl::get_fast_random(total_test_blocks);
        //if(false == generated_blocks[random_index]->check_block_flag(base::enum_xvblock_flag_committed))
        sync_store->store_block(generated_blocks[random_index]);//push block as random order
    }
#elif defined(__full_random__)
    std::vector<int>  block_indexs;
    block_indexs.resize(total_test_blocks,0);
    for(int i = 0; i < total_test_blocks; ++i)
    {
        block_indexs[i] = i;
    }
    
    for(int i = 0; i < 100000; ++i)
    {
        const int it1 = base::xtime_utl::get_fast_random(total_test_blocks);
        const int it2 = base::xtime_utl::get_fast_random(total_test_blocks);
        
        const int copy = block_indexs[it1];
        block_indexs[it1] = block_indexs[it2];
        block_indexs[it2] = copy;
    }
    
    for(int i = 0; i < total_test_blocks; ++i)
    {
        base::xvblock_t * test_block = generated_blocks[block_indexs[i]];
        if(test_block != NULL)
        {
            if(test_block->get_height() != 0)
            {
                //sync_store->get_vblockstore()->store_block(test_account_obj,test_block);//push block as random order
                sync_store->store_block(test_block);//push block as random order
            }
        }
    }
#else
    for(int i = 0; i < total_test_blocks; ++i)
    {
        base::xvblock_t * test_block = generated_blocks[i];
        if(test_block != NULL)
        {
            if(test_block->get_height() != 0)
            {
                sync_store->store_block(test_block);//push block as random order
            }
        }
    }
#endif //end of __portional_random__
    
#ifdef __execute_test__
    for(int i = 0; i < total_test_blocks; ++i)
    {
        base::xvblock_t * test_block = generated_blocks[i];
        if(test_block != NULL)
        {
            base::xauto_ptr<base::xvblock_t> loaded_block(sync_store->get_vblockstore()->load_block_object(test_account_obj, test_block->get_height(), test_block->get_viewid(),true));
            
            if(loaded_block)
            {
                if(loaded_block->check_block_flag(base::enum_xvblock_flag_committed))
                    sync_store->get_vblockstore()->execute_block(test_account_obj,loaded_block.get());//push block as random order
            }
        }
    }
#endif //end of __execute_test__
    
    sleep(2);

    printf("////////////////////////////////////////////////////////////// \n");
    base::xauto_ptr<base::xvbindex_t> index(sync_store->get_vblockstore()->get_latest_genesis_connected_index(test_account_obj,true));
    printf("latest_genesis_connected_index as detail=%s \n",index->dump().c_str());
    
    for(auto it : generated_blocks)
    {
        base::xauto_ptr<base::xvbindex_t> index(sync_store->get_vblockstore()->load_block_index(test_account_obj, it->get_height(), 0,0));
        if(false == index->check_block_flag(base::enum_xvblock_flag_committed))
            printf("block is not commit as detail=%s \n",index->dump().c_str());
        
        it->release_ref();
    }
    printf("////////////////////////////////////////////////////////////// \n");
    return 0;
}

int main(int argc, const char * argv[]) {
    
    
#ifdef __WIN_PLATFORM__
    xinit_log("C:\\Users\\taylo\\Downloads\\", true, true);
#else
    xinit_log("/tmp/",true,true);
#endif
    
#ifdef DEBUG
    xset_log_level(enum_xlog_level_debug);
#else
    xset_log_level(enum_xlog_level_key_info);
#endif
    
    xdup_trace_to_terminal(true);
    
    new top::xhashtest_t(); //register this plugin into xbase
    
    xschnorrcert_t*       global_certauth   = new xschnorrcert_t(4);
    
    xveventbus_impl * mbus_store = new xveventbus_impl();
    base::xvchain_t::instance().set_xevmbus(mbus_store);
    
    const std::string  default_path = "/";
    xstoredb_t* _persist_db = new xstoredb_t(default_path);
    base::xvchain_t::instance().set_xdbstore(_persist_db);
    base::xvblockstore_t * blockstore_ptr = store::get_vblockstore();
    
    store::xsyncvstore_t* global_sync_store = new store::xsyncvstore_t(*global_certauth,*blockstore_ptr);
    
    const int result = test_sync_vstore(global_sync_store);
    if(result != 0)
    {
        printf("test_sync_vstore failed as error:%d \n",result);
        return -1;
    }
    
    printf("test over, quit now! \n");
    sleep(80);
    return 0;
}
