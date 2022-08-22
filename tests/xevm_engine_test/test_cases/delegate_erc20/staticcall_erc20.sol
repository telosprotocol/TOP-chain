// SPDX-License-Identifier: MIT
pragma solidity 0.8.10;

contract TopErc20Wrapper {
    address constant TopErc20 = 0xff00000000000000000000000000000000000005;

    string public name = "TOP Network";
    string public symbol = "TOP";
    bytes1 public chain_uuid;

    constructor(bytes1 chain_uuid_) {
        chain_uuid = chain_uuid_;
    }

    fallback() external {
        bytes memory call_data = abi.encodePacked(chain_uuid, msg.data);
        (bool success, bytes memory result) = TopErc20.staticcall(call_data);

        require(success, string(result));

        assembly {
            return(add(result, 0x20), mload(result))
        }
    }
}

contract EthWrapper {
    address constant Eth = 0xFF00000000000000000000000000000000000006;

    string public name = "ETH";
    string public symbol = "ETH";
    bytes1 public chain_uuid;

    constructor(bytes1 chain_uuid_) {
        chain_uuid = chain_uuid_;
    }

    fallback() external {
        bytes memory call_data = abi.encodePacked(chain_uuid, msg.data);
        (bool success, bytes memory result) = Eth.staticcall(call_data);

        require(success, string(result));

        assembly {
            return(add(result, 0x20), mload(result))
        }
    }
}

contract UsdtErc20Wrapper {
    address constant TopErc20 = 0xfF00000000000000000000000000000000000007;

    string public name = "Wrapped Tether USD";
    string public symbol = "tUSDT";
    bytes1 public chain_uuid;

    constructor(bytes1 chain_uuid_) {
        chain_uuid = chain_uuid_;
    }

    fallback() external {
        bytes memory call_data = abi.encodePacked(chain_uuid, msg.data);
        (bool success, bytes memory result) = TopErc20.staticcall(call_data);

        require(success, string(result));

        assembly {
            return(add(result, 0x20), mload(result))
        }
    }
}

contract UsdcErc20Wrapper {
    address constant TopErc20 = 0xFF00000000000000000000000000000000000008;

    string public name = "Wrapped USD Coin";
    string public symbol = "tUSDC";
    bytes1 public chain_uuid;

    constructor(bytes1 chain_uuid_) {
        chain_uuid = chain_uuid_;
    }

    fallback() external {
        bytes memory call_data = abi.encodePacked(chain_uuid, msg.data);
        (bool success, bytes memory result) = TopErc20.staticcall(call_data);

        require(success, string(result));

        assembly {
            return(add(result, 0x20), mload(result))
        }
    }
}

