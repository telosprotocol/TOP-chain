// SPDX-License-Identifier: MIT
pragma solidity >=0.4.22 <0.9.0;


// 调用别的合约
contract A {
    event ALog(bool success, bytes data);

    // 调用其他合约中的函数
    function testCallSetB(address _addr, uint256 num) public {
        (bool success, bytes memory data) = _addr.call(
            abi.encodeWithSignature("set_B(uint256)", num)
        );
        emit ALog(success, data);
    }

    function testCallGetB(address _addr) public returns (bool, bytes memory) {
        (bool success, bytes memory data) = _addr.call(
            abi.encodeWithSignature("get_B()")
        );
        emit ALog(success, data);
        return (success, data);
    }

    // 调用其他合约中不存在的函数
    function testCallDoesNotExist(address _addr) public {
        (bool success, bytes memory data) = _addr.call(
            abi.encodeWithSignature("doesNotExist()")
        );
        emit ALog(success, data);
    }
}
