#include "tests/xevm_contract_runtime/test_evm_eth_bridge_contract_fixture.h"
#include "xbasic/xhex.h"
#include "xcommon/rlp.h"
#include "xevm_common/xabi_decoder.h"
#include "xevm_common/xcrosschain/xvalidators_snapshot.h"

#define private public
#include "xevm_common/xcrosschain/xeth_header.h"
#include "xevm_contract_runtime/xevm_sys_contract_face.h"

namespace top {
namespace tests {

using namespace contract_runtime::evm::sys_contract;
using namespace evm_common;

class xheco_contract_fixture_t : public testing::Test {
public:
    xheco_contract_fixture_t() {
    }

    void init() {
        auto bstate = make_object_ptr<base::xvbstate_t>(
            evm_eth_bridge_contract_address.to_string(), (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
        auto canvas = make_object_ptr<base::xvcanvas_t>();
        bstate->new_string_map_var(data::system_contract::XPROPERTY_HEADERS, canvas.get());
        bstate->new_string_map_var(data::system_contract::XPROPERTY_HEADERS_SUMMARY, canvas.get());
        bstate->new_string_map_var(data::system_contract::XPROPERTY_ALL_HASHES, canvas.get());
        bstate->new_string_map_var(data::system_contract::XPROPERTY_EFFECTIVE_HASHES, canvas.get());
        bstate->new_string_var(data::system_contract::XPROPERTY_LAST_HASH, canvas.get());
        auto bytes = (evm_common::h256(0)).asBytes();
        bstate->load_string_var(data::system_contract::XPROPERTY_LAST_HASH)->reset({bytes.begin(), bytes.end()}, canvas.get());
        contract_state = std::make_shared<data::xunit_bstate_t>(bstate.get(), bstate.get());
        statectx = top::make_unique<xmock_statectx_t>(contract_state);
        statectx_observer = make_observer<statectx::xstatectx_face_t>(statectx.get());
        context.address = common::xtop_eth_address::build_from("ff00000000000000000000000000000000000004");
        context.caller = common::xtop_eth_address::build_from("f8a1e199c49c2ae2682ecc5b4a8838b39bab1a38");
    }

    void SetUp() override {
        init();
    }

    void TearDown() override {
    }

    contract_runtime::evm::sys_contract::xevm_heco_client_contract_t contract;
    data::xunitstate_ptr_t contract_state;
    std::unique_ptr<statectx::xstatectx_face_t> statectx;
    observer_ptr<statectx::xstatectx_face_t> statectx_observer;
    contract_runtime::evm::sys_contract_context context;
};

#include "tests/xevm_contract_runtime/test_evm_heco_client_contract_data.cpp"

// TEST(test, test) {
//     auto rlp_bytes = from_hex(heco_sync_17276012_hex);
//     // auto decode_items = RLP::decode(rlp_bytes);
//     // printf("%lu, %lu\n", decode_items.decoded.size(), decode_items.remainder.size());

//     auto left_bytes = std::move(rlp_bytes);
//     while (left_bytes.size() != 0) {
//         RLP::DecodedItem item = RLP::decode(left_bytes);
//         auto decoded_size = item.decoded.size();
//         if (decoded_size < 4) {
//             xwarn("[xtop_evm_eth_bridge_contract::sync] rlp param error");
//             return;
//         }
//         left_bytes = std::move(item.remainder);
//         xeth_header_t header;
//         {
//             auto item_header = RLP::decode_once(item.decoded[0]);
//             auto header_bytes = item_header.decoded[0];
//             if (header.decode_rlp(header_bytes) == false) {
//                 xwarn("[xtop_evm_eth_bridge_contract::sync] decode header error");
//                 return;
//             }
//         }
//         header.print();

//         auto validator_num = static_cast<uint64_t>(evm_common::fromBigEndian<u64>(item.decoded[1]));
//         if (decoded_size < validator_num + 2 + 1) {
//             xwarn("[xtop_evm_eth_bridge_contract::sync] rlp param error");
//             return;
//         }

//         heco::xheco_snapshot_t snap;
//         snap.number = static_cast<uint64_t>(header.number - 1);
//         snap.hash = header.parent_hash;
//         for (uint64_t i = 0; i < validator_num; ++i) {
//             snap.validators.insert(item.decoded[i + 2]);
//             printf("validator: %s\n", to_hex(item.decoded[i + 2]).c_str());
//         }

//         auto recent_num = static_cast<uint64_t>(evm_common::fromBigEndian<u64>(item.decoded[1 + validator_num + 1]));
//         if (decoded_size < validator_num + 2 + 1 + recent_num) {
//             xwarn("[xtop_evm_eth_bridge_contract::sync] rlp param error");
//             return;
//         }

//         for (uint64_t i = 0; i < recent_num; ++i) {
//             uint64_t k = static_cast<uint64_t>(evm_common::fromBigEndian<u64>(item.decoded[1 + validator_num + 1 + 1 + i * 2]));
//             snap.recents[k] = item.decoded[1 + validator_num + 1 + 1 + i * 2 + 1];
//             printf("recents: %lu, %s\n", k, to_hex(item.decoded[1 + validator_num + 1 + 1 + i * 2 + 1]).c_str());
//         }
//         snap.print();
//     }
// }

TEST_F(xheco_contract_fixture_t, test_heco_init) {
    auto bytes = top::from_hex(heco_init_17276000_hex);
    EXPECT_TRUE(contract.init(bytes, contract_state));
}

TEST_F(xheco_contract_fixture_t, test_heco_sync) {
    auto init_bytes = top::from_hex(heco_init_17276000_hex);
    EXPECT_TRUE(contract.init(init_bytes, contract_state));
    auto sycn_bytes = top::from_hex(heco_sync_17276012_hex);
    EXPECT_TRUE(contract.sync(sycn_bytes, contract_state));
}

}
}