#include <iostream>
#include <vector>

#include "xbase/xlog.h"
#include "xbase/xhash.h"
#include "xblockstore/xblockstore_face.h"
#include "xutility/xhash.h"
#include "xmetrics/xmetrics.h"
#include "xdata/xrootblock.h"

#include "test_xtableinfo.hpp"

using namespace std;
using namespace top;
using namespace top::base;
using namespace top::mock;

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

void blockstore_perf_test(xvblockstore_t*& blockstore, test_blockmock_t& blockmock, table_info_t& table_info, size_t iterations) {
    std::mt19937 gen(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<size_t> interval_dist(4, 10);
    std::uniform_int_distribution<size_t> packed_tx_dist(0, 64);

    std::string property("sample_list");

    base::xvblock_t *curr_block = nullptr;

    size_t total_transactions = 0;
    uint64_t blockstore_duration{0};

    for (size_t index = 0; index < iterations; ++index) {
        std::string table_addr = table_info.get_random_available_table();
        std::set<std::string> packed_unit_addresses;

        // distribute unit into table randomly
        size_t packed_tx_count = packed_tx_dist(gen);
        size_t matched = 0;
        for (size_t j = 0; j < table_info.per_table_unit_count(); ++j) {
            std::string unit_address = table_info.get_random_unit_address(table_addr);
            // check the units in the pending set
            if (table_info.unit_in_pending_table(unit_address)) {
                continue;
            } else {
                packed_unit_addresses.insert(unit_address);
                if (++matched >= packed_tx_count) {
                    break;
                }
            }
        }

        // build the necessary units for the table address
        std::vector<base::xvblock_t*> packed_units;
        for (const auto& unit_account : packed_unit_addresses) {
            base::xvblock_t* prev_block = table_info.get_prev_unitblock(unit_account);
            if (prev_block == nullptr) {
                prev_block = blockmock.create_property_block(prev_block, unit_account, property);
                assert(prev_block != nullptr);
                table_info.update_prev_unitblock(unit_account, prev_block);
            }

            // set property for the account in the height divided by interval
            if (prev_block->get_height() > 0 && prev_block->get_height() % interval_dist(gen) == 0) {
                std::string value(std::to_string(prev_block->get_height()));
                curr_block = blockmock.create_property_block(prev_block, unit_account, property, value);
            } else {
                curr_block = blockmock.create_property_block(prev_block, unit_account, property);
            }

            packed_units.push_back(curr_block);
        }
        base::xvblock_t* prev_tableblock = table_info.get_prev_tableblock(table_addr);
        if (prev_tableblock == nullptr) {
            prev_tableblock = xblocktool_t::create_genesis_empty_table(table_addr);
            assert(prev_tableblock != nullptr);
            table_info.update_prev_tableblock(table_addr, prev_tableblock);
        }

        // create commit/lock/cert table to commit the unit
        base::xauto_ptr<base::xvblock_t> proposal_tableblock = xtableblock_util::create_tableblock(packed_units, prev_tableblock);
        assert(base::enum_vcert_auth_result::enum_successful == xcertauth_util::instance().get_certauth().verify_muti_sign(proposal_tableblock.get()));
        struct timeval start, end;
        gettimeofday(&start, NULL);

        assert(blockstore->store_block(proposal_tableblock.get()));
        gettimeofday(&end, NULL);
        uint64_t elapse = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
        blockstore_duration += elapse;
        total_transactions += packed_units.size() + 1;

        for (auto& unit : packed_units) {
            unit->release_ref();
        }

        // a better way to decide if unit can be packeed
        // add these unit to the pending list and remove committed unit in (height - 2) tableblock
        if (proposal_tableblock->get_height() > 2) {
            // remove the accounts in (height - 2) tableblock
            base::xauto_ptr<base::xvblock_t> commit_block = blockstore->get_latest_committed_block(table_addr);
            assert(commit_block != nullptr);
            assert(commit_block->get_height() == (proposal_tableblock->get_height() - 2));
            if (commit_block->get_block_class() == base::enum_xvblock_class_light) {
                data::xtable_block_t* commit_tableblock = dynamic_cast<data::xtable_block_t*>(commit_block.get());
                assert(commit_tableblock != nullptr);
                // get unit addresses from commit tableblock
                std::vector<xobject_ptr_t<base::xvblock_t>> cache_units;
                commit_tableblock->extract_sub_blocks(cache_units);
                for (auto & cache_unit : cache_units) {
                    // check the units in the pending set
                    const std::string& addr = cache_unit->get_account();
                    table_info.erase_pending_table(addr);
                    // update the previous block (commited unit)
                    table_info.update_prev_unitblock(addr, cache_unit.get());
                }
            }
        }

        table_info.insert_pending_table(packed_unit_addresses);
        table_info.update_prev_tableblock(table_addr, proposal_tableblock.get());
        table_info.set_table_available(table_addr);

        if (index > 0 && index % 20 == 0) {
            std::stringstream ss;
            ss << std::this_thread::get_id();
            ss << " blockstore performance test ";
            ss << " iteration: " << index;
            ss << " count: " << total_transactions;
            ss << ", time: " << blockstore_duration << "ms";
            ss  << ", qps: " << total_transactions * 1000.0 / blockstore_duration << std::endl;

            std::cout << ss.str();
        }
    }

    std::stringstream ss;
    ss << std::this_thread::get_id();
    ss << " blockstore performance test ";
    ss << " iteration: " << iterations;
    ss << " count: " << total_transactions;
    ss << ", time: " << blockstore_duration << "ms";
    ss  << ", qps: " << total_transactions * 1000.0 / blockstore_duration << std::endl;

    std::cout << ss.str();
}

int main(int argc, char **argv) {
    std::cout << "usage: " << argv[0] << " iterations [rocksdb]" << std::endl;
    bool use_memory_db = true;
    size_t iterations = 5000;
    if (argc >= 2) {
        iterations = atoi(argv[1]);
        if (iterations == 0) {
            iterations = 5000;
        }
        if (argc >= 3 && strcmp(argv[2], "rocksdb") == 0) {
            use_memory_db = false;
        }
    }

    new xhashtest_t();

    data::xrootblock_para_t para;
    data::xrootblock_t::init(para);

    std::cout << "xblockstore test main run" << std::endl;

    xinit_log("./xblockstore_mock.log", true, true);
    xset_log_level(enum_xlog_level_debug);
    xdbg("------------------------------------------------------------------");
    xinfo("new log start here");

    unsigned n = std::thread::hardware_concurrency();
    std::string db_path = "blockstore_performance_test";
    XMETRICS_INIT();
    xobject_ptr_t<xstore_face_t> store_face = nullptr;
    if (use_memory_db) {
        store_face = xstore_factory::create_store_with_memdb();
    } else {
        store_face = xstore_factory::create_store_with_kvdb(db_path);
    }
    {
        auto store = store_face.get();
        xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, db_path);

        test_blockmock_t blockmock(store_face.get());
        table_info_t table_info;

        std::cout << "begin performance test." << std::endl;

        std::vector<std::thread> db_thread(n);

        for (unsigned i = 0; i < n; ++i) {
            db_thread[i] = std::move(std::thread(std::bind(blockstore_perf_test, std::ref(blockstore), std::ref(blockmock), std::ref(table_info), iterations)));
        }

        for (unsigned i = 0; i < n; ++i) {
            db_thread[i].join();
        }

        blockstore->release_ref();

        std::string cmd = "rm -rf " + db_path;
        system(cmd.c_str());
    }
    std::this_thread::sleep_for(std::chrono::seconds(1200));
    std::cout << "performance test done." << std::endl;

    return 0;
}
