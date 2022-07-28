#include "tests/xevm_contract_runtime/test_evm_eth_bridge_contract_fixture.h"
#include "xbasic/xhex.h"
#include "xdepends/include/ethash/ethash.hpp"
#include "xevm_common/rlp.h"
#include "xevm_common/xabi_decoder.h"
#include "xevm_common/xcrosschain/xheco_snapshot.h"
#include "xevm_common/xeth/xethash.h"

#define private public
#include "xevm_common/xeth/xeth_header.h"
#include "xevm_contract_runtime/xevm_sys_contract_face.h"

namespace top {
namespace tests {

using namespace contract_runtime::evm::sys_contract;
using namespace evm_common;
using namespace evm_common::eth;
using namespace ethash;

const char * header_rlp = "f9025fa08dc59c44d8f2b880dd15645b02192e590604fe0da2a36e9cb4aedb72a63bd8dea01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d4934794140a9c525b8825b4d2a9f665ddfafb404fc7c4f5a0c006bb1297601f52e4180d35c00b8b5a7a09ee57e17b53ec9537998e8040859fa02a6645b4bd5de63016c3f9e7170a96c6a8e4b802ec4a9b7a9aeac5faef2a6819a04cf972ed0f20c06233dd8012e24bf8d22a1ffaffa55d4220f63d3c6603de3ccab901000100120000000000400010000020000100000000084000000000100008200080430050000000000028100004201002000000020084020000001000001031000100002000000240040020002d0022800000001080010000000280047500004880000000000210000020000000000208100100010208040002000000b04800000000000080000000000840000000040a00002004800002142420004800800200004210002110000004000000100000004000000000c421000040001000000009000000000200000000010000000c0000000041120420000001400000000800610080100000000000e0000005000000002002080000000000102000000400020001028401079bac8402625a00831e338f8462de153ab861d883010202846765746888676f312e31372e33856c696e757800000000000000029a82e55d1d25a1354533a1bc75717d6de318bc0d84df37e3e333eb97479a7160bb67013d6b2fdec39e7e6262a812c8b3f28861dcfb07ac7d6237b0843ab6b201a0000000000000000000000000000000000000000000000000000000000000000088000000000000000080";
const char * data = "f9032ab90262f9025fa08dc59c44d8f2b880dd15645b02192e590604fe0da2a36e9cb4aedb72a63bd8dea01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d4934794140a9c525b8825b4d2a9f665ddfafb404fc7c4f5a0c006bb1297601f52e4180d35c00b8b5a7a09ee57e17b53ec9537998e8040859fa02a6645b4bd5de63016c3f9e7170a96c6a8e4b802ec4a9b7a9aeac5faef2a6819a04cf972ed0f20c06233dd8012e24bf8d22a1ffaffa55d4220f63d3c6603de3ccab901000100120000000000400010000020000100000000084000000000100008200080430050000000000028100004201002000000020084020000001000001031000100002000000240040020002d0022800000001080010000000280047500004880000000000210000020000000000208100100010208040002000000b04800000000000080000000000840000000040a00002004800002142420004800800200004210002110000004000000100000004000000000c421000040001000000009000000000200000000010000000c0000000041120420000001400000000800610080100000000000e0000005000000002002080000000000102000000400020001028401079bac8402625a00831e338f8462de153ab861d883010202846765746888676f312e31372e33856c696e757800000000000000029a82e55d1d25a1354533a1bc75717d6de318bc0d84df37e3e333eb97479a7160bb67013d6b2fdec39e7e6262a812c8b3f28861dcfb07ac7d6237b0843ab6b201a00000000000000000000000000000000000000000000000000000000000000000880000000000000000808401079baca09b253032f692647599d53f2fe1418538431752872b307042e09994c31f14ba5303f83f94ff0000000000000000000000000000000000000294ff0000000000000000000000000000000000000394ff0000000000000000000000000000000000000403f85a8800000000000003e894ff000000000000000000000000000000000000028800000000000003e894ff000000000000000000000000000000000000038800000000000003e894ff00000000000000000000000000000000000004";

void header_rlp_bytes_decode(const char * hex_input) {
    std::error_code ec;
    auto bytes = top::from_hex(hex_input, ec);
    auto item = RLP::decode_once(bytes);
    auto header_bytes = item.decoded[0];
    xeth_header_t header;
    header.decode_rlp(header_bytes);
    header.print();
}

TEST(test, header) {
    // header_rlp_bytes_decode(header_rlp);

    auto bytes = top::from_hex(header_rlp);
    auto item = RLP::decode_once(bytes);
    auto header_bytes = item.decoded[0];
    xeth_header_t header;
    header.decode_rlp(header_bytes);
    auto address_bytes = heco::ecrecover(header);
    printf("address: %s\n", to_hex(address_bytes).c_str());
}

static heco::snapshot parse_rlp(const xbytes_t & rlp_bytes) {
    return {};
}

TEST(test, test) {
    auto rlp_bytes = from_hex(data);
    // auto decode_items = RLP::decode(rlp_bytes);
    // printf("%lu, %lu\n", decode_items.decoded.size(), decode_items.remainder.size());

    auto left_bytes = std::move(rlp_bytes);
    while (left_bytes.size() != 0) {
        RLP::DecodedItem item = RLP::decode(left_bytes);
        auto decoded_size = item.decoded.size();
        if (decoded_size < 4) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] rlp param error");
            return;
        }
        left_bytes = std::move(item.remainder);
        xeth_header_t header;
        {
            auto item_header = RLP::decode_once(item.decoded[0]);
            auto header_bytes = item_header.decoded[0];
            if (header.decode_rlp(header_bytes) == false) {
                xwarn("[xtop_evm_eth_bridge_contract::sync] decode header error");
                return;
            }
        }
        header.print();

