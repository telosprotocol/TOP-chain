// SPDX-License-Identifier: MIT
pragma solidity >=0.4.22 <0.9.0;

// 被调用的合约
contract B {
    uint256 public num;
    event BLog(address caller, uint256 amount, string message);

    function set_B(uint256 _num) public {
        num = _num + 1;
        emit BLog(msg.sender, num, "set_B was called");
    }

    function get_B() public view returns (uint256) {
        return num;
    }

    fallback() external {
        num = num + 2;
        emit BLog(msg.sender, num, "fallback was called");
    }
}