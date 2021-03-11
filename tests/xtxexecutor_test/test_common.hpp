#include "gtest/gtest.h"
#include "xtxexecutor/xunit_maker.h"
#include "xstore/xstore_face.h"
#include "xstore/test/test_datamock.hpp"
#include "xstore/xaccount_context.h"
#include "xblockstore/xblockstore_face.h"
#include "xdata/xtransaction_maker.hpp"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xdatamock_tx.hpp"
#include "tests/mock/xcertauth_util.hpp"

using namespace top::txexecutor;
using namespace top::store;
using namespace top::base;
using namespace top::mock;

class test_xblockmaker_resources_t : public xblockmaker_resources_t {
 public:
    static xblockmaker_resources_ptr_t create() {
        xblockmaker_resources_ptr_t resources = std::make_shared<test_xblockmaker_resources_t>();
        return resources;
    }

 public:
    test_xblockmaker_resources_t() {
        xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();
        auto store = store_face.get();
        m_store = store_face;
        xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, {});
        m_blockstore.attach(blockstore);
    }

    virtual store::xstore_face_t*       get_store() const {return m_store.get();}
    virtual base::xvblockstore_t*       get_blockstore() const {return m_blockstore.get();}
    virtual xtxpool::xtxpool_face_t*    get_txpool() const {return nullptr;}

 private:
    xstore_face_ptr_t               m_store{nullptr};
    xobject_ptr_t<xvblockstore_t>   m_blockstore{nullptr};
};


