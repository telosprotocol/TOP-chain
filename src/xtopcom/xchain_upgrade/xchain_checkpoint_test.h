#pragma once

namespace top
{
    namespace chain_checkpoint
    {
        static auto checkpoint_json = 
            R"T(
{
    "10": {
        "Ta0000@0": {
            "height": "1",
            "hash": "abcd"
        },
        "Ta0000@1": {
            "height": "2",
            "hash": "abcd"
        },
    },
    "50": {
        "Ta0000@0": {
            "height": "5",
            "hash": "abcd"
        },
        "Ta0000@1": {
            "height": "6",
            "hash": "abcd"
        },
    },
    "200": {
        "Ta0000@0": {
            "height": "20",
            "hash": "abcd"
        },
        "Ta0000@1": {
            "height": "21",
            "hash": "abcd"
        },
    },
}
)T";
    }
}