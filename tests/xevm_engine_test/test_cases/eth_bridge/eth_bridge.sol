// SPDX-License-Identifier: GPL-3.0-or-later
pragma solidity ^0.8.0;

contract HeaderSync {
    address public bridgeLight;
    uint64 public height_view;

    constructor(address _bridgeLight) {
        bridgeLight = _bridgeLight;
    }

    fallback() external {
        (bool success, bytes memory result) = bridgeLight.delegatecall(msg.data);
        require(success, string(result));
        assembly {
            return(add(result, 0x20), mload(result))
        }
    }

    function get_height(uint64 chainId) public returns (uint64 height) {
        bytes memory payload = abi.encodeWithSignature("get_height(uint64)", chainId);
        bool success = false;
        bytes memory returnData;
        (success, returnData) = bridgeLight.call(payload);
        require(returnData.length >= height + 32);
        assembly {
            height := mload(add(returnData, 0x20))
        }
        height_view = height;
        require (success);
    }

    function is_confirmed(bytes memory data) public returns (bool success){
        bytes memory payload = abi.encodeWithSignature("is_confirmed(bytes)", data);
        (success,) = bridgeLight.call(payload);
    }
}