        auto height = static_cast<uint64_t>(evm_common::fromBigEndian<u64>(item.decoded[1]));
        if (height != header.number) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] number mismatch");
            return;
        }
        auto hash = static_cast<h256>(item.decoded[2]);
        if (hash != header.hash()) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] hash mismatch");
            return;
        }

        printf("height: %lu, hash: %s\n", height, hash.hex().c_str());

        auto validator_num = static_cast<uint64_t>(evm_common::fromBigEndian<u64>(item.decoded[3]));
        if (decoded_size < validator_num + 4 + 1) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] rlp param error");
            return;
        }

        heco::snapshot snap;
        snap.number = height;
        snap.hash = hash;
        for (uint64_t i = 0; i < validator_num; ++i) {
            snap.validators.insert(item.decoded[i + 4]);
            printf("validator: %s\n", to_hex(item.decoded[i + 4]).c_str());
        }

        auto recent_num = static_cast<uint64_t>(evm_common::fromBigEndian<u64>(item.decoded[3 + validator_num + 1]));
        if (decoded_size < validator_num + 4 + 1 + recent_num) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] rlp param error");
            return;
        }

        for (uint64_t i = 0; i < recent_num; ++i) {
            uint64_t k = static_cast<uint64_t>(evm_common::fromBigEndian<u64>(item.decoded[3 + validator_num + 1 + 1 + i * 2]));
            snap.recents[k] = item.decoded[3 + validator_num + 1 + 1 + i * 2 + 1];
            printf("recents: %lu, %s\n", k, to_hex(item.decoded[3 + validator_num + 1 + 1 + i * 2 + 1]).c_str());
        }

        snap.digest();
    }
}

TEST(test, init_decode) {
    const char * data = "f90286b90262f9025fa08dc59c44d8f2b880dd15645b02192e590604fe0da2a36e9cb4aedb72a63bd8dea01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d4934794140a9c525b8825b4d2a9f665ddfafb404fc7c4f5a0c006bb1297601f52e4180d35c00b8b5a7a09ee57e17b53ec9537998e8040859fa02a6645b4bd5de63016c3f9e7170a96c6a8e4b802ec4a9b7a9aeac5faef2a6819a04cf972ed0f20c06233dd8012e24bf8d22a1ffaffa55d4220f63d3c6603de3ccab901000100120000000000400010000020000100000000084000000000100008200080430050000000000028100004201002000000020084020000001000001031000100002000000240040020002d0022800000001080010000000280047500004880000000000210000020000000000208100100010208040002000000b04800000000000080000000000840000000040a00002004800002142420004800800200004210002110000004000000100000004000000000c421000040001000000009000000000200000000010000000c0000000041120420000001400000000800610080100000000000e0000005000000002002080000000000102000000400020001028401079bac8402625a00831e338f8462de153ab861d883010202846765746888676f312e31372e33856c696e757800000000000000029a82e55d1d25a1354533a1bc75717d6de318bc0d84df37e3e333eb97479a7160bb67013d6b2fdec39e7e6262a812c8b3f28861dcfb07ac7d6237b0843ab6b201a0000000000000000000000000000000000000000000000000000000000000000088000000000000000080a09b253032f692647599d53f2fe1418538431752872b307042e09994c31f14ba53";
    auto rlp_bytes = from_hex(data);
    auto item = RLP::decode(rlp_bytes);
    xeth_header_t header;
    {
        auto item_header = RLP::decode_once(item.decoded[0]);
        auto header_bytes = item_header.decoded[0];
        if (header.decode_rlp(header_bytes) == false) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] decode header error");
            return;
        }
    }
    header.print();
    auto hash = static_cast<h256>(item.decoded[1]);
    printf("hash: %s\n", hash.hex().c_str());
}

}
}