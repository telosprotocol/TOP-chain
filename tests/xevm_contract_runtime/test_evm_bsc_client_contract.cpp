#include "tests/xevm_contract_runtime/test_evm_eth_bridge_contract_fixture.h"
#include "xbasic/xhex.h"
#include "xevm_common/rlp.h"
#include "xevm_common/xabi_decoder.h"
#include "xevm_common/xcrosschain/xvalidators_snapshot.h"
#include "xevm_contract_runtime/sys_contract/xevm_bsc_client_contract.h"

#define private public
#include "xevm_common/xcrosschain/xeth_header.h"
#include "xevm_contract_runtime/xevm_sys_contract_face.h"

namespace top {
namespace tests {

using namespace contract_runtime::evm::sys_contract;
using namespace evm_common;

class xbsc_contract_fixture_t : public testing::Test {
public:
    xbsc_contract_fixture_t() {
    }

    void init() {
        auto bstate = make_object_ptr<base::xvbstate_t>(evm_eth_bridge_contract_address.value(), (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
        auto canvas = make_object_ptr<base::xvcanvas_t>();
        bstate->new_string_map_var(data::system_contract::XPROPERTY_HEADERS, canvas.get());
        bstate->new_string_map_var(data::system_contract::XPROPERTY_HEADERS_SUMMARY, canvas.get());
        bstate->new_string_map_var(data::system_contract::XPROPERTY_ALL_HASHES, canvas.get());
        bstate->new_string_map_var(data::system_contract::XPROPERTY_EFFECTIVE_HASHES, canvas.get());
        bstate->new_string_var(data::system_contract::XPROPERTY_LAST_HASH, canvas.get());
        auto bytes = (evm_common::h256(0)).asBytes();
        bstate->load_string_var(data::system_contract::XPROPERTY_LAST_HASH)->reset({bytes.begin(), bytes.end()}, canvas.get());
        contract_state = std::make_shared<data::xunit_bstate_t>(bstate.get(), false);
        statectx = make_unique<xmock_statectx_t>(contract_state);
        statectx_observer = make_observer<statectx::xstatectx_face_t>(statectx.get());
        context.address = common::xtop_eth_address::build_from("ff00000000000000000000000000000000000003");
        context.caller = common::xtop_eth_address::build_from("f8a1e199c49c2ae2682ecc5b4a8838b39bab1a38");
    }

    void SetUp() override {
        init();
    }

    void TearDown() override {
    }

    contract_runtime::evm::sys_contract::xevm_bsc_client_contract_t contract;
    data::xunitstate_ptr_t contract_state;
    std::unique_ptr<statectx::xstatectx_face_t> statectx;
    observer_ptr<statectx::xstatectx_face_t> statectx_observer;
    contract_runtime::evm::sys_contract_context context;
};

#include "tests/xevm_contract_runtime/test_evm_bsc_client_contract_data.cpp"

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

TEST_F(xbsc_contract_fixture_t, test_bsc_init) {
    auto bytes = top::from_hex(bsc_init_20250000_hex);
    EXPECT_TRUE(contract.init(bytes, contract_state));
}

TEST_F(xbsc_contract_fixture_t, test_parse_validators) {
    const char * header_hex = "f90403a0c18af239fc81de948d4f7785b1ea8c70eaa23283a1ca29f7bfbd164f91d37201a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d4934794be807dddb074639cd9fa61b47676c064fc50d62ca0cce0dfc134ef9611ed22535b05e1ff39ada7eef0613f62311b11e8af31243225a034000ae64d1693c21abfdce1af136c2b2ebfea48626add1a911a08642830d532a0bc20983deafc6035dd81cf7fea6aff00d6a06e2b2c1db78236e3b97123a9c849b9010000a222890b00c03030240001920c04e014000010e610a5a444300000281045a24080144201801012efe020d190c04032007900d106b00d60e10320402b3f00801e04882055301818013000080501502162100c000cc408908a04320298080000409014ec0e472018904b80c40508892288604042ce84848682240018880004297a04100124090d4000c21210285608a78c651407000a0409011009730027882002394100110952c03610a0218a0a880472000912090083a51208200005620828602c691208064d2722a048001a00d0c0b2429106e90170b109414b831000e0a12070542804434960d1c4848100ca920140027461a1000854200201380008309102840134fd908404c54a9b834ef0368462f07be6b90205d88301010b846765746888676f312e31332e34856c696e75780000005d43d2fd2465176c461afb316ebc773c61faee85a6515daa295e26495cef6f69dfa69911d9d8e4f3bbadb89b2b3a6c089311b478bf629c29d790a7a6db3fc1b92d4c407bbe49438ed859fe965b140dcf1aab71a93f349bbafec1551819b8be1efea2fc46ca749aa161dd481a114a2e761c554b641742c973867899d3685b1ded8013785d6623cc18d214320b6bb6475970f657164e5b75689b64b7fd1fa275f334f28e1872b61c6014342d914470ec7ac2975be345796c2b7ae2f5b9e386cd1b50a4550696d957cb4900f03a8b6c8fd93d6f4cea42bbb345dbc6f0dfdb5bec739f8ccdafcc39f3c7d6ebf637c9151673cbc36b88a6f79b60359f141df90a0c745125b131caaffd12aacf6a8119f7e11623b5a43da638e91f669a130fac0e15a038eedfc68ba3c35c73fed5be4a07afb5be807dddb074639cd9fa61b47676c064fc50d62ce2d3a739effcd3a99387d015e260eefac72ebea1e9ae3261a475a27bb1028f140bc2a7c843318afdea0a6e3c511bbd10f4519ece37dc24887e11b55dee226379db83cffc681495730c11fdde79ba4c0cef0274e31810c9df02f98fafde0f841f4e66a1cd5f0b40e8f4d4c857f4ea521ab99410c2a3d370e1f9f38fc5fa195eb3ed7707fc2f3fd6541d5041cbd30ee63f1aced286eefe989742716965d9927ca0d09e635b01a00000000000000000000000000000000000000000000000000000000000000000880000000000000000";
    auto bytes = top::from_hex(header_hex);
    xeth_header_t header;
    header.decode_rlp(bytes);
    xvalidators_snapshot_t snap;
    EXPECT_TRUE(snap.init_with_epoch(header));
    for (auto v : snap.validators) {
        printf("%s\n", to_hex(v).c_str());
    }
}

TEST_F(xbsc_contract_fixture_t, test_parse_validator) {
    const char * header_hex = "f90403a0c18af239fc81de948d4f7785b1ea8c70eaa23283a1ca29f7bfbd164f91d37201a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d4934794be807dddb074639cd9fa61b47676c064fc50d62ca0cce0dfc134ef9611ed22535b05e1ff39ada7eef0613f62311b11e8af31243225a034000ae64d1693c21abfdce1af136c2b2ebfea48626add1a911a08642830d532a0bc20983deafc6035dd81cf7fea6aff00d6a06e2b2c1db78236e3b97123a9c849b9010000a222890b00c03030240001920c04e014000010e610a5a444300000281045a24080144201801012efe020d190c04032007900d106b00d60e10320402b3f00801e04882055301818013000080501502162100c000cc408908a04320298080000409014ec0e472018904b80c40508892288604042ce84848682240018880004297a04100124090d4000c21210285608a78c651407000a0409011009730027882002394100110952c03610a0218a0a880472000912090083a51208200005620828602c691208064d2722a048001a00d0c0b2429106e90170b109414b831000e0a12070542804434960d1c4848100ca920140027461a1000854200201380008309102840134fd908404c54a9b834ef0368462f07be6b90205d88301010b846765746888676f312e31332e34856c696e75780000005d43d2fd2465176c461afb316ebc773c61faee85a6515daa295e26495cef6f69dfa69911d9d8e4f3bbadb89b2b3a6c089311b478bf629c29d790a7a6db3fc1b92d4c407bbe49438ed859fe965b140dcf1aab71a93f349bbafec1551819b8be1efea2fc46ca749aa161dd481a114a2e761c554b641742c973867899d3685b1ded8013785d6623cc18d214320b6bb6475970f657164e5b75689b64b7fd1fa275f334f28e1872b61c6014342d914470ec7ac2975be345796c2b7ae2f5b9e386cd1b50a4550696d957cb4900f03a8b6c8fd93d6f4cea42bbb345dbc6f0dfdb5bec739f8ccdafcc39f3c7d6ebf637c9151673cbc36b88a6f79b60359f141df90a0c745125b131caaffd12aacf6a8119f7e11623b5a43da638e91f669a130fac0e15a038eedfc68ba3c35c73fed5be4a07afb5be807dddb074639cd9fa61b47676c064fc50d62ce2d3a739effcd3a99387d015e260eefac72ebea1e9ae3261a475a27bb1028f140bc2a7c843318afdea0a6e3c511bbd10f4519ece37dc24887e11b55dee226379db83cffc681495730c11fdde79ba4c0cef0274e31810c9df02f98fafde0f841f4e66a1cd5f0b40e8f4d4c857f4ea521ab99410c2a3d370e1f9f38fc5fa195eb3ed7707fc2f3fd6541d5041cbd30ee63f1aced286eefe989742716965d9927ca0d09e635b01a00000000000000000000000000000000000000000000000000000000000000000880000000000000000";
    const char * apply_header_hex = "f9025ea017e406a592682adc0e9f1eb5c19acb5084802a1934e30bb21e5a0ac278beb03fa01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d4934794e2d3a739effcd3a99387d015e260eefac72ebea1a06f2f480217dcd5a8a4e6faa20cbc226a610d2284f569017bb01c046e449ce251a00b78ef6a1cec327953d87ba99a2425dcb232d20afd340d95db853370ecd41ec1a01b2d5149dd7dd68d4932ef966b0cf89472e7bcb3af9715d0b6aa124194eb2310b90100b07023094e511452632400f190da356195080c6a951a8d8f8338203301a2c040c4b01562e222911c46e36079f0ac256a36781038042f18045dcea04c873486905d718b6eac001a1c1ba801595275523da1706cc94fc478448bb71831a0203612c176082d1a0743beb761207a8068bd503872a5c0d1829c73042b2311c84084b0d7609316158db4016c8c2531073008a188a624d50c40008cc11a14c8084088308b92300811b80a65b4402da10e7ae002c5268203283301404141aa2c816272e936d97402610af5232082014b02401407a140831441135c12625544629031faa011501409f15140204b1476b182d9162241147725c14d20d3a80a76250eea161502840134fd918404ca0fe483ad45448462f07be9b861d88301010b846765746888676f312e31332e34856c696e75780000005d43d2fdfb4d3bbee25e7d95fcf0d75c1e1c1fca457d534534700f97444a64212a8101de1c247ccd2665baa20fdecbc5a0d485583688466e1a723ec80c5e5ebfca66e7fa01a00000000000000000000000000000000000000000000000000000000000000000880000000000000000";
    auto bytes = top::from_hex(header_hex);
    auto apply_bytes = top::from_hex(apply_header_hex);
    xeth_header_t header;
    xeth_header_t apply_header;
    header.decode_rlp(bytes);
    apply_header.decode_rlp(apply_bytes);
    xvalidators_snapshot_t snap;
    EXPECT_TRUE(snap.init_with_epoch(header));
    for (auto v : snap.validators) {
        printf("%s\n", to_hex(v).c_str());
    }
    EXPECT_TRUE(snap.apply_with_chainid(apply_header, 56, false));
}

}
}