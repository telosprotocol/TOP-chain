#include "gtest/gtest.h"
#include "xbase/xmem.h"
#include "xbase/xdata.h"
#include "xbase/xcontext.h"
#include "xdata/xdata_defines.h"
#include "xbase/xobject_ptr.h"
#include "xdata/xblock.h"
#include "xdata/xdatautil.h"

using namespace top;
using namespace top::base;
using namespace top::data;

class test_transaction : public testing::Test {
 protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

class test_transaction_1 : public xbase_dataunit_t<test_transaction_1, 100> {
 public:
    ~test_transaction_1() override {}
 public:
    virtual int32_t do_write(base::xstream_t &stream) override {
        const int32_t begin_size = stream.size();
        stream << m_value1;
        const int32_t end_size = stream.size();
        return (end_size - begin_size);
    }
    virtual int32_t do_read(base::xstream_t &stream) override {
        const int32_t begin_size = stream.size();
        stream >> m_value1;
        const int32_t end_size = stream.size();
        return (begin_size - end_size);
    }

 public:
    std::string m_value1;
};
REG_CLS(test_transaction_1);


TEST_F(test_transaction, transaction_serialize_1) {
    base::xstream_t stream(base::xcontext_t::instance());
    {
        xobject_ptr_t<test_transaction_1> t1 = make_object_ptr<test_transaction_1>();
        t1->m_value1 = "hello";
        t1->serialize_to(stream);
    }

    {
        xdataunit_t* data = base::xdataunit_t::read_from(stream);
        xdataunit_ptr_t data_ptr;
        data_ptr.attach(data);
        xobject_ptr_t<test_transaction_1> t2 = dynamic_xobject_ptr_cast<test_transaction_1>(data_ptr);
        ASSERT_EQ(t2->m_value1, "hello");
    }
}

