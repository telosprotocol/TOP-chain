// #include "gtest/gtest.h"
// 
// #include "xdata/xcons_transaction.h"

// using namespace top;

// class test_xtxexecutor_util_t {
//  public:
//     // static bool save_and_execute_block(store::xstore_face_t* store, base::xvblock_t* block) {
//     //     bool ret = store->set_vblock(block);
//     //     xassert(ret == true);
//     //     block->set_block_flag(base::enum_xvblock_flag_connected);
//     //     ret = store->execute_block(block);
//     //     xassert(ret == true);
//     //     return ret;
//     // }

//     static xcons_transaction_ptr_t create_mock_recv_tx(xtransaction_t* rawtx, const xtransaction_exec_state_t & state) {
//         xlightunit_output_entity_t* txinfo = new xlightunit_output_entity_t(enum_transaction_subtype_send, rawtx, state);
//         xblockcert_t* cert = new xblockcert_t();
//         xmerkle_path_256_t path;
//         xtx_receipt_ptr_t txreceipt = make_object_ptr<xtx_receipt_t>(txinfo, path, cert);
//         xcons_transaction_ptr_t constx = make_object_ptr<xcons_transaction_t>(rawtx, txreceipt);
//         txinfo->release_ref();
//         cert->release_ref();
//         return constx;
//     }

//     static xcons_transaction_ptr_t create_mock_confirm_tx(xtransaction_t* rawtx, const xtransaction_exec_state_t & state) {
//         xlightunit_output_entity_t* txinfo = new xlightunit_output_entity_t(enum_transaction_subtype_recv, rawtx, state);
//         xblockcert_t* cert = new xblockcert_t();
//         xmerkle_path_256_t path;
//         xtx_receipt_ptr_t txreceipt = make_object_ptr<xtx_receipt_t>(txinfo, path, cert);
//         xcons_transaction_ptr_t constx = make_object_ptr<xcons_transaction_t>(rawtx, txreceipt);
//         txinfo->release_ref();
//         cert->release_ref();
//         return constx;
//     }
// };
