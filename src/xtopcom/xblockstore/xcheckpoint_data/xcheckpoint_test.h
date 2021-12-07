#pragma once

namespace top {
namespace store {
static auto checkpoint_json =
    R"T(
{
    "10": {
        "Ta0000@0": {
            "height": "1",
            "hash": "354e069bdcf8094c132f433b2e445f35a1ced1ebc32bf44e77098e81e374285f"
        }
    },
    "50": {
        "Ta0000@0": {
            "height": "5",
            "hash": "84ced0711bcc248949109e70672a85a25746b3c92146a65a49bda620c76e6efc"
        },
        "Ta0000@1": {
            "height": "6",
            "hash": "1ea9a6f9eb6c7fd493b8f07638474cc37037511d29d6a8a3cd6904b3406a9d07"
        }
    },
    "200": {
        "Ta0000@0": {
            "height": "20",
            "hash": "f2c810091ee336f1d7ebe2d462886f52d02a152d5b6f35cc8521ecea2ff16afa"
        },
        "Ta0000@1": {
            "height": "21",
            "hash": "ff5248ad2881c2489c28044404096e461ad86df75c90a9e09c4c85c2b11a0575"
        }
    }
}
)T";
}
}