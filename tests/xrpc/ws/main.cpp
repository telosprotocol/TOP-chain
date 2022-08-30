#include <gtest/gtest.h>
#include <memory>

#include "xstore/xstore.h"
#include "xdb/xdb_factory.h"
#include "xbase/xcontext.h"

int main(int argc, char * argv[])
{
    testing::InitGoogleTest(&argc, argv);
    using namespace top;
    using top::base::xcontext_t;
    using namespace top::store;
    // top::topchain_init(argc, argv);
    // std::shared_ptr<xdb_face_t> rocksdb = xdb_factory_t::create_kvdb("./");
    // xstore_face_t* store = new xstore(ledger);
    // xcontext_t::instance().set_global_object(enum_xtop_global_object_store, static_cast<xobject_t*>(store));

    // consensus_service::xunit_service_face* consensus_mock = new top::xrpc::test::xconsensus_mock();
    // assert(consensus_mock != nullptr);
    // xcontext_t::instance().set_global_object(enum_xtop_global_object_unit_service, static_cast<xobject_t*>(consensus_mock));
    return RUN_ALL_TESTS();
}
