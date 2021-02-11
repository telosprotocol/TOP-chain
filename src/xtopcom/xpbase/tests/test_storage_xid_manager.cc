//
//  test_xid_manager.cc
//  test
//
//  Created by Sherlock on 12/18/2018.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

// #include <string.h>
// 
// #include <string>
// #include <iostream>
// 
// #include <gtest/gtest.h>
// #define private public
// #define protected public
// #include "xpbase/base/xip_parser.h"
// #include "xpbase/base/xip_generator.h"
// #include "xpbase/base/xid/xid_manager.h"
// #include "xpbase/base/xid/storage_xid_manager.h"
// #include "xkad/routing_table/routing_utils.h"
// 
// namespace top {
// 
// namespace base {
// 
// namespace test {
// 
// class TestStorageXIdManager : public testing::Test {
// public:
//     static void SetUpTestCase() {
//         top::storage::XLedgerDB::Instance()->Init("db_test");
//         sleep(5);
//     }
// 
//     static void TearDownTestCase() {
//     }
// 
//     virtual void SetUp() {
// 
//     }
// 
//     virtual void TearDown() {
//     }
// };
// 
// TEST_F(TestStorageXIdManager, All) {
//     XIDManagerSptr xid_mgr;
//     xid_mgr.reset(new StorageXIDManager());
//     ASSERT_TRUE(xid_mgr->Init(250));
//     XIDType xid_type(kTopStorage,250);
//     ASSERT_TRUE(xid_mgr->CreateXID(xid_type));
//     XIDSptr xid_ptr;
//     ASSERT_TRUE(xid_mgr->GetXID(xid_type, xid_ptr));
//     ASSERT_NE(xid_ptr,nullptr);
//     ASSERT_TRUE(xid_mgr->DeleteXID(xid_type));
//     ASSERT_TRUE(xid_mgr->UnInit());
// }
// 
// }  // namespace test
// 
// }  // namespace kadmlia
// 
// }  // namespace top
