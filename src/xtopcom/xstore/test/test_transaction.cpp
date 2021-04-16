#include <vector>
#include <iostream>

#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "gtest/gtest.h"
#include "xbase/xcontext.h"
#include "xbase/xobject_ptr.h"
#include "xdata/xnative_property.h"
#include "xstore/xstore_face.h"
#include "xstore/test/test_datamock.hpp"

using namespace top;
using namespace top::store;

class test_transaction : public testing::Test {
 protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

