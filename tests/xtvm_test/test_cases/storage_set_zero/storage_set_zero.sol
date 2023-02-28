// SPDX-License-Identifier: GPL-3.0

pragma solidity >=0.7.0 <0.9.0;

contract Test1 {
    bool public a = true;
}

contract Test2 is Test1 {
    uint256 public b = 10;

    function setB(uint256 c) public {
        b = c;
    }
}
