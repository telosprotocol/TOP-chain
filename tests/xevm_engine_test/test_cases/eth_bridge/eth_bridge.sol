// SPDX-License-Identifier: MIT
pragma solidity >=0.5.12;

contract EthBridge {
    address constant TopEthBridge = 0xff00000000000000000000000000000000000002;

    fallback() external {
        bytes memory call_data = abi.encodePacked(msg.data);
        (bool success, bytes memory result) = TopEthBridge.delegatecall(call_data);

        require(success, string(result));

        assembly {
            return(add(result, 0x20), mload(result))
        }
    }
}
