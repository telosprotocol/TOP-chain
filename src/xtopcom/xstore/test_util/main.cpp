#include <iostream>
#include <gtest/gtest.h>
#include "xbase/xlog.h"
#include "xbase/xhash.h"
#include "xstore/xstore.h"
#include "xutility/xhash.h"

using namespace std;
using namespace top;

class xhashtest_t : public top::base::xhashplugin_t
{
public:
    xhashtest_t():
        top::base::xhashplugin_t(-1) //-1 = support every hash types
    {
    }
private:
    xhashtest_t(const xhashtest_t &);
    xhashtest_t & operator = (const xhashtest_t &);
    virtual ~xhashtest_t(){};
public:
    virtual const std::string hash(const std::string & input,enum_xhash_type type) override
    {
        auto hash = top::utl::xsha2_256_t::digest(input);
        return std::string(reinterpret_cast<char*>(hash.data()), hash.size());
    }
};

int main(int argc, char **argv) {
    new xhashtest_t();

    if (argc != 4) {
        std::cout << "usage: " << argv[0] << " db_path account_name height" << std::endl;
    }
    std::string db_path = argv[1];
    std::string account = argv[2];
    uint64_t height = atol(argv[3]);

    xinit_log("./xstore_test.log", true, true);
    xset_log_level(enum_xlog_level_debug);
    xdbg("------------------------------------------------------------------");
    xinfo("new log start here");


    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_static_kvdb(db_path, nullptr);
    data::xblock_t* block = store->get_block_by_height(account, height);
    if (block != nullptr) {
        std::cout << block->dump() << std::endl;
        const auto& property_hash_map = block->get_property_hash_map();

        for (const auto& entry : property_hash_map) {
            std::cout << entry.first << ":" << data::to_hex_str(entry.second) << std::endl;
        }
    }
}
